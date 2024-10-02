#pragma once

#include <bson/doc.h>
#include <bson/mut.h>

#include <concepts>
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

    constexpr std::size_t operator()(double) const noexcept { return sizeof(double); }
    constexpr std::size_t operator()(bool) const noexcept { return 1; }
    constexpr std::size_t operator()(std::int32_t) const noexcept { return sizeof(std::int32_t); }
    constexpr std::size_t operator()(std::int64_t) const noexcept { return sizeof(std::int64_t); }
    constexpr std::size_t operator()(::bson_oid) const noexcept { return sizeof(::bson_oid); }
    constexpr std::size_t operator()(std::string_view sv) const noexcept {
        return sv.length() + 1 + 4;  // Length + null terminator + size prefix
    }
    constexpr std::size_t operator()(null_t) const noexcept { return 0; }
    constexpr std::size_t operator()(bson_view v) const noexcept { return v.byte_size(); }
    constexpr std::size_t operator()(undefined_t) const noexcept { return 0; }
} value_byte_size;

// Match an object that can be appending to a document as the value of some element
template <typename T>
concept appendable_value = requires(mutator& doc, std::string_view key, const T& obj) {
    append_value(doc, key, obj);
    value_byte_size(obj);
};

// Append a value to a document with an integer key
void append_value(mutator& doc, std::size_t nth, const appendable_value auto& value) {
    auto buf = ::bson_tmp_uint_string(static_cast<std::uint32_t>(nth));
    append_value(doc, std::string_view(buf.buf), value);
}

/**
 * @brief Match a type that presents a pair-like interface that can be appended to
 * a document
 */
template <typename T>
concept pairlike_element = requires(const T& obj) {
    // The first element should be convertible to a string view
    { std::get<0>(obj) } -> std::convertible_to<std::string_view>;
    { std::get<1>(obj) } -> appendable_value;
};

/**
 * @brief Match a type that can be appended to a document using `append_to`. Also requires a
 * `byte_size()`
 */
template <typename T>
concept indirect_appendable_element = requires(mutator& out, const T& obj) {
    obj.append_to(out);
    { obj.byte_size() };
};

// Append a pair directly to a document
template <pairlike_element T>
void append_element(mutator& out, const T& pair) {
    decltype(auto) key   = std::get<0>(pair);
    decltype(auto) value = std::get<1>(pair);
    append_value(out, key, value);
}

// Append an element to a document in using its custom append_to method
template <indirect_appendable_element T>
void append_element(mutator& out, const T& elem) {
    elem.append_to(out);
}

// Obtain the size of an element as a number of bytes
inline constexpr struct element_byte_size_fn {
    template <indirect_appendable_element E>
    constexpr std::size_t operator()(const E& el) const {
        return el.byte_size();
    }

    template <pairlike_element P>
    constexpr std::size_t operator()(const P& pair) const {
        decltype(auto) key = std::get<0>(pair);
        return 1                                  // Type tag
            + std::string_view(key).length() + 1  // key and null terminator
            + value_byte_size(std::get<1>(pair));
    }

} element_byte_size;

// Match a type that can be appended to a document, providing its own element key
template <typename T>
concept appendable_element = requires(mutator& doc, const T& obj) {
    append_element(doc, obj);
    element_byte_size(obj);
};

/**
 * @brief A builder combinator that takes an optional of an appendable element
 *
 * If the optional has a value, the element will be appended. Otherwise, this
 * builder will append nothing to the document
 */
template <typename Opt>
    requires requires(const Opt& opt) {
        opt ? 1 : 1;  // Check for bool conversion
        { *opt } -> appendable_element;
    }
struct conditional {
    Opt _opt;

    constexpr std::size_t byte_size() const noexcept {
        if (not _opt) {
            // No element will be appended
            return 0;
        }
        return element_byte_size(*_opt);
    }

    void append_to(mutator& doc) const {
        if (_opt) {
            append_element(doc, *_opt);
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
template <appendable_value... Vs>
struct array {
    using iseq = std::index_sequence_for<Vs...>;

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
        (append_value(out, Ns, std::get<Ns>(_elements)), ...);
    }
};

template <typename... Vs>
array(Vs&&...) -> array<Vs...>;

/**
 * @brief Create a BSON array element using the given range of appendable objects to fill
 * the array.
 */
template <std::ranges::forward_range R>
    requires appendable_value<std::ranges::range_reference_t<R>>
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
        for (appendable_value auto const& el : _range) {
            append_value(child, nth, el);
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
template <appendable_element... Els>
struct doc {
    using iseq = std::index_sequence_for<Els...>;

    doc() = default;
    constexpr explicit doc(Els&&... els) noexcept
        requires(sizeof...(els) != 0)
        : tpl(mlib_fwd(els)...) {}

    bson::document build(mlib::allocator<> a) const {
        bson::document ret{a, this->byte_size()};
        bson::mutator  mut{ret};
        this->_build(mut, iseq{});
        return ret;
    }

    void append_to(mutator& into, std::string_view key) const {
        auto child = into.push_subdoc(key);
        this->_build(child, iseq{});
    }

    constexpr std::size_t byte_size() const noexcept {
        // Size of all elements plus document header/trailer
        return this->_size(iseq{}) + 5;
    }

private:
    template <std::size_t... Ns>
    constexpr std::size_t _size(std::index_sequence<Ns...>) const noexcept {
        return (element_byte_size(std::get<Ns>(tpl)) + ... + 0);
    }
    template <std::size_t... Ns>
    void _build(mutator& out, std::index_sequence<Ns...>) const {
        (append_element(out, std::get<Ns>(tpl)), ...);
    }

    std::tuple<Els...> tpl;
};

template <appendable_element... Ps>
explicit doc(Ps&&...) -> doc<Ps...>;

}  // namespace bson::make
