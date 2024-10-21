#pragma once

#include <bson/detail/assert.h>
#include <bson/detail/mem.h>
#include <bson/iterator.h>
#include <bson/value_ref.h>
#include <bson/view.h>

#include <mlib/alloc.h>
#include <mlib/config.h>

/**
 * @brief A mutable BSON document.
 *
 * This type is trivially relocatable.
 */
typedef struct bson_doc {
    /**
     * @brief Points to the beginning of the document data.
     */
    bson_byte* _bson_document_data;
    /**
     * @brief The allocator used with this document
     */
    struct mlib_allocator _allocator;

#if mlib_is_cxx()
    ::bson_iterator begin() const noexcept { return bson_begin(*this); }
    ::bson_iterator end() const noexcept { return bson_end(*this); }

    operator bson_view() const noexcept {
        bson_view r;
        r._bson_document_data = this->_bson_document_data;
        return r;
    }
#endif  // C++
} bson_doc;

mlib_extern_c_begin();

// Define a global single byte array to represent the empty document, which has a
// known fixed form. This byte array will be used for empty-initialized document
// objects to prevent immediate allocation.
inline const bson_byte* _bson_get_global_empty_doc_data(void) mlib_noexcept {
    // A statically allocated bson_doc backing buffer. The first four bytes
    // are a uint32_t{5} to represent the statically allocated capacity
    static const bson_byte data[9] = {{5}, {0}, {0}, {0}, {5}, {0}, {0}, {0}, {0}};
    return data + 4;
}

/**
 * @internal
 * @brief Obtain a pointer to the memory buffer owned by `d`.
 *
 * This is distinct from the buffer that contains the actual document data. We
 * store a cookie that holds the allocated capacity in the first four bytes. The
 * remainder of the buffer is the actual document data.
 *
 * To get the pointer to the BSON data, use `bson_data`/`bson_mut_data` instead.
 *
 * @param d
 * @return mlib_constexpr*
 */
mlib_constexpr bson_byte* _bson_doc_buffer_ptr(bson_doc d) mlib_noexcept {
    return d._bson_document_data - 4;
}

// Obtain the byte capacity of the document managed by `d`
mlib_constexpr uint32_t bson_doc_capacity(bson_doc d) mlib_noexcept {
    bson_byte* p = _bson_doc_buffer_ptr(d);
    return _bson_read_u32le(p);
}

/**
 * @brief Obtain the `mlib_allocator` used with the given BSON mutator object
 */
mlib_constexpr mlib_allocator bson_doc_get_allocator(bson_doc m) mlib_noexcept {
    return m._allocator;
}

inline bool _bson_realloc(bson_doc* doc, uint32_t new_size) mlib_noexcept {
    const uint32_t cookie_size = sizeof(uint32_t);
    // Cap allocation size:
    if (new_size > (INT32_MAX - cookie_size)) {
        return false;
    }
    // Get the allocator:
    mlib_allocator alloc = bson_doc_get_allocator(*doc);
    // Perform the reallocation:
    bson_byte* prev_buf;
    uint32_t   prev_alloc_size;
    if (bson_mut_data(*doc) == _bson_get_global_empty_doc_data()) {
        // We're pointing to the global empty constant. Don't attempt to reallocate this.
        prev_buf        = NULL;
        prev_alloc_size = 0;
    } else {
        prev_buf        = _bson_doc_buffer_ptr(*doc);
        prev_alloc_size = bson_doc_capacity(*doc) + cookie_size;
    }
    // We want enough bytes for the document, plus more for our capacity cookie
    const size_t want_size = new_size + cookie_size;
    size_t       got_size  = 0;
    bson_byte*   new_buf
        = (bson_byte*)mlib_reallocate(alloc, prev_buf, want_size, 1, prev_alloc_size, &got_size);
    if (!new_buf) {
        // The allocator reports failure
        return false;
    }
    // Assert that the allocator actually gave us the space we requested:
    BV_ASSERT(got_size >= want_size);
    BV_ASSERT(got_size <= INT32_MAX);
    // Save the new pointer, adjusted past our cookie
    doc->_bson_document_data = new_buf + cookie_size;
    // Remember the buffer size that the allocator gave us:
    _bson_write_u32le(new_buf, got_size - cookie_size);
    if (prev_buf == NULL) {
        // We reallocated a region after using the empty doc. Initialize it to an empty doc
        memcpy(bson_mut_data(*doc), _bson_get_global_empty_doc_data(), 5);
    }
    return true;
}

/**
 * @brief Adjust the capacity of the given bson_doc
 *
 * @param d The document to update
 * @param size The requested capacity
 * @return int32_t Returns the new capacity upon success, otherwise a negative
 * value
 *
 * @note If `size` is less than our current capacity, the capacity will remain
 * unchanged.
 *
 * All outstanding iterators and pointers are invalidated if the requested size
 * is greater than our current capacity.
 */
inline int32_t bson_doc_reserve(bson_doc* d, uint32_t size) mlib_noexcept {
    if (bson_doc_capacity(*d) >= size) {
        // We already have enough room
        return (int32_t)bson_doc_capacity(*d);
    }
    if (!_bson_realloc(d, size)) {
        // Allocation failed
        return -1;
    }
    return (int32_t)bson_doc_capacity(*d);
}

/**
 * @brief A BSON document mutator
 *
 * This type is trivial.
 */
typedef struct bson_mut {
    /**
     * @brief Points to the beginning of the document data.
     *
     * @internal
     *
     * In CHILD MODE, this is a non-owning pointer. It should not be freed or
     * reallocated.
     *
     * In ROOT MODE, this is an owning pointer and can be freed or reallocated.
     */
    bson_byte* _bson_document_data;
    union {
        /**
         * @internal
         * @brief In CHILD MODE, this points to the bson_mut that manages the parent document
         */
        struct bson_mut* _parent_mut;
        /**
         * @internal
         * @brief In ROOT MODE, this is the allocator for the document data.
         */
        struct bson_doc* _doc;
    };
    /**
     * @internal
     * @brief The offset of this mutator within the parent document (CHILD MODE).
     * If zero, then this mutator is manipulating the parent document directly (ROOT MODE)
     *
     * In CHILD MODE, this is the the byte-offset of the document /element/
     * within the parent document. This is used to quickly recover a
     * bson_iterator that refers to the document element within the
     * parent, since iterators point to the element data. This is also necessary
     * to quickly compute the element key length without a call to strlen(),
     * since we can compute the key length based on the difference between the
     * element's address and the address of the document data within the element.
     */
    uint32_t _offset_within_parent_data;

#if mlib_is_cxx()
    friend ::bson_iterator begin(const bson_mut& m) noexcept { return bson_begin(m); }
    friend ::bson_iterator end(const bson_mut& m) noexcept { return bson_end(m); }

    operator bson_view() const noexcept {
        bson_view r;
        r._bson_document_data = this->_bson_document_data;
        return r;
    }
#endif  // C++
} bson_mut;

#define BSON_MUT_v2_NULL (mlib_init(bson_mut){NULL, {NULL}, 0})

/**
 * @brief Create a new BSON document or copy an existing one.
 *
 * Callable with several signatures:
 *
 * - `bson_new()`
 * - `bson_new(uint32_t reserve)`
 * - `bson_new(uint32_t reserve, mlib_allocator)`
 * - `bson_new(mlib_allocator)`
 * - `bson_new(bson_doc)`
 * - `bson_new(bson-viewable)`
 * - `bson_new(bson-viewable, mlib_allocator)`
 */
#define bson_new(...) _bsonNew(__VA_ARGS__)
#if mlib_is_cxx()
#define _bsonNew(...) _bson_new_generic(__VA_ARGS__)
#else
#define _bsonNew(...) MLIB_PASTE(_bsonNewArgc_, MLIB_ARG_COUNT(__VA_ARGS__))(__VA_ARGS__)
#endif
#define _bsonNewArgc_0() _bson_new(5, mlib_default_allocator)
#define _bsonNewArgc_1(X)                                                                          \
    _Generic(X,                                                                                    \
        mlib_allocator: _bson_new_with_alloc,                                                      \
        bson_doc: _bson_copy_doc,                                                                  \
        bson_view: _bson_copy_view_with_default_allocator,                                         \
        struct bson_mut: _bson_copy_mut_with_default_allocator,                                    \
        default: _bson_new_reserve_with_default_alloc)(X)
#define _bsonNewArgc_2(ReserveOrDoc, Alloc)                                                        \
    _Generic((ReserveOrDoc),                                                                       \
        bson_doc: _bson_copy_doc_with_allocator,                                                   \
        bson_view: _bson_copy_view_with_allocator,                                                 \
        struct bson_mut: _bson_copy_mut_with_allocator,                                            \
        default: _bson_new)((ReserveOrDoc), (Alloc))

/**
 * @internal
 * @brief Base implementation of creating a new document with a reservation
 *
 * [[3]]
 */
inline bson_doc _bson_new(uint32_t reserve, mlib_allocator allocator) mlib_noexcept {
    bson_doc ret;
    ret._allocator = allocator;
    // We're starting out empty. We can use the global empty document for now.
    // This cast is safe to remove `const`, because no well-formed program will
    // attempt to modify the bytes of the empty document.
    ret._bson_document_data = (bson_byte*)_bson_get_global_empty_doc_data();
    if (reserve > 5) {
        // Create the initial storage:
        if (!bson_doc_reserve(&ret, reserve)) {
            return ret;
        }
        // Set an initial empty document:
        memset(bson_mut_data(ret), 0, bson_doc_capacity(ret));
        ret._bson_document_data[0].v = 5;
    }
    return ret;
}

/**
 * @internal
 * @brief Base implementation of copying an existing document with an allocator
 *
 * [[7]]
 */
inline bson_doc _bson_copy_view_with_allocator(bson_view      other,
                                               mlib_allocator alloc) mlib_noexcept {
    bson_doc ret = _bson_new(bson_size(other), alloc);
    // If we copied an empty doc, then we did not allocate memory, and we point to
    // the global empty instance. We should not attempt to write anything there.
    // It is already statically initialized.
    if (ret._bson_document_data != _bson_get_global_empty_doc_data()) {
        memcpy(bson_mut_data(ret), bson_data(other), bson_size(other));
    }
    return ret;
}
inline bson_doc _bson_copy_mut_with_allocator(struct bson_mut other,
                                              mlib_allocator  alloc) mlib_noexcept {
    bson_doc ret = _bson_new(bson_size(other), alloc);
    // If we copied an empty doc, then we did not allocate memory, and we point to
    // the global empty instance. We should not attempt to write anything there.
    // It is already statically initialized.
    if (ret._bson_document_data != _bson_get_global_empty_doc_data()) {
        memcpy(bson_mut_data(ret), bson_data(other), bson_size(other));
    }
    return ret;
}
inline bson_doc _bson_copy_doc_with_allocator(bson_doc other, mlib_allocator alloc) mlib_noexcept {
    return _bson_copy_view_with_allocator(bson_as_view(other), alloc);
}

// [[2]]
inline bson_doc _bson_new_reserve_with_default_alloc(uint32_t n) mlib_noexcept {
    return _bson_new(n, mlib_default_allocator);
}
// [[4]]
inline bson_doc _bson_new_with_alloc(mlib_allocator a) mlib_noexcept { return _bson_new(5, a); }

// [[6]]
inline bson_doc _bson_copy_view_with_default_allocator(bson_view view) mlib_noexcept {
    return _bson_copy_view_with_allocator(view, mlib_default_allocator);
}

inline bson_doc _bson_copy_mut_with_default_allocator(struct bson_mut other) mlib_noexcept {
    return _bson_copy_mut_with_allocator(other, mlib_default_allocator);
}

// [[6]]
inline bson_doc _bson_copy_doc(bson_doc other) mlib_noexcept {
    return _bson_copy_view_with_allocator(bson_as_view(other), bson_doc_get_allocator(other));
}

mlib_extern_c_end();

#if mlib_is_cxx()
// Base reserve impl
inline bson_doc _bson_new_generic(std::uint32_t reserve, ::mlib_allocator alloc) noexcept {
    return ::_bson_new(reserve, alloc);
}
// Base copy impl
inline bson_doc _bson_new_generic(bson_view doc, ::mlib_allocator alloc) noexcept {
    return ::_bson_copy_view_with_allocator(doc, alloc);
}
inline bson_doc _bson_new_generic(bson_doc doc, ::mlib_allocator alloc) noexcept {
    return ::_bson_copy_view_with_allocator(bson_as_view(doc), alloc);
}
inline bson_doc _bson_new_generic(struct bson_mut doc, ::mlib_allocator alloc) noexcept {
    return ::_bson_copy_view_with_allocator(bson_as_view(doc), alloc);
}

inline bson_doc _bson_new_generic(::mlib_allocator alloc) noexcept { return ::bson_new(5, alloc); }
inline bson_doc _bson_new_generic(std::uint32_t reserve) noexcept {
    return ::bson_new(reserve, ::mlib_default_allocator);
}
inline bson_doc _bson_new_generic() noexcept { return ::bson_new(5, ::mlib_default_allocator); }
inline bson_doc _bson_new_generic(bson_doc doc) noexcept {
    return ::bson_new(bson_as_view(doc), bson_doc_get_allocator(doc));
}
inline bson_doc _bson_new_generic(bson_view doc) noexcept {
    return ::bson_new(bson_as_view(doc), ::mlib_default_allocator);
}
inline bson_doc _bson_new_generic(bson_mut doc) noexcept {
    return ::bson_new(bson_as_view(doc), ::mlib_default_allocator);
}
#endif  // C++

mlib_extern_c_begin();

/**
 * @brief Free the resources of the given BSON document
 */
mlib_constexpr void bson_delete(bson_doc d) mlib_noexcept {
    if (d._bson_document_data == NULL
        || d._bson_document_data == _bson_get_global_empty_doc_data()) {
        // Object is null or statically allocated
        return;
    }
    mlib_reallocate(d._allocator, _bson_doc_buffer_ptr(d), 0, 1, bson_doc_capacity(d) + 4, NULL);
}

mlib_extern_c_end();

#define T bson_doc
#define VecDestroyElement bson_delete
#include <mlib/vec.t.h>

#if mlib_is_cxx()

namespace bson {

/**
 * @brief Presents a mutable BSON document STL-like interface
 *
 * This type provides insert/emplace/push_back/emplace_back APIs based on those
 * found on std::map.
 *
 * The type *does not* provide a `size()`, because such a method cannot be implemented
 * as a constant-time algorithm. To get the number of elements, use `std::ranges::distance`
 */
class document {
public:
    using allocator_type = mlib::allocator<bson_byte>;

    using iterator = bson_iterator;

    using enable_trivially_relocatable = document;

#if !mlib_audit_allocator_passing()
    document()
        : document(allocator_type(mlib_default_allocator)) {}
    explicit document(bson_view v)
        : document(v, allocator_type(mlib_default_allocator)) {}
#endif

    ~document() { _del(); }

    /**
     * @brief Construct a document object using the given allocator
     */
    explicit document(allocator_type alloc)
        : document(alloc, 5) {}

    /**
     * @brief Construct a new document with the given allocator, and reserving
     * the given capacity in the new document.
     */
    explicit document(allocator_type alloc, std::size_t reserve_size) {
        _doc = bson_new(static_cast<std::uint32_t>(reserve_size), alloc.c_allocator());
        if (data() == nullptr) {
            throw std::bad_alloc();
        }
    }

    /**
     * @brief Take ownership of a C-style `::bson_doc` object
     */
    explicit document(bson_doc&& o) noexcept {
        _doc = o;
        o    = bson_doc{};
    }

    /**
     * @brief Copy from the given BSON view
     *
     * @param v The document to be copied
     * @param alloc An allocator for the operation
     */
    explicit document(bson_view v, allocator_type alloc)
        : _doc(bson_new(v, alloc.c_allocator())) {
        if (data() == nullptr)
            throw std::bad_alloc();
    }

    document(document const& other) { _doc = bson_new(other._doc); }
    document(document&& other) noexcept
        : _doc(((document&&)other).release()) {}

    document& operator=(const document& other) {
        _del();
        _doc = bson_new(other._doc);
        if (data() == nullptr) {
            throw std::bad_alloc();
        }
        return *this;
    }

    document& operator=(document&& other) noexcept {
        _del();
        _doc = ((document&&)other).release();
        return *this;
    }

    iterator begin() const noexcept { return bson_begin(_doc); }
    iterator end() const noexcept { return bson_end(_doc); }

    iterator find(auto&& key) const noexcept
        requires requires(view v) { v.find(key); }
    {
        return view(*this).find(key);
    }

    bson_byte*       data() noexcept { return bson_mut_data(_doc); }
    const bson_byte* data() const noexcept { return bson_data(_doc); }
    std::size_t      byte_size() const noexcept { return bson_size(_doc); }

    bool empty() const noexcept { return byte_size() == 5; }

    void reserve(std::size_t n) {
        if (::bson_doc_reserve(&_doc, static_cast<std::uint32_t>(n)) < 0) {
            throw std::bad_alloc();
        }
    }

    operator bson_view() const noexcept { return bson_view::from_data(data(), byte_size()); }
    operator bson_value_ref() const noexcept { return bson_value_ref::from(bson_view(*this)); }

    /**
     * @brief Prepare the internal buffer to be overwritten via invoking the given
     * operation
     *
     * @param len The number of bytes to reserve for the operation
     * @param oper The operation that will fill the document. Must be invocable
     * with a `bson_byte*` pointing to the beginning of the reserved buffer. If
     * the operation throws, then `*this` will be left in a valid-but-unspecified
     * state.
     */
    template <class Operation>
    void resize_and_overwrite(std::size_t len, Operation oper) {
        BV_ASSERT(len >= 5);
        reserve(len);
        try {
            oper(data());
        } catch (...) {
            _del();
            throw;
        }
    }

    // Obtain a reference to the wrapped C API struct
    bson_doc&       get() noexcept { return _doc; }
    const bson_doc& get() const noexcept { return _doc; }

    bson_doc release() && noexcept {
        auto m                   = _doc;
        _doc._bson_document_data = nullptr;
        return m;
    }

    allocator_type get_allocator() const noexcept {
        return allocator_type(::bson_doc_get_allocator(_doc));
    }

private:
    void _del() noexcept {
        ::bson_delete(_doc);
        _doc._bson_document_data = nullptr;
    }
    ::bson_doc _doc;
};

}  // namespace bson
#endif  // C++
