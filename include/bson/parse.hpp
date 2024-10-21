/**
 * @file parse.hpp
 * @brief BSON data parsing utilities
 * @date 2024-09-27
 *
 * The interfaces in this file are for parsing and decomposing BSON documents declaratively.
 */
#pragma once

#include <bson/iterator.h>
#include <bson/types.h>
#include <bson/view.h>

#include <mlib/alloc.h>
#include <mlib/config.h>
#include <mlib/object_t.hpp>

#include <csignal>
#include <iterator>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

namespace bson::parse {

// Alias this, we use it a lot
using reference = bson::view::iterator::reference;

// Result state of trying to parse a value
enum pstate {
    // Rejection (soft failure): The parser did not match
    reject = 0,
    // Success: The parser matched a value
    accept = 0b01,
    // Error (hard failure): The parser did not match, and wants to halt parsing
    error = 0b10,
};

/**
 * @brief Match a type that represents the result of parsing a value
 */
template <typename T>
concept result_type = requires(const T& x, std::back_insert_iterator<std::string> out) {
    // The result must declare a state
    { x.state() } -> std::convertible_to<pstate>;
    // The result must have a way to format itself as a string (should be written in terms of
    // a generic character output iterator)
    { x.format_to(out) } -> std::same_as<std::back_insert_iterator<std::string>>;
};

/**
 * @brief Match a type `R` that can be used to parse an object of type `T`
 *
 * @tparam R The candidate parsing rule
 * @tparam T The object type to be parsed
 */
template <typename Rule, typename Arg>
concept rule = requires(Arg const& arg, Rule& rule) {
    { rule(arg) } -> result_type;
};

template <typename R, typename T>
void requires_rule();

template <typename T, typename... Rs>
concept all_valid_rules = requires { (requires_rule<Rs, T>(), ...); };

// Get the result of applying a parse rule to an object
template <typename P, typename T>
    requires rule<P, T>
using result_t = decltype(std::declval<P&>()(std::declval<const T&>()));

// Test whether the given parse result accepted its input
constexpr bool did_accept(result_type auto const& r) noexcept {
    return r.state() == pstate::accept;
}

// Write a string to an output iterator
template <std::output_iterator<char> O>
[[nodiscard]]
constexpr O write_str(O out, std::string_view s) {
    return std::ranges::copy(s, out).out;
}

extern template std::back_insert_iterator<std::string>
    write_str(std::back_insert_iterator<std::string>, std::string_view);

/**
 * @brief A basic parsing result that holds a state and an optional fixed statically-allocated
 * message string
 */
struct basic_result {
    // The end state. Default is to accept
    pstate _state = pstate::accept;
    // An optional message to include when the result is formatted
    std::optional<std::string_view> message{};

    constexpr pstate state() const noexcept { return _state; }

    template <typename O>
    constexpr O format_to(O out) const {
        if (message) {
            return write_str(out, *message);
        } else {
            if (_state == pstate::accept) {
                return write_str(out, "[accepted]");
            } else {
                return write_str(out, "[rejected]");
            }
        }
    }
};

template <std::size_t N, typename R>
struct variadic_nth_storage {
    R item;
};

template <typename... Rs>
struct variadic_base : variadic_base<std::index_sequence_for<Rs...>, Rs...> {
    using variadic_base<std::index_sequence_for<Rs...>, Rs...>::variadic_base;
};

template <std::size_t... Ns, typename... Rs>
struct variadic_base<std::index_sequence<Ns...>, Rs...> : variadic_nth_storage<Ns, Rs>... {
    variadic_base() = default;
    constexpr explicit variadic_base(Rs&&... ps)
        requires(sizeof...(ps) != 0)
        : variadic_nth_storage<Ns, Rs>{mlib_fwd(ps)}... {}

    template <typename F>
    constexpr decltype(auto) apply_all(F&& fn) {
        return fn(this->variadic_nth_storage<Ns, Rs>::item...);
    }

    template <typename F>
    constexpr decltype(auto) apply_all(F&& fn) const {
        return fn(this->variadic_nth_storage<Ns, Rs>::item...);
    }
};

/**
 * @brief A parser combinator that tries each rule in sequence until one accepts or errors
 */
template <typename... Rs>
struct any : variadic_base<Rs...> {
    using any::variadic_base::variadic_base;

    template <typename T>
    struct result : variadic_base<std::optional<result_t<Rs, T>>...> {
        constexpr pstate state() const noexcept {
            return this->apply_all([&](const auto&... subs) -> pstate {
                // We accept if any child accepted
                bool accept = ((subs.has_value() and subs->state() == pstate::accept) or ...);
                if (accept) {
                    return pstate::accept;
                }
                // We hard-error if any child hard-errored
                bool hard_err = ((subs.has_value() and subs->state() == pstate::error) or ...);
                if (hard_err) {
                    return pstate::error;
                }
                // Otherwise, we reject
                return pstate::reject;
            });
        }

        template <typename O>
        constexpr O _format_nth(O out, int& error_countdown, const auto& sub) const {
            if (not sub.has_value() or sub->state() == pstate::accept) {
                return out;
            }
            out = sub->format_to(out);
            if (--error_countdown) {
                out = write_str(out, ", ");
            }
            return out;
        }

        template <typename O>
        constexpr O format_to(O out) const {
            return this->apply_all([&](const auto&... subs) -> O {
                int n_errors
                    = (0 + ... + int(subs.has_value() and subs->state() != pstate::accept));
                if (n_errors == 0) {
                    return write_str(out, "[rejected]");
                }
                out = write_str(out, "no candidate rule was satisfied: [");
                ((out = _format_nth(out, n_errors, subs)), ...);
                out = write_str(out, "]");
                return out;
            });
        }
    };

    // Parse a value, requiring that all sub-parsers are able to parse this type
    template <typename T>
        requires(rule<Rs, T> and ...)
    result<T> operator()(const T& obj) {
        result<T> ret;
        this->apply_all([&](auto&... rules) {
            ret.apply_all([&](auto&... subresults_opts) {
                // Invoke each sub-parser in order until one accepts or errors
                static_cast<void>(
                    (((subresults_opts.emplace(rules(obj))).state() != pstate::reject) or ...));
            });
        });
        return ret;
    }
};

template <typename... Ps>
explicit any(Ps&&... ps) -> any<Ps...>;

/**
 * @brief A parser combinator that requires that all rules accept
 *
 * @tparam Rs
 */
template <typename... Rs>
struct all : variadic_base<Rs...> {
    using all::variadic_base::variadic_base;

    template <typename T>
    struct result : variadic_base<std::optional<result_t<Rs, T>>...> {
        constexpr pstate state() const noexcept {
            return this->apply_all([&](const auto&... subs) -> pstate {
                // We accept if all children accepted
                bool accept = ((subs.has_value() and subs->state() == pstate::accept) and ...);
                if (accept) {
                    return pstate::accept;
                }
                // We hard-error if any child hard-errored
                bool hard_err = ((subs.has_value() and subs->state() == pstate::error) or ...);
                if (hard_err) {
                    return pstate::error;
                }
                // Otherwise, we reject
                return pstate::reject;
            });
        }

        template <typename O>
        constexpr O _format_nth(O out, int& error_countdown, const auto& sub) const {
            if (not sub.has_value() or sub->state() == pstate::accept) {
                return out;
            }
            out = sub->format_to(out);
            if (--error_countdown) {
                out = write_str(out, ", ");
            }
            return out;
        }

        template <typename O>
        constexpr O format_to(O out) const {
            return this->apply_all([&](const auto&... subs) -> O {
                int n_errors
                    = (0 + ... + int(subs.has_value() and subs->state() != pstate::accept));
                if (n_errors == 0) {
                    return write_str(out, "[accepted]");
                }
                out = write_str(out, "on or more parse rules were unsatisfied: [");
                ((out = _format_nth(out, n_errors, subs)), ...);
                out = write_str(out, "]");
                return out;
            });
        }
    };

    // Parse a value, requiring that all sub-parsers are able to parse this type
    template <typename T>
        requires(rule<Rs, T> and ...)
    result<T> operator()(const T& obj) {
        result<T> ret;
        this->apply_all([&](auto&... rules) {
            ret.apply_all([&](auto&... subresults_opts) {
                // Invoke each sub-parser in order until one does not accept
                static_cast<void>(
                    (((subresults_opts.emplace(rules(obj))).state() == pstate::accept) and ...));
            });
        });
        return ret;
    }
};

template <typename... Ps>
explicit all(Ps&&... ps) -> all<Ps...>;

// A parser combinator that immediatly accepts any input
struct just_accept : all<> {};

// A parser combinator that immediatly rejects any input
struct just_reject : any<> {};

/**
 * @brief A special rule for use with parse::doc that rejects the whole document
 * if an unhandled element is found
 *
 * This is implemented in a specialization of doc_part<> below
 */
struct reject_others : just_reject {};

/**
 * @brief Parser combinator that turns rejection (soft failure) into errors (hard failure)
 */
template <typename R>
struct must {
    // The rule that we wrap
    R rule;

    template <typename T>
    struct result : result_t<R, T> {
        constexpr pstate state() const noexcept {
            // If the sub-rule rejects, then we error
            pstate s = this->result_t<R, T>::state();
            if (s == pstate::reject) {
                return pstate::error;
            }
            return s;
        }
    };

    // Parse using the underlying rule
    template <typename T>
        requires parse::rule<R, T>
    constexpr result<T> operator()(T const& ref) {
        return result<T>(this->rule(ref));
    }
};

template <typename P>
explicit must(P&&) -> must<P>;

/**
 * @brief A parser combinator that attempts to convert a BSON value into the target
 * type. If the conversion succeeds, attempts to parse the converted value using `P`
 *
 * @tparam T The type to be tested
 * @tparam P The sub-parser to apply
 */
template <typename T, rule<T> P>
struct type_rule {
    P subparser;

    struct result {
        std::optional<result_t<P, T>> subresult;

        pstate state() const noexcept {
            if (not subresult.has_value()) {
                // Type conversion failed
                return pstate::reject;
            }
            return subresult->state();
        }

        template <typename O>
        constexpr O format_to(O out) const {
            if (not subresult.has_value()) {
                return write_str(out, "element has incorrect type");
            } else {
                return subresult->format_to(out);
            }
        }
    };

    result operator()(const reference& value) {
        auto opt = value.try_as<T>();
        if (not opt) {
            // Type conversion failed
            return {};
        }
        return result{subparser(*opt)};
    }
};

/**
 * @brief Create a parser combinator that tries to convert to the given type,
 * and then applies a sub-parser to that converted value
 *
 * @tparam T The target type for the conversion
 * @param parse A parser that can parse the object `T`
 */
template <typename T, rule<T> P = just_accept>
constexpr type_rule<T, P> type(P&& parse = {}) {
    return type_rule<T, P>(mlib_fwd(parse));
}

/**
 * @brief A parser rule that simply invokes a function with the parse value
 * and then accepts.
 */
template <typename F>
struct action {
    mlib::object_t<F> _action;

    template <typename T>
    constexpr basic_result operator()(T&& value) {
        _action(mlib_fwd(value));
        return {};
    }
};

template <typename F>
explicit action(F&&) -> action<F>;

/**
 * @brief Parser combinator that accepts a value and assigns it to a target, then
 * accepts
 */
template <typename T>
struct store {
    // The assignment target. Usualy an l-value reference
    T dest;

    // Assign the given value to the target
    template <typename U>
    constexpr basic_result operator()(U&& arg)
        requires requires { dest = mlib_fwd(arg); }
    {
        dest = mlib_fwd(arg);
        return {pstate::accept};
    }

    // Try to convert the value before assigning:
    constexpr result_type auto operator()(const reference& elem) {
        return type<std::remove_cvref_t<T>>(store(dest))(elem);
    }
};

template <typename T>
explicit store(T&&) -> store<T>;

/**
 * @brief A rule that tries to match an element with a key and parse its value
 *
 * @tparam R A parsing rule for the element value
 */
template <typename R>
struct field {
    // The key we are looking for
    std::string_view key;
    // The value parser
    R value_rule;

    static_assert(parse::rule<R, reference>,
                  "Sub-rule given to field<> must be a valid rule for parsing a BSON element");

    struct result {
        // The key we tried to find
        std::string_view key;
        // The sub-result of the wrapped parser, if it was invoked
        std::optional<result_t<R, reference>> subresult{};

        constexpr pstate state() const noexcept {
            if (not subresult.has_value()) {
                // We did not find the element key
                return pstate::reject;
            }
            // Defer to the subparser
            return subresult->state();
        }

        template <typename O>
        constexpr O format_to(O out) const {
            if (state() == pstate::accept) {
                return write_str(out, "[accepted]");
            }
            if (subresult.has_value()) {
                // We found the field.
                out = write_str(out, "in field ‘");
                out = write_str(out, key);
                out = write_str(out, "’: ");
                subresult->format_to(out);
            } else {
                // We did not find the field
                out = write_str(out, "element ‘");
                out = write_str(out, key);
                out = write_str(out, "’ not found");
            }
            return out;
        }
    };

    // We parsing a document, scan for the field and parse that
    constexpr result operator()(view doc) {
        result ret{key};
        auto   iter = doc.find(key);
        if (iter == doc.end()) {
            return ret;
        }
        ret.subresult.emplace(this->value_rule(*iter));
        return ret;
    }

    // We are parsing an element directly. Try to match its key
    constexpr result operator()(const reference& ref) {
        result ret{key};
        if (ref.key() != this->key) {
            return ret;
        }
        ret.subresult.emplace(this->value_rule(ref));
        return ret;
    }
};

template <typename Rule>
explicit field(std::string_view, Rule&&) -> field<Rule>;

// Require a "must<field<...>>" parsing rule
template <typename R>
constexpr must<field<R>> require(std::string_view key, R&& rule) noexcept {
    return must(field(key, mlib_fwd(rule)));
}

/**
 * @brief A parse rule that expects an integer value
 *
 * @tparam P
 */
template <typename Rule = just_accept>
struct integer {
    Rule rule;

    static_assert(parse::rule<Rule, std::int64_t>,
                  "Sub-rule given to integer<> rule must be a rule to handle a std::int64_t");

    struct result {
        std::optional<result_t<Rule, std::int64_t>> subresult;

        constexpr pstate state() const noexcept {
            return subresult ? subresult->state() : pstate::reject;
        }

        template <typename O>
        constexpr O format_to(O out) const {
            if (not subresult) {
                return write_str(out, "element does not have a numeric type");
            }
            return subresult->format_to(out);
        }
    };

    result operator()(const reference& ref) {
        if (ref.type() == ::bson_type_int32 or ref.type() == ::bson_type_int64) {
            return result{rule(ref.value().as_int64())};
        }
        return {};
    }
};

/**
 * @brief A parser combinator that attempts to parse each element in a document/array
 *
 * If the parser `P` rejects an element, then `each<P>` parser will reject the full document.
 *
 * @tparam P The parser to apply to each element
 */
template <typename Rule>
struct each {
    Rule rule;

    static_assert(parse::rule<Rule, reference>,
                  "Sub-rule for each<> must be a rule to parse a BSON element");

    struct result {
        std::string_view                         bad_key;
        std::optional<result_t<Rule, reference>> subresult;

        constexpr pstate state() const noexcept {
            if (not subresult.has_value()) {
                return pstate::accept;
            }
            return subresult->state();
        }

        template <typename O>
        constexpr O format_to(O out) const {
            if (not subresult.has_value()) {
                return basic_result{pstate::accept}.format_to(out);
            } else {
                out = write_str(out, "field ‘");
                out = write_str(out, bad_key);
                out = write_str(out, "’ was rejected: ");
                return subresult->format_to(out);
            }
        }
    };

    result operator()(view doc) {
        for (const reference& el : doc) {
            result_type auto r = rule(el);
            if (r.state() != pstate::accept) {
                return result{el.key(), mlib_fwd(r)};
            }
        }
        return result{};
    }

    result operator()(bson_array_view arr) { return (*this)(bson_view(arr)); }
};

template <rule<reference> P>
explicit each(P&&) -> each<P>;

/**
 * @brief A parser combinator that attempts to match a rule. It will always accept
 * the input, unless the sub-rule generate an error, in which case `maybe<>` will
 * propagate that error.
 */
template <typename R>
struct maybe {
    R rule;

    template <typename T>
    struct result {
        std::optional<result_t<R, T>> error_subresult;

        constexpr pstate state() const noexcept {
            if (not error_subresult.has_value()) {
                return pstate::accept;
            }
            return pstate::error;
        }

        template <typename O>
        constexpr O format_to(O out) const {
            if (not error_subresult.has_value()) {
                return write_str(out, "[accepted]");
            }
            return error_subresult->format_to(out);
        }
    };

    template <typename T>
        requires parse::rule<R, T>
    constexpr result<T> operator()(const T& arg) {
        auto subres = this->rule(arg);
        if (subres.state() != pstate::error) {
            return {};
        }
        return {mlib_fwd(subres)};
    }
};

template <typename R>
explicit maybe(R&&) -> maybe<R>;

/**
 * @brief A special document parser that is optimized for decomposing a document
 * into its constituent fields
 *
 * @tparam Rs The parsers to parse the document content
 */
template <typename... Rs>
struct doc;

// Implmentation details for doc<>
struct doc_impl {
    template <typename R>
    struct part {
        R& rule;

        bool did_accept      = false;
        bool did_error       = false;
        using subresult_type = result_t<R&, reference>;

        std::optional<subresult_type> rejection_subresult{};

        constexpr bool done_looking() const noexcept { return this->did_accept; }

        constexpr basic_result handle_element(const reference& ref) {
            if (this->did_accept) {
                // We've already been fulfilled. Nothing to do
                return {pstate::reject};
            }
            const result_type auto subresult = this->rule(ref);
            if (subresult.state() == pstate::accept) {
                // This is what we were looking for!
                this->did_accept = true;
                this->rejection_subresult.reset();
            } else {
                // Error or rejection
                this->rejection_subresult.emplace(subresult);
                if (subresult.state() == pstate::error) {
                    this->did_error = true;
                }
            }
            return {subresult.state()};
        }

        constexpr pstate final_state() const noexcept {
            if (did_error) {
                return pstate::error;
            } else {
                // We are not a required field, so we accept as long as we did not
                // encounter any errors
                return pstate::accept;
            }
        }

        template <typename O>
        constexpr O format_to_with_comma(O out, int& n_rejected) const {
            if (this->final_state() == pstate::accept) {
                return out;
            }
            if (this->rejection_subresult.has_value()) {
                out = this->rejection_subresult->format_to(out);
            } else {
                out = write_str(out, "TODO: doc part format message");
            }
            if (--n_rejected) {
                out = write_str(out, ", ");
            }
            return out;
        }
    };

    template <typename R>
    struct part<must<R>> : part<R> {
        // This rule represents a required field

        constexpr part(must<R>& rule) noexcept
            : part<R>(rule.rule) {}

        constexpr pstate final_state() const noexcept {
            if (this->did_error) {
                return pstate::error;
            } else if (not this->did_accept) {
                return pstate::reject;
            } else {
                return pstate::accept;
            }
        }

        template <typename O>
        constexpr O format_to_with_comma(O out, int& n_rejected) const {
            if (this->did_accept) {
                // Nothing to write
                return out;
            }
            if (this->rejection_subresult) {
                out = this->rejection_subresult->format_to(out);
            } else {
                out = _format_no_result(out, this->rule);
            }
            if (--n_rejected) {
                out = write_str(out, ", ");
            }
            return out;
        }

        template <typename T>
        static auto _format_no_result(auto out, field<T>& r) {
            out = write_str(out, "element ‘");
            out = write_str(out, r.key);
            out = write_str(out, "’ not found");
            return out;
        }

        static auto _format_no_result(auto out, auto&) {
            return write_str(out, "[unimplemented parsing error formating]");
        }
    };
};

template <>
struct doc_impl::part<reject_others> {
    // This special will reject if it is tested on anything, and rejects the entire document.

    std::optional<std::string_view> got_key;

    constexpr part(reject_others) noexcept {}

    // We are done if we are ever tested on any element
    constexpr bool done_looking() const noexcept { return this->got_key.has_value(); }

    constexpr basic_result handle_element(const reference& elem) {
        got_key.emplace(elem.key());
        return {pstate::reject};
    }

    constexpr pstate final_state() const noexcept {
        if (this->got_key.has_value()) {
            return pstate::reject;
        } else {
            return pstate::accept;
        }
    }

    template <typename O>
    constexpr O format_to_with_comma(O out, int& n_rejected) const {
        if (not this->got_key) {
            // No error
            return out;
        }
        out = write_str(out, "unexpected element ‘");
        out = write_str(out, *this->got_key);
        out = write_str(out, "’");
        if (--n_rejected) {
            out = write_str(out, ", ");
        }
        return out;
    }
};

template <typename... Ts>
struct doc : variadic_base<Ts...> {
    using doc::variadic_base::variadic_base;

    struct final_result;

    struct stateful : variadic_base<doc_impl::part<Ts>...> {
        constexpr final_result parse(bson::view view);
    };

    struct final_result {
        std::optional<stateful> self;

        constexpr int _nreject() const noexcept {
            // Count the number of rules that reject as their final result
            return self->apply_all([](const doc_impl::part<Ts>&... parts) {
                return ((parts.final_state() != pstate::accept) + ...);
            });
        }

        constexpr pstate state() const noexcept {
            if (not self.has_value()) {
                return pstate::reject;
            }
            // Did any rule generate a final error
            const bool any_error = self->apply_all([](const doc_impl::part<Ts>&... parts) {
                return ((parts.final_state() == pstate::error) or ...);
            });
            if (any_error) {
                return pstate::error;
            }
            if (_nreject()) {
                return pstate::reject;
            }
            return pstate::accept;
        }

        template <typename O>
        constexpr O format_to(O out) const {
            if (not self.has_value()) {
                return write_str(out, "the given value is not a document element");
            }
            int n_rejected_or_errored = this->_nreject();
            if (n_rejected_or_errored) {
                // Build the error message.
                out = write_str(out, "errors: [");
                self->apply_all([&](const doc_impl::part<Ts>&... parts) {
                    static_cast<void>(
                        ((out = (parts.format_to_with_comma(out, n_rejected_or_errored))), ...));
                });
                out = write_str(out, "]");
            } else {
                out = write_str(out, "[accepted]");
            }
            return out;
        }
    };

    constexpr stateful stateful_parser() noexcept {
        return this->apply_all([](Ts&... rules) -> stateful {  //
            return stateful{variadic_base<doc_impl::part<Ts>...>{doc_impl::part<Ts>{rules}...}};
        });
    }

    constexpr final_result operator()(const bson::view& doc) {
        auto p = this->stateful_parser();
        return p.parse(doc);
    }

    constexpr final_result operator()(const reference& ref) {
        auto d = ref.value().get_document_or_array();
        if (not d) {
            // Indicate that the value is not a document/array:
            return final_result{};
        }
        return (*this)(d);
    }
};

template <typename... Ts>
explicit doc(Ts&&...) -> doc<Ts...>;

template <typename... Ts>
constexpr typename doc<Ts...>::final_result doc<Ts...>::stateful::parse(bson::view view) {
    return this->apply_all([&](doc_impl::part<Ts>&... parts) {
        for (const auto& element : view) {
            if ((parts.done_looking() and ...)) {
                break;
            }
            pstate merged = pstate::reject;
            // Invoke each sub-parser until one accepts or errors
            static_cast<void>((((merged = (pstate)(merged | parts.handle_element(element).state()))
                                == pstate::reject)
                               and ...));
            if (merged & pstate::error) {
                // At least one of the rules generated a hard-error
                break;
            }
        }
        return final_result{*this};
    });
}

// Write the description of an error to the given character output
template <std::output_iterator<char> O, result_type R>
constexpr void describe_error(O out, const R& res) {
    res.format_to(out);
}

// Generate a std::string describing the result of a parse
template <result_type R>
constexpr std::string describe_error(const R& res) {
    std::string ret;
    describe_error(std::back_inserter(ret), res);
    return ret;
}

/**
 * @brief Parse the given value according to the given rule, throwing an exception
 * if the parse fails
 *
 * @param value The value to be parsed
 * @param rule The rule to be matched
 *
 * If the given rule does not match, throw std::system_error(std::errc::protocol_error).
 * The error message string describes the reason the parse failed.
 */
template <typename T, rule<T> R>
void must_parse(const T& value, R&& rule) {
    result_type auto res = rule(value);
    if (res.state() != pstate::accept) {
        auto e = describe_error(res);
        throw std::system_error(std::make_error_code(std::errc::protocol_error), e);
    }
}

}  // namespace bson::parse
