#pragma once

#include "./query.hpp"

#include <amongoc/box.h>

#include <mlib/object_t.hpp>

#include <neo/attrib.hpp>
#include <neo/invoke.hpp>
#include <neo/like.hpp>
#include <neo/type_traits.hpp>

#include <concepts>
#include <functional>
#include <numeric>
#include <ranges>
#include <type_traits>

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
    mlib::object_t<F> _func;

    constexpr operator std::invoke_result_t<F>() { return std::invoke(static_cast<F&&>(_func)); }

    constexpr operator std::invoke_result_t<const F>() const {
        return std::invoke(static_cast<F&&>(_func));
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
    NEO_NO_UNIQUE_ADDRESS mlib::object_t<F> _function;
    NEO_NO_UNIQUE_ADDRESS std::tuple<Args...> _args;

    template <typename Arg, std::size_t... Ns>
    constexpr static auto apply(auto&& self, Arg&& arg, std::index_sequence<Ns...>)
        AMONGOC_RETURNS(std::invoke(mlib_fwd(self)._function,
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
 * The expression `atop(f, g)(xs...)` is equivalent to `f(g(xs...))`
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
    NEO_NO_UNIQUE_ADDRESS mlib::object_t<F> _f;
    NEO_NO_UNIQUE_ADDRESS mlib::object_t<G> _g;

public:
    template <typename... Ts>
    static constexpr auto invoke(auto&& self, Ts&&... args)
        AMONGOC_RETURNS(NEO_INVOKE(mlib_fwd(self)._f,
                                   NEO_INVOKE(mlib_fwd(self)._g, mlib_fwd(args)...)));

    template <valid_query_for<F> Q>
    constexpr query_t<Q, F> query(Q q) const {
        return q(static_cast<const F&>(_f));
    }
};

template <typename F, typename G>
explicit atop(F&&, G&&) -> atop<F, G>;

/**
 * @brief Like `atop`, but passes each argument to `g` separately
 *
 * `over(f, g)(x, y, z)` is equivalent to `f(g(x), g(y), g(z))`
 *
 * The `over` object forwards `query()` calls to the `f` function.
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
    NEO_NO_UNIQUE_ADDRESS mlib::object_t<F> _f;
    NEO_NO_UNIQUE_ADDRESS mlib::object_t<G> _g;

public:
    template <typename... Ts>
    static constexpr auto invoke(auto&& self, Ts&&... args)
        AMONGOC_RETURNS(NEO_INVOKE(mlib_fwd(self)._f,
                                   NEO_INVOKE(mlib_fwd(self)._g, mlib_fwd(args))...));

    template <valid_query_for<F> Q>
    constexpr query_t<Q, F> query(Q q) const {
        return q(static_cast<const F&>(_f));
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
    NEO_NO_UNIQUE_ADDRESS mlib::object_t<T> _value;

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
 * @brief Like `constant`, but returns a compile-time constant value given as a template parameter
 *
 * @tparam Value The value that will be returned from the constant function
 */
template <auto Value>
struct ct_constant {
    constexpr auto operator()(auto&&...) const noexcept { return Value; }
};

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
    [[no_unique_address]] T _object;
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
    static constexpr auto invoke(auto&& self, auto&& tpl)
        AMONGOC_RETURNS(std::apply(mlib_fwd(self)._func, mlib_fwd(tpl)));
};

template <typename F>
explicit unpack_args(F&&) -> unpack_args<F>;

/**
 * @brief Implement the S-combinator
 *
 * - `after(f, g)(x)` → `f(x, g(x))`
 *
 * @tparam F The binary function to be applied after `G`
 * @tparam G The unary function to be applied before `F`
 */
template <typename F, typename G>
struct after : invocable_cvr_helper<after<F, G>> {
public:
    after() = default;
    explicit constexpr after(F&& f, G&& g)
        : f(mlib_fwd(f))
        , g(mlib_fwd(g)) {}

    AMONGOC_TRIVIALLY_RELOCATABLE_THIS(
        enable_trivially_relocatable_v<F>and enable_trivially_relocatable_v<G>);

private:
    [[no_unique_address]] F f;
    [[no_unique_address]] G g;

public:
    static constexpr auto invoke(auto&& self, auto&& x)
        AMONGOC_RETURNS(mlib_fwd(self).f(x, mlib_fwd(self).g(x)));
};

template <typename F, typename G>
explicit after(F&&, G&&) -> after<F, G>;

/**
 * @brief Φ-Combinator:
 *
 * - `phi(h, f, g)(xs...)` → `h(f(xs...), g(xs...))`
 *
 * @tparam H A binary function
 * @tparam F The left-hand function
 * @tparam G The right-hand function
 */
template <typename H, typename F, typename G>
struct phi : invocable_cvr_helper<phi<H, F, G>> {
public:
    phi() = default;
    constexpr explicit phi(H&& h, F&& f, G&& g)
        : h(mlib_fwd(h))
        , f(mlib_fwd(f))
        , g(mlib_fwd(g)) {}

private:
    [[no_unique_address]] H h;
    [[no_unique_address]] F f;
    [[no_unique_address]] G g;

public:
    static constexpr auto invoke(auto&& self, auto&&... args)
        AMONGOC_RETURNS(mlib_fwd(self).h(mlib_fwd(self).f(args...),  //
                                         mlib_fwd(self).g(args...)));
};

template <typename H, typename F, typename G>
explicit phi(H&&, F&&, G&&) -> phi<H, F, G>;

// Return `true` if any boolean in the given range is true
constexpr auto any = []<std::ranges::input_range R>(R&& rng) -> bool
    requires std::convertible_to<std::ranges::range_value_t<R>, bool>
{
    for (bool b : rng) {
        if (b) {
            return true;
        }
    }
    return false;
};

/**
 * @brief Convert an ASCII char from an uppercase letter to a lowercase letter.
 *
 * This is different from `std::tolower` in that it does not consult a locale
 */
constexpr auto ascii_tolower = [](char c) noexcept -> char {
    if (c >= 'A' and c <= 'Z') {
        c += ('a' - 'A');
    }
    return c;
};

/**
 * @brief Invocable that calls the constructor for an object
 *
 * @tparam T The type to be constructed.
 *
 * Use this to create an invocable from an arbitrary type, treating it's constructor
 * overload set as an invocable object
 */
template <typename T>
struct construct {
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    T operator()(Args&&... args) const noexcept(std::is_nothrow_constructible_v<T, Args...>) {
        return T(static_cast<Args&&>(args)...);
    }
};

template <typename T>
constexpr std::size_t effective_sizeof_v = std::is_empty_v<T> ? 0 : sizeof(T);

/**
 * @brief An invocable object that assigns its parameter into a bound object
 *
 * - `assign(x)(y)` → `x = y`
 *
 * @tparam T The object that is the target of the assignment
 */
template <typename T>
class assign {
public:
    assign() = default;
    constexpr explicit assign(T&& x)
        : _dest(mlib_fwd(x)) {}

    template <typename U>
        requires std::assignable_from<T&, U>
    constexpr T& operator()(U&& arg) noexcept {
        _dest = mlib_fwd(arg);
        return _dest;
    }

    template <typename U>
        requires std::assignable_from<const T&, U>
    constexpr const T& operator()(U&& arg) const noexcept {
        _dest = mlib_fwd(arg);
        return _dest;
    }

private:
    [[no_unique_address]] T _dest;
};

template <typename T>
explicit assign(T&&) -> assign<T>;

/**
 * @brief Convert a function `T -> U` to a function `optional<T> -> optional<U>`
 */
template <typename F>
struct opt_fmap {
    [[no_unique_address]] F _func;

    AMONGOC_TRIVIALLY_RELOCATABLE_THIS(
        enable_trivially_relocatable_v<F>and enable_trivially_relocatable_v<F>);

    template <typename Opt,
              typename Result = decltype(std::declval<const F&>()(*std::declval<Opt>()))>
    constexpr std::optional<Result> operator()(Opt&& opt) const {
        std::optional<Result> ret;
        if (opt) {
            ret.emplace(_func(*mlib_fwd(opt)));
        }
        return ret;
    }
};

/**
 * @brief Decay-copy an object.
 *
 * This can be replaced with `auto()` when C++23 is available
 */
constexpr auto decay_copy = []<typename T>(T&& obj) { return static_cast<T&&>(obj); };

}  // namespace amongoc
