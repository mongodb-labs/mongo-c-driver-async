/**
 * @file parse.hpp
 * @brief BSON data parsing utilities
 * @date 2024-09-27
 *
 * The interfaces in this file are for parsing and decomposing BSON documents declaratively.
 */
#pragma once

#include <bson/types.h>
#include <bson/view.h>

#include <mlib/alloc.h>
#include <mlib/config.h>

#include <iterator>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>

namespace bson::parse {

// Alias this, we use it a lot
using reference = bson::view::iterator::reference;

// Result state of trying to parse a value
enum class pstate {
    // Rejection (soft failure): The parser did not match
    reject,
    // Error (hard failure): The parser did not match, and wants to halt parsing
    error,
    // Success: The parser matched a value
    accept,
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

// Test whether the given parse result accepted its input
constexpr bool did_accept(result_type auto const& r) { return r.state() == pstate::accept; }

// Write a string to an output iterator
template <std::output_iterator<char> O>
[[nodiscard]]
constexpr O write_str(O out, std::string_view s) {
    return std::ranges::copy(s, out).out;
}

// A parse result type that represents unconditional acceptance
struct accepted {
    constexpr static pstate state() noexcept { return pstate::accept; }
    static constexpr auto   format_to(auto out) { return write_str(out, "[accepted]"); }
};

// A parse result type that represents unconditional rejection
struct rejected {
    std::string             message = "[rejected]";
    constexpr static pstate state() noexcept { return pstate::reject; }
    auto                    format_to(auto out) const { return write_str(out, message); }
};

// A parse result type that represents unconditional rejection
struct errored {
    std::string             message = "[error]";
    constexpr static pstate state() noexcept { return pstate::error; }
    auto                    format_to(auto out) const { return write_str(out, "[error]"); }
};

/**
 * @brief Match a type `R` that can be used to parse an object of type `T`
 *
 * @tparam R The candidate parsing rule
 * @tparam T The object type to be parsed
 */
template <typename R, typename T>
concept rule = requires(T const& ref, R& r) {
    { r(ref) } -> result_type;
};

// Get the result of applying a parse rule to an object
template <typename P, typename T>
    requires rule<P, T>
using result_t = decltype(std::declval<P&>()(std::declval<const T&>()));

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
 * @brief Parser combinator that accepts a value and assigns it to a target, then
 * accepts
 */
template <typename T>
struct store {
    // The assignment target. Usualy an l-value reference
    T dest;

    // Assign the given value to the target
    template <typename U>
    constexpr accepted operator()(U&& arg)
        requires requires { dest = mlib_fwd(arg); }
    {
        dest = mlib_fwd(arg);
        return {};
    }

    // Try to convert the value before assigning:
    constexpr auto operator()(const reference& elem) {
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
template <rule<reference> R>
struct field {
    // The key we are looking for
    std::string_view key;
    // The value parser
    R value_rule;

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

        constexpr auto format_to(auto out) const {
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

template <rule<reference> P>
explicit field(std::string_view, P&&) -> field<P>;

// Require a "must<field<...>>" parsing rule
template <rule<reference> R>
constexpr must<field<R>> require(std::string_view key, R&& rule) noexcept {
    return must(field(key, mlib_fwd(rule)));
}

/**
 * @brief A parser combinator that tries each rule in sequence until one accepts or errors
 */
template <typename... Rs>
struct any
    // Inject an index sequence for tuple accessing
    : any<std::index_sequence_for<Rs...>, Rs...> {
    using any<std::index_sequence_for<Rs...>, Rs...>::any;
};

// Impl for `any<>`
template <std::size_t... Ns, typename... Rs>
struct any<std::index_sequence<Ns...>, Rs...> {
    any() = default;
    constexpr explicit any(Rs&&... ps)
        requires(sizeof...(ps) != 0)
        : rules(mlib_fwd(ps)...) {}

    std::tuple<Rs...> rules;

    template <typename T>
    struct result {
        // The child results. Only children that we actually tried will generate a result
        std::tuple<std::optional<result_t<Rs, T>>...> child_results;

        constexpr pstate state() const noexcept { return _state(std::get<Ns>(child_results)...); }

        constexpr pstate _state(const auto&... subs) const noexcept {
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
        }

        template <std::size_t N>
        constexpr auto _format_nth(auto out,
                                   int& error_countdown,
                                   std::integral_constant<std::size_t, N>,
                                   const auto& sub) const {
            if (not sub.has_value() or sub->state() == pstate::accept) {
                return out;
            }
            out = sub->format_to(out);
            if (--error_countdown) {
                out = write_str(out, ", ");
            }
            return out;
        }

        constexpr auto _format_to(auto out, const auto&... subres) const {
            int n_errors
                = (0 + ... + int(subres.has_value() and subres->state() != pstate::accept));
            if (n_errors == 0) {
                return accepted{}.format_to(out);
            }
            out = write_str(out, "errors: [");
            ((out = _format_nth(out, n_errors, std::integral_constant<std::size_t, Ns>{}, subres)),
             ...);
            out = write_str(out, "]");
            return out;
        }

        constexpr auto format_to(auto out) const {
            out = _format_to(out, std::get<Ns>(child_results)...);
            return out;
        }
    };

    // Parse a value, requiring that each sub-parser is able to parse this value
    template <typename T>
        requires(rule<Rs, T> and ...)
    result<T> operator()(const T& obj) {
        result<T> ret;
        // Invoke each sub-parser in order until one does not accepts or errors
        static_cast<void>(
            (((std::get<Ns>(ret.child_results).emplace(std::get<Ns>(rules)(obj))).state()
              != pstate::reject)
             or ...));
        return ret;
    }
};

template <typename... Ps>
explicit any(Ps&&... ps) -> any<Ps...>;

// A parser combinator that immediatly accepts any input
struct just_accept {
    accepted operator()(auto const&) const noexcept { return {}; }
    accepted finish() const noexcept { return {}; }
};

// A parser combinator that immediatly accepts any input
struct just_reject {
    rejected operator()(auto const&) const noexcept { return {}; }
    rejected finish() const noexcept { return {}; }
};

/**
 * @brief A special rule for use with parse::doc that rejects the whole document
 * if an unhandled element is found
 *
 * This is implemented in a specialization of doc_part<> below
 */
struct reject_others : just_reject {};

/**
 * @brief A parser combinator that attempts to convert a BSON value into the target
 * type. If the conversion succeeds, attempts to parse the converted value using `P`
 *
 * @tparam T The type to be tested
 * @tparam P The sub-parser to apply
 */
template <typename T, rule<T> P>
struct type_parser {
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

        constexpr auto format_to(auto out) const {
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
constexpr type_parser<T, P> type(P&& parse = {}) {
    return type_parser<T, P>(mlib_fwd(parse));
}

template <rule<std::int64_t> P>
struct integral {
    P subparser;

    struct result {
        std::optional<result_t<P, std::int64_t>> subresult;

        pstate state() const noexcept { return subresult ? subresult->state() : pstate::reject; }

        auto format_to(auto out) const {
            if (not subresult) {
                return write_str(out, "element does not have an numeric type");
            }
            return subresult->format_to(out);
        }
    };

    result operator()(const reference& ref) {
        bool okay = false;
        auto i    = ref.as_int64(&okay);
        if (not okay) {
            return result{};
        }
        return result{subparser(i)};
    }
};

/**
 * @brief A parser combinator that attempts to parse each element in a document/array
 *
 * If the parser `P` rejects an element, then `each<P>` parser will reject the full document.
 *
 * @tparam P The parser to apply to each element
 */
template <rule<reference> P>
struct each {
    each() = default;
    constexpr explicit each(P&& p)
        : subparser(mlib_fwd(p)) {}

    P subparser;
    struct result {
        std::string_view                      bad_key;
        std::optional<result_t<P, reference>> subresult;

        constexpr pstate state() const noexcept {
            if (not subresult.has_value()) {
                return pstate::accept;
            }
            return subresult->state();
        }

        constexpr auto format_to(auto out) const {
            if (not subresult.has_value()) {
                return accepted{}.format_to(out);
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
            result_type auto r = subparser(el);
            if (r.state() == pstate::accept) {
                return result{el.key(), mlib_fwd(r)};
            }
        }
        return result{};
    }
};

template <rule<reference> P>
explicit each(P&&) -> each<P>;

/**
 * @brief A special document parser that is optimized for decomposing a document
 * into its constituent fields
 *
 * @tparam Ps The parser factories to parse the document content
 */
template <typename... Ps>
struct doc;

// We use this level of indirection to inject an integer sequence into the parser,
// since we will need to use that repeatedly all over.
template <rule<reference>... Es>
struct doc<Es...> : doc<std::index_sequence_for<Es...>, Es...> {
    // Inherit constructor
    using doc<std::index_sequence_for<Es...>, Es...>::doc;
};

// Implementation details for `doc<>`
struct _doc_impl {
    // Format an error from a sub-parser
    static auto format_sub(auto out, auto& sub, int& n_rejects) {
        out = sub.format_to(out);
        if (--n_rejects) {
            out = write_str(out, ", ");
        }
        return out;
    }

    // Base doc part state
    template <rule<reference> P>
    struct part_state {
        // The parser for this part
        P& parser;

        using opt_subresult = std::optional<result_t<P, reference>>;

        // The result from the the sub-parser. Will be null until we accept or error
        opt_subresult subresult{};

        // The part of the result for this sub-parser
        struct result_part {
            opt_subresult subresult;

            pstate state() const noexcept {
                if (not subresult) {
                    // We are an optional field. If the subparser did not parse,
                    // we accept
                    return pstate::accept;
                }
                // Defer to whether the sub-parser accepted
                return subresult->state();
            }

            auto format_part(auto out, int& n_rejs) const {
                if (state() == pstate::accept) {
                    return out;
                }
                return format_sub(out, *subresult, n_rejs);
            }
        };

        result_part finish() { return result_part{mlib_fwd(subresult)}; }

        // The element parser should stop trying other parts if we errored
        constexpr bool halt() const noexcept {
            return subresult and subresult->state() == pstate::error;
        }

        constexpr pstate handle(const reference& elem) {
            auto subres = this->parser(elem);
            auto st     = subres.state();
            if (st != pstate::reject) {
                // We accepted or errored on this element.
                subresult.emplace(mlib_fwd(subres));
            }
            return st;
        }
    };

    static auto format_missing(auto out, const auto&) {
        return write_str(out, "missing required element");
    }

    template <typename Sub>
    static auto format_missing(auto out, const field<Sub>& f) {
        out = write_str(out, "missing required element ‘");
        out = write_str(out, f.key);
        return write_str(out, "’");
    }

    // Doc parse state with a "must<>" parser
    template <rule<reference> P>
    struct part_state<must<P>> : part_state<P> {
        // Reuse coode from part_state<P> for the base non-required parser
        explicit part_state(must<P>& m)
            : part_state<P>(m.rule) {}

        struct result_part {
            P& parser;

            std::optional<result_t<P, reference>> subresult;

            pstate state() const noexcept {
                if (not subresult) {
                    // We are must<>: If the subparser didn't accept, we reject
                    return pstate::reject;
                }
                return subresult->state();
            }

            auto format_part(auto out, int& n_rejs) const {
                if (subresult and subresult->state() == pstate::accept) {
                    // Subparser accepted, no error
                    return out;
                }
                if (not subresult) {
                    // We did not parse a required element
                    out = format_missing(out, parser);
                } else {
                    out = subresult->format_to(out);
                }
                if (--n_rejs) {
                    out = write_str(out, ", ");
                }
                return out;
            }
        };

        result_part finish() { return result_part{this->parser, mlib_fwd(this->subresult)}; }
    };

    // The N parameter is required to ensure all base classes are unique
    template <std::size_t N, rule<reference> P>
    struct part {
        explicit constexpr part(P&& p)
            : parser(mlib_fwd(p)) {}
        P parser;

        part_state<P> state_part() noexcept { return part_state<P>{parser}; }
    };
};

// Special doc parsing part that generates a full rejection when an unexpected key is found
template <>
struct _doc_impl::part_state<reject_others> {
    // Required for constructor uniformity
    reject_others unused{};

    // The key of the element that we didn't expect to see
    std::optional<std::string_view> got_key{};

    struct result_part {
        std::optional<std::string_view> got_key;

        pstate state() const noexcept {
            if (got_key.has_value()) {
                // We found an element that we didn't want
                return pstate::reject;
            }
            return pstate::accept;
        }

        auto format_part(auto out, int& n_rejs) const {
            if (not got_key.has_value()) {
                return out;
            }
            out = write_str(out, "unexpected element ‘");
            out = write_str(out, *got_key);
            out = write_str(out, "’");
            if (--n_rejs) {
                out = write_str(out, ", ");
            }
            return out;
        }
    };

    result_part finish() { return result_part{got_key}; }

    // The element parser should stop if we found a rejected key
    constexpr bool halt() const noexcept { return got_key.has_value(); }

    constexpr pstate handle(const reference& elem) {
        got_key = elem.key();
        return pstate::reject;
    }
};

// The actual doc parser implementation, with the index sequence given as the first template
// argument.
template <std::size_t... Ns, rule<reference>... Ps>
struct doc<std::index_sequence<Ns...>, Ps...>
    // Inherit each part:
    : _doc_impl::part<Ns, Ps>... {
    // Construct each part:
    explicit constexpr doc(Ps&&... ps)
        : _doc_impl::part<Ns, Ps>(mlib_fwd(ps))... {}

    struct result {
        std::optional<std::tuple<typename _doc_impl::part_state<Ps>::result_part...>> subresults;

        pstate state() const noexcept {
            if (not subresults.has_value()) {
                // The input element is not a document
                return pstate::reject;
            }
            // Error if any elements error
            if (((std::get<Ns>(*subresults).state() == pstate::error) or ...)) {
                return pstate::error;
            }
            // Accept if all elements accept
            if (((std::get<Ns>(*subresults).state() == pstate::accept) and ...)) {
                return pstate::accept;
            }
            // Reject otherwise
            return pstate::reject;
        }

        auto format_to(auto o) const {
            if (not subresults.has_value()) {
                return write_str(o, "element is not a document/array");
            }
            // Count number of non-accepted rules:
            auto n_rejs = (0 + ... + (std::get<Ns>(*subresults).state() != pstate::accept));
            if (n_rejs == 0) {
                return accepted{}.format_to(o);
            }
            // Format each failing rule:
            o = write_str(o, "errors: [");
            ((o = std::get<Ns>(*subresults).format_part(o, n_rejs)), ...);
            o = write_str(o, "]");
            return o;
        }
    };

    // l-ref qualify this function, because the returned result usually contains
    // references to internals that will almost certainly be needed to generate the error message
    result operator()(const bson::view doc) & {
        auto state_parts = std::tuple(this->_doc_impl::part<Ns, Ps>::state_part()...);
        for (const reference& elem : doc) {
            // Invoke each sub-parser on this element until one accepts or errors
            pstate stop_state{};
            static_cast<void>(
                (((stop_state = std::get<Ns>(state_parts).handle(elem)) != pstate::reject) or ...));
            if (((std::get<Ns>(state_parts).halt()) or ...)) {
                break;
            }
        }
        return result{std::tuple(std::get<Ns>(state_parts).finish()...)};
    }

    result operator()(const reference& elem) & {
        auto doc = elem.document();
        if (doc.data() == nullptr) {
            return result{};  // Not a document
        }
        return (*this)(doc);
    }
};

template <rule<reference>... Es>
explicit doc(Es&&...) -> doc<Es...>;

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
