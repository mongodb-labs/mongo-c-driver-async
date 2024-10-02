#pragma once

#include "./types.h"

#include <bson/byte.h>
#include <bson/detail/assert.h>
#include <bson/detail/mem.h>
#include <bson/iterator.h>

#include <mlib/config.h>
#include <mlib/cstring.h>
#include <mlib/integer.h>

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#if mlib_is_cxx()
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <string_view>
#endif

mlib_extern_c_begin();

typedef struct bson_view bson_view;

/**
 * @brief A type specifically representing a nullable read-only view of a BSON
 * document.
 */
typedef struct bson_view {
    /**
     * @private
     * @brief Pointer to the beginning of the referenced BSON document, or NULL.
     *
     * If NULL, the view refers to nothing.
     */
    const bson_byte* _bson_document_data;

#if mlib_is_cxx()
    /**
     * @brief Alias of @ref bson_iterator
     */
    using iterator = bson_iterator;

    using size_type = std::uint32_t;

    /**
     * @brief Equivalent to @ref bson_begin()
     */
    [[nodiscard]] constexpr iterator begin() const noexcept { return ::bson_begin(*this); }

    /**
     * @brief Equivalent to @ref bson_end()
     */
    [[nodiscard]] constexpr iterator end() const noexcept { return ::bson_end(*this); }

    /**
     * @brief Check whether this view is non-null
     *
     * @return true If the view refers to a document
     * @return false If the view is null
     */
    constexpr explicit operator bool() const noexcept { return has_value(); }

    // Return a pointer to the document data
    [[nodiscard]] constexpr const bson_byte* data() const noexcept { return ::bson_data(*this); }

    /**
     * @brief Determine whether the view is null or refers to an empty document.
     */
    [[nodiscard]] constexpr bool empty() const noexcept {
        return not has_value() or byte_size() == 5;
    }

    /**
     * @brief Obtain the size of the document data, in bytes
     */
    [[nodiscard]] constexpr size_type byte_size() const noexcept { return ::bson_size(*this); }

    /**
     * @brief Determine whether this view is non-null
     *
     * @return true If the view refers to a document
     * @return false If the view is null
     */
    [[nodiscard]] constexpr bool has_value() const noexcept { return data() != nullptr; }

    /**
     * @brief Obtain an iterator referring to the first element with the
     * specified key.
     *
     * @param key The key to search for
     * @return iterator An iterator referring to the found element, or the end
     * iterator, or an errant iterator
     */
    [[nodiscard]] constexpr iterator find(std::string_view key) const noexcept {
        return ::bson_find(*this, key);
    }

    /**
     * @brief Construct a view from the given input data
     *
     * @param b Pointer to the beginning of a buffer
     * @param datalen The addressable length beginning at `b`
     * @return bson_view A non-null view
     *
     * @throws bson_view_error if parsing the data buffer results in an error.
     *
     * @note Use @ref bson_view_from_data() for a non-throwing equivalent API.
     */
    [[nodiscard]] static constexpr bson_view from_data(const bson_byte* b, size_t datalen);
#endif
} bson_view;

/// A "null" constant expression for bson_view objects
#define BSON_VIEW_NULL (mlib_init(bson_view){NULL})

/**
 * @brief The reason that we may have failed to create a bson_view object in
 */
enum bson_view_errc {
    /**
     * @brief There is no error creating the view, and the view is ready.
     * Equivalent to zero.
     */
    bson_view_errc_okay,
    /**
     * @brief The given data buffer is too short to possibly hold the
     * document.
     */
    bson_view_errc_short_read,
    /**
     * @brief The document header declares an invalid length.
     */
    bson_view_errc_invalid_header,
    /**
     * @brief The document does not have a null terminator
     */
    bson_view_errc_invalid_terminator,
};

/**
 * @brief View the given pointed-to data as a BSON document.
 */
mlib_constexpr bson_view bson_view_from_data(const bson_byte* const data,
                                             const size_t           data_len,
                                             enum bson_view_errc*   error) mlib_noexcept {
    bson_view           ret = BSON_VIEW_NULL;
    int32_t             len = 0;
    enum bson_view_errc err_sink;
    if (!error) {
        error = &err_sink;
    }
    // All BSON data must be at least five bytes long
    if (data_len < 5) {
        *error = bson_view_errc_short_read;
        return ret;
    }
    // Read the length header. This includes the header's four bytes, the
    // document's element data, and the null terminator byte.
    len = (int32_t)_bson_read_u32le(data);
    // Check that the size is in-bounds
    if (len < 5) {
        *error = bson_view_errc_invalid_header;
    }
    // Check that the buffer is large enough to hold expected the document
    else if ((uint32_t)len > data_len) {
        // Not enough data to do the read
        *error = bson_view_errc_short_read;
    }
    // The document must have a zero-byte at the end.
    else if (data[len - 1].v != 0) {
        *error = bson_view_errc_invalid_terminator;
    }
    // Okay!
    else {
        *error                  = bson_view_errc_okay;
        ret._bson_document_data = data;
    }
    return ret;
}

/**
 * @brief Obtain a bson_view for the given document-like entity.
 */
#define bson_as_view(...) _bson_view_from_ptr(bson_data(__VA_ARGS__))
mlib_constexpr bson_view _bson_view_from_ptr(const bson_byte* p) mlib_noexcept {
    if (!p) {
        return BSON_VIEW_NULL;
    }
    int32_t len = (int32_t)_bson_read_u32le(p);
    BV_ASSERT(len >= 0);
    return bson_view_from_data(p, (uint32_t)len, NULL);
}

/**
 * @brief Obtain a view of the BSON document referred to by the given iterator.
 *
 * If the iterator does not refer to a document/array element, returns @ref
 * BSON_VIEW_NULL
 *
 * @param it A valid iterator referring to an element of a bson_view
 * @return bson_view A view derived from the bson_view being iterated with `it`.
 */
mlib_constexpr bson_view bson_iterator_document(bson_iterator        it,
                                                enum bson_view_errc* error) mlib_noexcept {
    BV_ASSERT(!bson_stop(it));
    if (bson_iterator_type(it) != bson_type_document && bson_iterator_type(it) != bson_type_array) {
        return BSON_VIEW_NULL;
    }
    const bson_byte* const valptr     = _bson_iterator_value_ptr(it);
    const int64_t          val_offset = (valptr - it._ptr);
    const int64_t          val_remain = it._rlen - val_offset;
    BV_ASSERT(val_remain > 0);
    return bson_view_from_data(valptr, (size_t)val_remain, error);
}

mlib_extern_c_end();

#if mlib_is_cxx()
namespace bson {

inline constexpr struct undefined_t {
} undefined;
inline constexpr struct null_t {
} null;

inline constexpr struct minkey_t {
} minkey;
inline constexpr struct maxkey_t {
} maxkey;

using view = ::bson_view;

}  // namespace bson

struct bson_view_error : std::runtime_error {
    bson_view_error() noexcept
        : std::runtime_error("Invalid BSON document data") {}

    [[nodiscard]] virtual bson_view_errc reason() const noexcept = 0;
};

template <bson_view_errc R>
struct bson_view_error_of : bson_view_error {
    [[nodiscard]] bson_view_errc reason() const noexcept { return R; }
};

class bson_iterator::reference {
    bson_iterator _iter;
    friend ::bson_iterator;

    constexpr explicit reference(bson_iterator it)
        : _iter(it) {}

public:
    [[nodiscard]] constexpr bson_type type() const noexcept { return bson_iterator_type(_iter); }

    [[nodiscard]] std::string_view key() const noexcept {
        return std::string_view(bson_key(_iter));
    }
    [[nodiscard]] constexpr double double_() const noexcept { return bson_iterator_double(_iter); }
    [[nodiscard]] std::string_view utf8() const noexcept { return bson_iterator_utf8(_iter); }
    [[nodiscard]] constexpr bson_view document() const noexcept {
        return bson_iterator_document(_iter, NULL);
    }
    [[nodiscard]] constexpr bson_view document(enum bson_view_errc& err) const noexcept {
        return bson_iterator_document(_iter, &err);
    }
    [[nodiscard]] constexpr bson_binary binary() const noexcept {
        return bson_iterator_binary(_iter);
    }
    [[nodiscard]] constexpr bson_oid oid() const noexcept { return bson_iterator_oid(_iter); }
    [[nodiscard]] constexpr bool     bool_() const noexcept { return bson_iterator_bool(_iter); }
    [[nodiscard]] constexpr bson_datetime datetime() const noexcept {
        return bson_iterator_datetime(_iter);
    }
    [[nodiscard]] bson_regex regex() const noexcept { return bson_iterator_regex(_iter); }
    [[nodiscard]] constexpr bson_dbpointer dbpointer() const noexcept {
        return bson_iterator_dbpointer(_iter);
    }
    [[nodiscard]] bson_code   code() const noexcept { return bson_iterator_code(_iter); }
    [[nodiscard]] bson_symbol symbol() const noexcept { return bson_iterator_symbol(_iter); }
    [[nodiscard]] constexpr std::int32_t int32() const noexcept {
        return bson_iterator_int32(_iter);
    }
    [[nodiscard]] constexpr bson_timestamp timestamp() const noexcept {
        return bson_iterator_timestamp(_iter);
    }
    [[nodiscard]] constexpr std::int64_t int64() const noexcept {
        return bson_iterator_int64(_iter);
    }
    [[nodiscard]] constexpr bson_decimal128 decimal128() const noexcept {
        return ::bson_iterator_decimal128(_iter);
    }

    // Coerce the referred-to element to a double float value
    [[nodiscard]] constexpr double as_double(bool* okay = nullptr) const noexcept {
        return bson_iterator_as_double(_iter, okay);
    }
    // Coerce the referred-to element to a boolean value
    [[nodiscard]] constexpr bool as_bool() const noexcept { return bson_iterator_as_bool(_iter); }
    // Coerce the referred-to element to an int32 value
    [[nodiscard]] constexpr std::int32_t as_int32(bool* okay = nullptr) const noexcept {
        return bson_iterator_as_int32(_iter, okay);
    }
    // Coerce the referred-to element to an int64 value
    [[nodiscard]] constexpr std::int64_t as_int64(bool* okay = nullptr) const noexcept {
        return bson_iterator_as_int64(_iter, okay);
    }

    template <typename F>
    constexpr auto visit(F&& fn) const -> decltype(static_cast<F&&>(fn)(std::string_view())) {
        auto call = [&](auto n) { return static_cast<F&&>(fn)(n); };
        switch (type()) {
        case bson_type_eod:
        default:
            return call(bson::null);  // What should this actually do?
        case bson_type_double:
            return call(double_());
        case bson_type_utf8:
            return call(utf8());
        case bson_type_document:
        case bson_type_array:
            return call(document());
        case bson_type_binary:
            return call(binary());
        case bson_type_undefined:
            return call(bson::undefined);
        case bson_type_oid:
            return call(oid());
        case bson_type_bool:
            return call(bool_());
        case bson_type_date_time:
            return call(datetime());
        case bson_type_null:
            return call(bson::null);
        case bson_type_regex:
            return call(regex());
        case bson_type_dbpointer:
            return call(dbpointer());
        case bson_type_code:
            return call(code());
        case bson_type_symbol:
            return call(symbol());
        case bson_type_codewscope:
            return call(code());
        case bson_type_int32:
            return call(int32());
        case bson_type_timestamp:
            return call(timestamp());
        case bson_type_int64:
            return call(int64());
        case bson_type_decimal128:
            return call(decimal128());
        case bson_type_maxkey:
            return call(bson::maxkey);
        case bson_type_minkey:
            return call(bson::minkey);
            break;
        }
    }

#define TRY_AS(Type, Getter, TypeTag)                                                              \
    friend std::optional<Type> bson_value_try_convert(reference const& self, Type*) noexcept {     \
        if (self.type() == TypeTag) {                                                              \
            return Getter;                                                                         \
        }                                                                                          \
        return {};                                                                                 \
    }                                                                                              \
    static_assert(true, "")
    TRY_AS(double, self.double_(), bson_type_double);
    TRY_AS(std::string_view, self.utf8(), bson_type_utf8);
    TRY_AS(bson_binary, self.binary(), bson_type_binary);
    TRY_AS(bson::undefined_t, bson::undefined, bson_type_undefined);
    TRY_AS(bson_oid, self.oid(), bson_type_oid);
    TRY_AS(bool, self.bool_(), bson_type_bool);
    TRY_AS(bson_datetime, self.datetime(), bson_type_date_time);
    TRY_AS(bson::null_t, bson::null, bson_type_null);
    TRY_AS(bson_regex, self.regex(), bson_type_regex);
    TRY_AS(::bson_dbpointer, self.dbpointer(), bson_type_dbpointer);
    TRY_AS(bson_code, self.code(), bson_type_code);
    TRY_AS(bson_symbol, self.symbol(), bson_type_symbol);
    // TRY_AS(std::string_view, self.code(), BSON_TYPE_CODEWSCOPE); // TODO
    TRY_AS(std::int32_t, self.int32(), bson_type_int32);
    TRY_AS(bson_timestamp, self.timestamp(), bson_type_timestamp);
    TRY_AS(std::int64_t, self.int64(), bson_type_int64);
    TRY_AS(bson_decimal128, self.decimal128(), bson_type_decimal128);
    TRY_AS(bson::minkey_t, bson::minkey, bson_type_minkey);
    TRY_AS(bson::maxkey_t, bson::maxkey, bson_type_maxkey);
#undef TRY_AS

    friend std::optional<bson_view> bson_value_try_convert(reference const& self,
                                                           bson_view*) noexcept {
        if (self.type() != bson_type_document and self.type() != bson_type_array) {
            return {};
        }
        return self.document();
    }

    template <typename T>
    [[nodiscard]] constexpr std::optional<T> try_as() const noexcept {
        return bson_value_try_convert(*this, static_cast<T*>(nullptr));
    }
};

class bson_iterator::arrow {
public:
    bson_iterator::reference _ref;

    constexpr const bson_iterator::reference* operator->() const noexcept { return &_ref; }
};

constexpr bson_iterator::reference bson_iterator::operator*() const {
    throw_if_error();
    return reference(*this);
}

constexpr bson_iterator::arrow bson_iterator::operator->() const noexcept { return arrow{**this}; }

constexpr bson_view bson_view::from_data(const bson_byte* b, size_t datalen) {
    bson_view_errc error;
    auto           v = bson_view_from_data(b, datalen, &error);
    switch (error) {
    case bson_view_errc_okay:
        return v;
#define X(Reason)                                                                                  \
    case Reason:                                                                                   \
        throw bson_view_error_of<Reason>()
        X(bson_view_errc_short_read);
        X(bson_view_errc_invalid_header);
        X(bson_view_errc_invalid_terminator);
    }
#undef X
    __builtin_unreachable();
}
#endif
