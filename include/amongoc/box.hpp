#pragma once

#include <amongoc/box.h>

namespace amongoc {

using box = amongoc_box;

/**
 * @brief Match types which can be stored inline within an amongoc_box.
 *
 * The type must meet the following properties:
 *
 * 1. Must qualify as trivially-relocatable. This is the default for any type that
 *    is trivially destructible and trivially move-constructible.
 * 2. Must be either:
 *    a. Trivially destructible AND no larger than AMONGOC_BOX_SMALL_SIZE
 *    b. OR no larger than AMONGOC_BOX_SMALL_SIZE_WITH_DTOR
 */
template <typename T>
concept box_inlinable_type = enable_trivially_relocatable_v<T>
    and ((std::is_trivially_destructible_v<T> and sizeof(T) <= AMONGOC_BOX_SMALL_SIZE)
         or (sizeof(T) <= AMONGOC_BOX_SMALL_SIZE_WITH_DTOR));

struct unique_box : mlib::unique<::amongoc_box> {
    AMONGOC_TRIVIALLY_RELOCATABLE_THIS(true, unique_box);
    using unique_box::unique::unique;

    template <typename T>
    T& as() noexcept {
        return amongoc_box_cast(T, get());
    }

    template <typename T>
    const T& as() const noexcept {
        return amongoc_box_cast(T, const_cast<amongoc_box&>(get()));
    }

    operator amongoc_view() const noexcept { return get().view; }

    template <typename T>
    T take() noexcept {
        T ret = (T&&)as<T>();
        amongoc_box_free_storage(((unique_box&&)*this).release());
        return ret;
    }

    void*       data() noexcept { return amongoc_box_data(get()); };
    const void* data() const noexcept { return amongoc_box_data(get()); };

    /**
     * @brief Construct a new box value by decay-copying the given value
     *
     * @param value The value to be decay-copied into a new box
     * @return amongoc_box The newly constructed box that contains the value
     */
    template <typename T>
        requires(not mlib::unique_deletable<T>)
    static unique_box from(mlib::allocator<> alloc, T&& value) {
        return make<std::decay_t<T>>(alloc, mlib_fwd(value));
    }

    /**
     * @brief Construct a new box that takes ownership of a type that is deletable
     * using the unique_deleter mechanism
     */
    template <mlib::unique_deletable T>
    static unique_box from(mlib::allocator<> a, T&& inst) {
        return from(a, mlib_fwd(inst), mlib::delete_unique);
    }

    // Prevent users from accidentally box-ing a box
    static void from(mlib::allocator<>, box&)               = delete;
    static void from(mlib::allocator<>, box&&)              = delete;
    static void from(mlib::allocator<>, box const&)         = delete;
    static void from(mlib::allocator<>, box const&&)        = delete;
    static void from(mlib::allocator<>, unique_box&)        = delete;
    static void from(mlib::allocator<>, unique_box&&)       = delete;
    static void from(mlib::allocator<>, unique_box const&)  = delete;
    static void from(mlib::allocator<>, unique_box const&&) = delete;

    template <typename T, typename D>
    static unique_box from(mlib::allocator<> a, mlib::unique<T, D> uniq) {
        return from(a, mlib_fwd(uniq).release(), D{});
    }

    /**
     * @brief Create a box that contains a `T` with an explicit destructor object type
     *
     * @tparam T The type to be boxed. MUST be trivially-destructible
     * @tparam D A stateless default-constructible destructor object
     * @param obj The object to be copied into the box
     */
    template <typename T, typename D>
    static unique_box from(mlib::allocator<> alloc, T obj, D) noexcept(box_inlinable_type<T>) {
        static_assert(std::is_trivially_destructible_v<T>,
                      "Creating a box with an explicit destructor requires that the object be "
                      "trivially destructible itself");
        static_assert(std::is_empty_v<D>, "The box destructor must be a stateless object type");
        // The destructor function that will be imbued in the box
        auto dtor = [](void* p) noexcept -> void {
            D d{};
            d(*static_cast<T*>(p));
        };
        amongoc_box ret;
        // Make storage
        T* ptr = ret.prepare_storage<T>(alloc, dtor);
        alloc.rebind<T>().construct(ptr, mlib_fwd(obj));
        return mlib_fwd(ret).as_unique();
    }

    /**
     * @brief Create a new box that contains a `T`, constructed from the given
     * arguments.
     *
     * @tparam T The type to be constructed into a box
     * @param args The constructor arguments
     */
    template <typename T, typename... Args>
    static unique_box make(mlib::allocator<> alloc, Args&&... args)
        noexcept(noexcept(T(mlib_fwd(args)...)) and box_inlinable_type<T>) {
        unique_box ret{amongoc_box{::amongoc_nil}};
        T*         ptr;
        // Conditionally add a destructor to the box based on whether the type
        // actually has a destructor.
        if constexpr (std::is_trivially_destructible_v<T>) {
            // No destructor needed. This gives more space within the box for the small-object
            // optimization
            ptr = ret.get().prepare_storage<T>(alloc, nullptr);
        } else {
            // Add a destructor function that just invokes the destructor on the object
            ptr = ret.get().prepare_storage<T>(alloc, indirect_destroy<T>);
        }
        // Placement-new the object into storage
        if constexpr (amongoc::box_inlinable_type<T> or noexcept(T(mlib_fwd(args)...))) {
            // No exception handling required: The constructor cannot throw OR we didn't allocate
            // any dynamic storage and there is nothing that would need to be freed
            alloc.rebind<T>().construct(ptr, mlib_fwd(args)...);
        } else {
            // We will need to free the storage if the constructor throws
            try {
                alloc.rebind<T>().construct(ptr, mlib_fwd(args)...);
            } catch (...) {
                amongoc_box_free_storage(mlib_fwd(ret).release());
                throw;
            }
        }
        return ret;
    }

    /**
     * @internal
     * @brief An internal API to visit a compressed box. See `box.compress.hpp` for information
     *
     * @tparam Sz Candidate compression sizes
     * @param fn The visitor function
     * @return auto Returns the result of invoking the visitor
     */
    template <std::size_t... Sz, typename F>
    decltype(auto) compress(F&& fn) &&;

private:
    template <typename T>
    static void indirect_destroy(void* p) noexcept {
        static_cast<T*>(p)->~T();
    }
};

/**
 * @brief Obtain a new nil `unique_box`
 *
 * @return unique_box
 */
mlib_always_inline unique_box nil() noexcept { return unique_box{amongoc_box{}}; }

}  // namespace amongoc

template <typename T>
T& amongoc_view::as() const noexcept {
    return amongoc_box_cast(T, (amongoc_view&)*this);
}

template <typename T>
T* amongoc_box::prepare_storage(mlib::allocator<> alloc, amongoc_box_destructor dtor) {
    T* p;
    if constexpr (amongoc::enable_trivially_relocatable_v<T>) {
        // The box can be safely stored inline, since moving the box will relocate the corresponding
        // object
        p = amongoc_box_init(*this, T, dtor, alloc.c_allocator());
    } else {
        // Box type cannot be safely stored inline, so dynamically allocate it:
        p = amongoc_box_init_noinline(*this, T, dtor, alloc.c_allocator());
    }
    if (p == nullptr) {
        throw std::bad_alloc();
    }
    return p;
}

amongoc::unique_box amongoc_box::as_unique() && noexcept {
    return amongoc::unique_box((amongoc_box&&)(*this));
}
