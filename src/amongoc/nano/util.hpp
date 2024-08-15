#pragma once

#include "./query.hpp"

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

    constexpr explicit operator neo::invoke_result_t<F>() {
        return NEO_INVOKE(static_cast<F&&>(_func));
    }

    constexpr explicit operator neo::invoke_result_t<const F>() const {
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
    return deferred_conversion<F>{NEO_FWD(fn)};
}

// Enclose a partially-applied invocable object so that it may be used as the operand to operator|
template <typename F, typename... Args>
struct [[nodiscard]] closure {
    NEO_NO_UNIQUE_ADDRESS neo::object_box<F> _function;
    NEO_NO_UNIQUE_ADDRESS std::tuple<Args...> _args;

    template <typename Arg, std::size_t... Ns>
    constexpr static auto apply(auto&& self, Arg&& arg, std::index_sequence<Ns...>)
        AMONGOC_RETURNS(std::invoke(NEO_FWD(self)._function.get(),
                                    NEO_FWD(arg),
                                    std::get<Ns>(NEO_FWD(self)._args)...));

    // Handle the closure object appear on the right-hand of a vertical pipe expression
    template <typename Left, neo::alike<closure> Self>
        requires neo::invocable2<F, Left, Args...>
    friend constexpr auto operator|(Left&& lhs, Self&& rhs) AMONGOC_RETURNS(
        closure::apply(NEO_FWD(rhs), NEO_FWD(lhs), std::make_index_sequence<sizeof...(Args)>{}));
};

template <typename T, typename... Args>
constexpr closure<T, Args...> make_closure(T&& func, Args&&... args) noexcept {
    return closure<T, Args...>{NEO_FWD(func), std::forward_as_tuple(NEO_FWD(args)...)};
}

template <std::size_t N>
using size_constant = std::integral_constant<std::size_t, N>;

/**
 * @brief Create a composed function from two functions (f ∘ g)
 *
 * The expression `atop(f, g)(x)` is equivalent to `f(g(x))`
 *
 * The `atop` object forwards `query()` calls to the `f` function.
 */
template <typename F, typename G>
class atop {
public:
    atop() = default;
    constexpr explicit atop(F&& f, G&& g)
        : _f(NEO_FWD(f))
        , _g(NEO_FWD(g)) {}

private:
    NEO_NO_UNIQUE_ADDRESS neo::object_t<F> _f;
    NEO_NO_UNIQUE_ADDRESS neo::object_t<G> _g;

public:
    template <typename X>
        requires neo::invocable2<const G&, X&&>
        and neo::invocable2<const F&, neo::invoke_result_t<const G&, X&&>>
    constexpr auto operator()(X&& x) const&  //
        AMONGOC_RETURNS(NEO_INVOKE(static_cast<const F&>(_f),
                                   NEO_INVOKE(static_cast<const G&>(_g), NEO_FWD(x))));

    template <typename X>
        requires neo::invocable2<G&, X&&>
        and neo::invocable2<F&, neo::invoke_result_t<G&, X&&>>
    constexpr auto operator()(X&& x) &  //
        AMONGOC_RETURNS(NEO_INVOKE(static_cast<F&>(_f),
                                   NEO_INVOKE(static_cast<G&>(_g), NEO_FWD(x))));

    template <typename X>
        requires neo::invocable2<G&&, X&&> and neo::invocable2<F&&, neo::invoke_result_t<G&&, X&&>>
    constexpr auto operator()(X&& x) &&  //
        AMONGOC_RETURNS(NEO_INVOKE(static_cast<F&&>(_f),
                                   NEO_INVOKE(static_cast<G&&>(_g), NEO_FWD(x))));

    template <typename X>
        requires neo::invocable2<const G&&, X&&>
        and neo::invocable2<const F&&, neo::invoke_result_t<const G&&, X&&>>
    constexpr auto operator()(X&& x) const&&  //
        AMONGOC_RETURNS(NEO_INVOKE(static_cast<F const&&>(_f),
                                   NEO_INVOKE(static_cast<G const&&>(_g), NEO_FWD(x))));

    constexpr auto query(valid_query_for<F> auto q) const {
        return static_cast<const F&>(_f).query(q);
    }
};

template <typename F, typename G>
explicit atop(F&&, G&&) -> atop<F, G>;

/**
 * @brief An invocable object that always returns the given value regardless
 * of arguments
 *
 * @tparam T The type of object stored in the handler
 */
template <typename T>
class konst {
public:
    constexpr explicit konst(T&& t)
        : _value(NEO_FWD(t)) {}

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
explicit konst(T&&) -> konst<T>;

/**
 * @brief Variable template for an invocable that calls the constructor of an object
 *
 * @tparam T The type to be constructed.
 *
 * Use this to create an invocable from an arbitrary type, treating it's constructor
 * overload set as an invocable object
 */
template <typename T>
constexpr auto construct = []<typename... Args>(Args && ... args) -> T
    requires std::constructible_from<T, Args...>
{
    return T(NEO_FWD(args)...);
};

}  // namespace amongoc
