#pragma once

#include <bson/doc.h>
#include <bson/iterator.h>
#include <bson/mut.h>
#include <bson/types.h>
#include <bson/value_ref.h>
#include <bson/view.h>

#include <mlib/str.h>

#include <concepts>
#include <cstddef>
#include <ranges>
#include <string_view>
#include <utility>

namespace bson::make {

/**
 * @brief Match a type that can be directly emplaced into a new BSON document element.
 *
 * Checks against document's `emplace_back` method
 */
template <typename T>
concept value_type = requires(const T& obj, bson::mutator doc) { doc.emplace_back("", obj); };

/**
 * @brief Match a type that exposes its own method of appending itself as an element
 * to an existing document
 */
template <typename T>
concept indirect_appendable_value
    = requires(const T& obj, bson::mutator& doc, std::string_view key) {
          // The vlaue must declare a byte size
          { obj.byte_size() } -> std::convertible_to<std::size_t>;
          // The element must have a method to append to an existing document with the given key
          obj.append_to(doc, key);
      };

// Append a value to an existing document diretly
template <value_type V>
void append_value(mutator& doc, std::string_view key, const V& v) {
    doc.emplace_back(key, v);
}

// Append a value to an existing document using that object's append_to method
template <indirect_appendable_value V>
void append_value(mutator& doc, std::string_view key, const V& v) {
    v.append_to(doc, key);
}

// Obtain the byte-size of a value
inline constexpr struct value_byte_size_fn {
    template <indirect_appendable_value E>
    constexpr std::size_t operator()(const E& e) const noexcept {
        return e.byte_size();
    }

    constexpr std::size_t operator()(bson_eod) const noexcept { return 0; }
    constexpr std::size_t operator()(bson::null) const noexcept { return 0; }
    constexpr std::size_t operator()(bson::undefined) const noexcept { return 0; }
    constexpr std::size_t operator()(bson::minkey) const noexcept { return 0; }
    constexpr std::size_t operator()(bson::maxkey) const noexcept { return 0; }
    constexpr std::size_t operator()(bson_binary_view bin) const noexcept {
        return sizeof(std::int32_t) + 1 + bin.data_len;
    }
    constexpr std::size_t operator()(bson_datetime dt) const noexcept { return sizeof dt; }
    constexpr std::size_t operator()(bson_oid oid) const noexcept { return sizeof oid; }
    constexpr std::size_t operator()(bson_timestamp ts) const noexcept { return sizeof ts; }
    constexpr std::size_t operator()(bson_symbol_view sym) const noexcept {
        return (*this)(sym.utf8);
    }
    constexpr std::size_t operator()(bson_code_view c) const noexcept { return (*this)(c.utf8); }
    constexpr std::size_t operator()(bson_dbpointer_view dbp) const noexcept {
        return (*this)(dbp.collection) + 12;
    }
    constexpr std::size_t operator()(bson_decimal128) const noexcept {
        return sizeof(bson_decimal128);
    }
    constexpr std::size_t operator()(bson_regex_view rx) const noexcept {
        return rx.regex.len + 1 + rx.options.len + 1;
    }
    constexpr std::size_t operator()(double) const noexcept { return sizeof(double); }
    constexpr std::size_t operator()(bool) const noexcept { return 1; }
    constexpr std::size_t operator()(std::int32_t) const noexcept { return sizeof(std::int32_t); }
    constexpr std::size_t operator()(std::int64_t) const noexcept { return sizeof(std::int64_t); }
    constexpr std::size_t operator()(std::string_view sv) const noexcept {
        return sv.length() + 1 + 4;  // Length + null terminator + size prefix
    }
    std::size_t operator()(bson::view v) const noexcept { return v.byte_size(); }
    std::size_t operator()(bson_array_view r) const noexcept { return r.byte_size(); }

    std::size_t operator()(bson_value_ref val) const noexcept { return val.visit(*this); }
} value_byte_size;

// Match an object that can be appending to a document as the value of some element
template <typename T>
concept value_rule = requires(mutator& doc, std::string_view key, const T& obj) {
    append_value(doc, key, obj);
    value_byte_size(obj);
};

template <value_rule T>
int requires_appendable_value();

template <typename... Ts>
concept all_value_rules = requires { (requires_appendable_value<Ts>(), ...); };

// Append a value to a document with an integer key
void append_nth_value(mutator& doc, std::size_t nth, const value_rule auto& value) {
    auto buf = ::bson_tmp_uint_string(static_cast<std::uint32_t>(nth));
    append_value(doc, std::string_view(buf.buf), value);
}

/**
 * @brief Match a type that can be appended to a document using `append_to`. Also requires a
 * `byte_size()`
 */
template <typename T>
concept element_rule  //
    = requires(mutator& doc, const T& obj) {
          obj.append_to(doc);
          obj.byte_size();
      };

template <element_rule T>
int requires_element_rule();

// This concept is for cleaning up ugly diagnostics from GCC, which doesn't disect constraint
// expressions that are fold-expressions. This guarantees that GCC's diagnostic will actually
// emit a message pointing to the exact build rule operand that generates the problem.
template <typename... Ts>
concept all_element_rules = requires { (requires_element_rule<Ts>(), ...); };

template <typename T>
concept contextual_bool_convertible = requires(const T& val) { val ? 0 : 0; };

/**
 * @brief A builder combinator that takes an optional of an appendable element
 *
 * If the optional has a value, the element will be appended. Otherwise, this
 * builder will append nothing to the document
 */
template <typename Opt>
    requires contextual_bool_convertible<Opt> and requires(const Opt& opt) {
        { *opt } -> element_rule;
    }
struct conditional {
    Opt _opt;

    constexpr std::size_t byte_size() const noexcept {
        if (not _opt) {
            // No element will be appended
            return 0;
        }
        return (*_opt).byte_size();
    }

    void append_to(mutator& doc) const {
        if (_opt) {
            (*_opt).append_to(doc);
        }
    }
};

template <typename O>
explicit conditional(O&&) -> conditional<O>;

// Count the number of decimal digits in a size_t value
constexpr std::size_t _ndigits(std::size_t nth) noexcept {
    if (nth == 0) {
        return 1;
    }
    std::size_t ret = 0;
    while (nth > 0) {
        ++ret;
        nth /= 10;
    }
    return ret;
}

/**
 * @brief Create an auto-numbered BSON array value with the given elements
 */
template <typename... Vs>
struct array {
    using iseq = std::index_sequence_for<Vs...>;

    static_assert(
        all_value_rules<Vs...>,
        "One or more of the objects given to bson::make::array<> does not satisfy the "
        "requriements of being either an insertible value or having a value_rule interface");

    array() = default;
    constexpr explicit array(Vs&&... vs)
        requires(sizeof...(vs) != 0)
        : _elements(mlib_fwd(vs)...) {}

    constexpr std::size_t byte_size() const noexcept { return this->_size(iseq{}); }

    void append_to(mutator& out, std::string_view key) const {
        auto child = out.push_array(key);
        _append_each(child, iseq{});
    }

private:
    std::tuple<Vs...> _elements;

    template <std::size_t... Ns>
    constexpr std::size_t _size(std::index_sequence<Ns...>) const noexcept {
        return ((_ndigits(Ns) + 1 + 1) + ... + 0)
            + (value_byte_size(std::get<Ns>(_elements)) + ... + 0);
    }

    template <std::size_t... Ns>
    void _append_each(mutator& out, std::index_sequence<Ns...>) const {
        (append_nth_value(out, Ns, std::get<Ns>(_elements)), ...);
    }
};

template <typename... Vs>
array(Vs&&...) -> array<Vs...>;

/**
 * @brief Create a BSON array element using the given range of appendable objects to fill
 * the array.
 */
template <std::ranges::forward_range R>
    requires value_rule<std::ranges::range_reference_t<R>>
class range {
public:
    range() = default;
    explicit constexpr range(R&& r) noexcept
        : _range(mlib_fwd(r)) {}

    constexpr std::size_t byte_size() const noexcept {
        std::size_t acc = 0;
        std::size_t idx = 0;
        for (const auto& el : _range) {
            acc += _ndigits(idx) + 1 + 1;
            acc += value_byte_size(el);
            ++idx;
        }
        return acc;
    }

    void append_to(mutator& out, std::string_view key) const {
        auto        child = out.push_array(key);
        std::size_t nth   = 0;
        for (value_rule auto const& el : _range) {
            append_nth_value(child, nth, el);
            ++nth;
        }
    }

private:
    R _range;
};

template <typename R>
explicit range(R&&) -> range<R>;

/**
 * @brief Create a BSON document using the given elements
 */
template <typename... Els>
struct doc : doc<std::index_sequence_for<Els...>, Els...> {
    using doc<std::index_sequence_for<Els...>, Els...>::doc;
};

template <std::size_t N, typename Elem>
struct doc_element_part {
    static_assert(element_rule<Elem>, "The given doc<> element is not a valid element_rule");
    Elem element;
};

template <std::size_t... Is, typename... Els>
struct doc<std::index_sequence<Is...>, Els...> : doc_element_part<Is, Els>... {
    doc() = default;
    constexpr explicit doc(Els&&... els) noexcept
        requires(sizeof...(els) != 0)
        : doc_element_part<Is, Els>(mlib_fwd(els))... {}

    bson::document build(mlib::allocator<> a) const {
        bson::document ret{a, this->byte_size()};
        bson::mutator  mut{ret};
        ((this->doc_element_part<Is, Els>::element.append_to(mut)), ...);
        return ret;
    }

    void append_to(mutator& into, std::string_view key) const {
        auto child [[maybe_unused]] = into.push_subdoc(key);
        ((this->doc_element_part<Is, Els>::element.append_to(child)), ...);
    }

    constexpr std::size_t byte_size() const noexcept {
        // Size of all elements plus document header/trailer
        return ((this->doc_element_part<Is, Els>::element.byte_size()) + ... + 0);
    }
};

template <typename... Ps>
explicit doc(Ps&&...) -> doc<Ps...>;

template <typename Val>
struct pair {
    std::string_view key;
    Val              value;

    static_assert(value_rule<Val>,
                  "The operand of pair<> must be an insertbile value or implement the "
                  "value_rule interface");

    constexpr std::size_t byte_size() const noexcept {
        return 1 + key.length() + 1 + value_byte_size(this->value);
    }

    void append_to(mutator& into) const { append_value(into, this->key, this->value); }
};

template <typename T>
explicit pair(std::string_view, T) -> pair<T>;

// Coerce string-view-like values to string_views, preventing copies and normalizing
// template instantiations
template <std::convertible_to<std::string_view> T>
explicit pair(std::string_view, T) -> pair<std::string_view>;

/**
 * @brief A special optional-pair element type that only appends an element
 * if the corresponding value is a truthy value.
 *
 * The value type must be contexturally convertible to `bool`
 */
template <typename Val>
struct optional_pair {
    std::string_view key;
    Val              opt;

    static_assert(value_rule<Val>,
                  "The operand of optional_pair must be an insertbile value or implement the "
                  "value_rule interface");
    static_assert(contextual_bool_convertible<Val>,
                  "The value type of an optional_pair must be contextually-convertible to `bool`");

    constexpr std::size_t byte_size() const noexcept {
        if (this->opt) {
            return 1 + key.length() + 1 + value_byte_size(this->opt);
        }
        return 0;
    }

    void append_to(mutator& into) const {
        if (not this->opt) {
            return;
        }
        append_value(into, this->key, opt);
    }
};

template <contextual_bool_convertible Val>
    requires requires(const Val v) {
        { *v } -> value_rule;
    }
struct optional_pair<Val> {
    std::string_view key;
    Val              opt;

    constexpr std::size_t byte_size() const noexcept {
        if (this->opt) {
            return 1 + key.length() + 1 + value_byte_size(*this->opt);
        }
        return 0;
    }

    void append_to(mutator& into) const {
        if (not this->opt) {
            return;
        }
        append_value(into, this->key, *opt);
    }
};

template <typename O>
explicit optional_pair(std::string_view, O) -> optional_pair<O>;

/**
 * @brief A special value type that will only append to a document/array if the
 * value is truthy.
 *
 * The value type must be contexturally convertible to `bool`
 *
 * If the value is an optional<>, then the wrapped value type will be used
 */
template <typename Val>
struct optional_value {
    Val opt;

    static_assert(value_rule<Val>,
                  "The operand of optional_value must be an insertbile value or implement the "
                  "value_rule interface, or must be an optional/pointer-to such a type");
    static_assert(contextual_bool_convertible<Val>,
                  "The value type of an optional_value must be contextually-convertible to `bool`");

    constexpr std::size_t byte_size() const noexcept {
        if (this->opt) {
            return value_byte_size(this->opt);
        }
        return 0;
    }

    void append_to(mutator& into, std::string_view key) const {
        if (not this->opt) {
            return;
        }
        append_value(into, key, opt);
    }
};

// Specalization for optional-like types, including pointers
template <typename Val>
    requires requires(const Val& v) {
        { *v } -> value_rule;
    }
struct optional_value<Val> {
    Val opt;

    static_assert(contextual_bool_convertible<Val>,
                  "The value type of an optional_value must be contextually-convertible to `bool`");

    constexpr std::size_t byte_size() const noexcept {
        if (this->opt) {
            return value_byte_size(*this->opt);
        }
        return 0;
    }

    void append_to(mutator& into, std::string_view key) const {
        if (not this->opt) {
            return;
        }
        append_value(into, key, *opt);
    }
};

template <typename O>
explicit optional_value(O&&) -> optional_value<O>;

//* extern templates for common situations. Aides in compile time and object size.
//* Keep in sync with the explicit instantiations in make.cpp
// Common case: An empty document
extern template struct doc<>;
extern template struct doc<std::index_sequence<>>;

#define X(_nth, ViewType, _owner, _name, _safename)                                                \
    extern template void append_value(mutator&, std::string_view, ViewType const&);                \
    extern template struct pair<ViewType>;                                                         \
    BSON_TYPE_X_LIST
extern template struct pair<bson_value_ref>;
extern template struct pair<std::string_view>;
extern template void append_value(mutator&, std::string_view, std::string_view const&);
extern template void append_value(mutator&, std::string_view, bson_value_ref const&);
#undef X

extern template struct optional_pair<bson_value_ref>;
extern template struct optional_pair<std::int32_t>;
extern template struct optional_pair<std::int64_t>;
extern template struct optional_pair<bool>;
extern template struct optional_pair<mlib_str_view>;
extern template struct optional_pair<bson_view>;

}  // namespace bson::make
