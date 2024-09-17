#pragma once

#include <neo/get.hpp>
#include <neo/meta.hpp>
#include <neo/type_traits.hpp>
#include <neo/visit.hpp>

#include <tuple>

namespace amongoc {

/**
 * @brief An invocable combinator that takes N invocables and returns a new
 * invocable that can be invoked with a variant, which automatically invokes the
 * Nth invocable based on the index of the variant
 *
 * @tparam Fs The function objects that are wrapped
 */
template <typename... Fs>
class branch {
public:
    branch() = default;

    constexpr branch(Fs&&... fs)
        : _tpl(NEO_FWD(fs)...) {}

private:
    template <typename V, std::size_t... Ns>
    static void _check_invoke(V&&, std::index_sequence<Ns...>)
        requires(neo::can_get_nth<V, Ns> and ...) and requires(Fs... fs, V v) {
            (fs(neo::get_nth<Ns>(NEO_FWD(v))), ...);
            typename std::common_reference_t<neo::invoke_result_t<Fs, neo::get_nth_t<V, Ns>>...>;
        };

public:
    /// XXX: Invocation constraints need to be made more correct
    template <neo::visitable V>
        requires(not neo::can_get_nth<V, sizeof...(Fs)>)
        and requires(V v) { _check_invoke(NEO_FWD(v), std::index_sequence_for<Fs...>{}); }
    constexpr decltype(auto) operator()(V&& var) {
        return _do_branch(NEO_FWD(var), _tpl, std::index_sequence_for<Fs...>{});
    }

private:
    using f_tpl = std::tuple<Fs...>;
    f_tpl _tpl;

    template <std::size_t N, typename Tpl, typename Var, typename Ret>
    struct brancher {
        constexpr static Ret apply(Tpl&& tpl, Var&& var) {
            return NEO_INVOKE(neo::get_nth<N>(NEO_FWD(tpl)), neo::get_nth<N>(NEO_FWD(var)));
        }
    };

    template <typename Var,
              typename Tpl,
              std::size_t... Ns,
              typename Ret
              = std::common_reference_t<neo::invoke_result_t<Fs, neo::get_nth_t<Var, Ns>>...>>
    constexpr Ret _do_branch(Var&& var, Tpl&& tpl, std::index_sequence<Ns...>) {
        Ret (*fns[])(Tpl&&, Var&&) = {&brancher<Ns, Tpl, Var, Ret>::apply...};
        return fns[var.index()](NEO_FWD(tpl), NEO_FWD(var));
    }
};

template <typename... F>
explicit branch(F&&...) -> branch<F...>;

}  // namespace amongoc
