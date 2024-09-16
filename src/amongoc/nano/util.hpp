#pragma once

#include "./query.hpp"

#include <amongoc/box.h>

#include <neo/attrib.hpp>
#include <neo/invoke.hpp>
#include <neo/like.hpp>
#include <neo/object_box.hpp>
#include <neo/object_t.hpp>
#include <neo/type_traits.hpp>

namespace amongoc {

#define AMONGOC_RETURNS(...)                                                                       \
    noexcept(noexcept(__VA_ARGS__))                                                                \
        ->decltype(__VA_ARGS__)                                                                    \
        requires requires { (__VA_ARGS__); }                                                       \
    {                                                                                              \
        return __VA_ARGS__;                                                                        \
    }                                                                                              \
    static_assert(true)

template <typename F>
struct deferred_conversion {
    neo::object_t<F> _func;

    constexpr operator neo::invoke_result_t<F>() { return NEO_INVOKE(static_cast<F&&>(_func)); }

    constexpr operator neo::invoke_result_t<const F>() const {
        return NEO_INVOKE(static_cast<F&&>(_func));
    }
};

/**
 * @brief Create a "deferred conversion" object from an invocable.
 *
 * The returned conversion object is explicitly convertible to the return type
 * of the invocable. Attempting the conversion will invoke the function to
 * generate the result value. This allows in-place construction for emplacement
 * functions on containers and wrappers.
 */
template <typename F>
constexpr deferred_conversion<F> defer_convert(F&& fn) {
    return deferred_conversion<F>{mlib_fwd(fn)};
}

// Enclose a partially-applied invocable object so that it may be used as the operand to operator|
template <typename F, typename... Args>
struct [[nodiscard]] closure {
    NEO_NO_UNIQUE_ADDRESS neo::object_box<F> _function;
    NEO_NO_UNIQUE_ADDRESS std::tuple<Args...> _args;

    template <typename Arg, std::size_t... Ns>
    constexpr static auto apply(auto&& self, Arg&& arg, std::index_sequence<Ns...>)
        AMONGOC_RETURNS(std::invoke(mlib_fwd(self)._function.get(),
                                    mlib_fwd(arg),
                                    std::get<Ns>(mlib_fwd(self)._args)...));

    // Handle the closure object appear on the right-hand of a vertical pipe expression
    template <typename Left, neo::alike<closure> Self>
        requires neo::invocable2<F, Left, Args...>
    friend constexpr auto operator|(Left&& lhs, Self&& rhs) AMONGOC_RETURNS(
        closure::apply(mlib_fwd(rhs), mlib_fwd(lhs), std::make_index_sequence<sizeof...(Args)>{}));
};

template <typename T, typename... Args>
constexpr closure<T, Args...> make_closure(T&& func, Args&&... args) noexcept {
    return closure<T, Args...>{mlib_fwd(func), std::forward_as_tuple(mlib_fwd(args)...)};
}

template <std::size_t N>
using size_constant = std::integral_constant<std::size_t, N>;

/**
 * @brief Helper mixin for providing cvref-qualified overloads of `operator()`
 *
 * Calls a static method `invoke` with the perfect-forwarded object as the first
 * argument.
 *
 * @tparam D The derived type that implements invocable
 *
 * @note This can be removed when we can use C++ explicit object parameters
 */
template <typename D>
class invocable_cvr_helper {
    D&       _self() noexcept { return static_cast<D&>(*this); }
    const D& _self() const noexcept { return static_cast<const D&>(*this); }

public:
    template <typename... Args>
    constexpr decltype(auto) operator()(Args&&... x) &
        requires requires { D::invoke(_self(), mlib_fwd(x)...); }
    {
        return D::invoke(_self(), mlib_fwd(x)...);
    };
    template <typename... Args>
    constexpr decltype(auto) operator()(Args&&... x) const&
        requires requires { D::invoke(_self(), mlib_fwd(x)...); }
    {
        return D::invoke(_self(), mlib_fwd(x)...);
    };

    template <typename... Args>
    constexpr decltype(auto) operator()(Args&&... x) &&
        requires requires { D::invoke(std::move(_self()), mlib_fwd(x)...); }
    {
        return D::invoke(std::move(_self()), mlib_fwd(x)...);
    };
    template <typename... Args>
    constexpr decltype(auto) operator()(Args&&... x) const&&
        requires requires { D::invoke(std::move(_self()), mlib_fwd(x)...); }
    {
        return D::invoke(std::move(_self()), mlib_fwd(x)...);
    };
};

/**
 * @brief Create a composed function from two functions (f ∘ g)
 *
 * The expression `atop(f, g)(x)` is equivalent to `f(g(x))`
 *
 * The `atop` object forwards `query()` calls to the `f` function.
 */
template <typename F, typename G>
class atop : public invocable_cvr_helper<atop<F, G>> {
public:
    atop() = default;
    constexpr explicit atop(F&& f, G&& g)
        : _f(mlib_fwd(f))
        , _g(mlib_fwd(g)) {}

    AMONGOC_TRIVIALLY_RELOCATABLE_THIS(
        enable_trivially_relocatable_v<F>and enable_trivially_relocatable_v<G>);

private:
    NEO_NO_UNIQUE_ADDRESS neo::object_box<F> _f;
    NEO_NO_UNIQUE_ADDRESS neo::object_box<G> _g;

public:
    template <typename... Ts>
    static constexpr auto invoke(auto&& self, Ts&&... args)
        AMONGOC_RETURNS(NEO_INVOKE(mlib_fwd(self)._f.get(),
                                   NEO_INVOKE(mlib_fwd(self)._g.get(), mlib_fwd(args)...)));

    template <valid_query_for<F> Q>
    constexpr query_t<Q, F> query(Q q) const {
        return q(_f.get());
    }
};

template <typename F, typename G>
explicit atop(F&&, G&&) -> atop<F, G>;

/**
 * @brief Like `atop`, but passes multiple arguments to `g` separately
 *
 * `over(f, g)(x, y, z)` is equivalent to `f(g(x), g(y), g(z))`
 */
template <typename F, typename G>
class over : public invocable_cvr_helper<over<F, G>> {
public:
    constexpr explicit over(F&& f, G&& g)
        : _f(mlib_fwd(f))
        , _g(mlib_fwd(g)) {}

    AMONGOC_TRIVIALLY_RELOCATABLE_THIS(
        enable_trivially_relocatable_v<F>and enable_trivially_relocatable_v<G>);

private:
    NEO_NO_UNIQUE_ADDRESS neo::object_box<F> _f;
    NEO_NO_UNIQUE_ADDRESS neo::object_box<G> _g;

public:
    template <typename... Ts>
    static constexpr auto invoke(auto&& self, Ts&&... args)
        AMONGOC_RETURNS(NEO_INVOKE(mlib_fwd(self)._f.get(),
                                   NEO_INVOKE(mlib_fwd(self)._g.get(), mlib_fwd(args))...));

    template <valid_query_for<F> Q>
    constexpr query_t<Q, F> query(Q q) const {
        return q(_f.get());
    }
};

template <typename F, typename G>
explicit over(F&&, G&&) -> over<F, G>;

/**
 * @brief An invocable object that always returns the given value regardless
 * of arguments. This is the K-combinator
 *
 * @tparam T The type of object stored and returned by the invocable
 */
template <typename T>
class constant {
public:
    constexpr explicit constant(T&& t)
        : _value(mlib_fwd(t)) {}

    AMONGOC_TRIVIALLY_RELOCATABLE_THIS(enable_trivially_relocatable_v<T>);

private:
    NEO_NO_UNIQUE_ADDRESS neo::object_t<T> _value;

public:
    constexpr T&       operator()(auto&&...) & noexcept { return static_cast<T&>(_value); }
    constexpr T&&      operator()(auto&&...) && noexcept { return static_cast<T&&>(_value); }
    constexpr const T& operator()(auto&&...) const& noexcept {
        return static_cast<const T&>(_value);
    }
    constexpr const T&& operator()(auto&&...) const&& noexcept {
        return static_cast<const T&&>(_value);
    }
};

template <typename T>
explicit constant(T&&) -> constant<T>;


/**
 * @brief Create an invocable that will pair-up the given argument as the second
 * element of pair.
 *
 * @tparam T The object to be paired. Will be forwarded through the pair (preserves references)
 *
 * Invocable signature of pair_append<T>: (U) -> pair<U, T>
 */
template <typename T>
class pair_append : public invocable_cvr_helper<pair_append<T>> {
public:
    AMONGOC_TRIVIALLY_RELOCATABLE_THIS(enable_trivially_relocatable_v<T>);

    constexpr explicit pair_append(T&& t)
        : _object(mlib_fwd(t)) {}

    template <typename Arg>
    static constexpr std::pair<Arg, T> invoke(auto&& self, Arg&& arg) noexcept {
        return std::pair<Arg, T>(mlib_fwd(arg), mlib_fwd(self)._object);
    }

private:
    T _object;
};

template <typename T>
explicit pair_append(T&&) -> pair_append<T>;

/**
 * @brief Combinator that unpacks a tuple-like argument before invoking the underlying invocable
 *
 * @tparam F The wrapped invocable
 *
 * When this object is invoked with an N-tuple argument, the underlying invocable
 * will be called with N arguments as-if by std::apply()
 */
template <typename F>
class unpack_args : public invocable_cvr_helper<unpack_args<F>> {
public:
    unpack_args() = default;

    explicit constexpr unpack_args(F&& fn)
        : _func(mlib_fwd(fn)) {}

    AMONGOC_TRIVIALLY_RELOCATABLE_THIS(enable_trivially_relocatable_v<F>);

private:
    [[no_unique_address]] F _func;

public:
    template <typename Tpl>
    static constexpr decltype(auto) invoke(auto&& self, Tpl&& tpl)
        requires requires { std::apply(mlib_fwd(self)._func, mlib_fwd(tpl)); }
    {
        return std::apply(mlib_fwd(self)._func, mlib_fwd(tpl));
    }
};

template <typename F>
explicit unpack_args(F&&) -> unpack_args<F>;

/**
 * @brief Variable template for an invocable that calls the constructor of an object
 *
 * @tparam T The type to be constructed.
 *
 * Use this to create an invocable from an arbitrary type, treating it's constructor
 * overload set as an invocable object
 */
template <typename T>
constexpr auto construct = []<typename... Args>(Args&&... args) -> T
    requires std::constructible_from<T, Args...>
{ return T(NEO_FWD(args)...); };

template <typename T>
constexpr std::size_t effective_sizeof_v = std::is_empty_v<T> ? 0 : sizeof(T);

}  // namespace amongoc
