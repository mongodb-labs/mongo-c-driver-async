#pragma once

#include <bson/byte.h>
#include <bson/detail/assert.h>
#include <bson/detail/mem.h>
#include <bson/view_errc.h>

#if mlib_is_cxx()
#include <stdexcept>
#include <string_view>

#if mlib_have_cxx20()
#include <span>
#endif
#endif

typedef struct bson_view bson_view;
struct bson_iterator;

/**
 * @brief Obtain a pointer to the beginning of the data for the given document.
 */
#define bson_data(X) _bson_data_as_const((X)._bson_document_data)

/**
 * @brief Obtain a mutable pointer to the beginning of the data for the given
 * document.
 *
 * @note This only works if the object is a mutable document reference (e.g. not
 * bson_view)
 */
#define bson_mut_data(X) _bson_data_as_mut((X)._bson_document_data)

/**
 * @brief Obtain the byte-size of the BSON document referred to by the given
 * bson_view.
 */
#define bson_size(...) _bson_byte_size(bson_data((__VA_ARGS__)))

/**
 * @brief Obtain the byte-size of the given BSON document, as a signed integer
 */
#define bson_ssize(...) _bson_byte_ssize(bson_data((__VA_ARGS__)))

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

    // Return a pointer to the document data
    [[nodiscard]] inline const bson_byte* data() const noexcept { return ::bson_data(*this); }

    /**
     * @brief Obtain the size of the document data, in bytes
     */
    [[nodiscard]] inline size_type byte_size() const noexcept { return ::bson_size(*this); }

#if mlib_have_cxx20()
    /**
     * @brief Obtain a span over the bytes in the document
     */
    [[nodiscard]] std::span<const bson_byte> bytes() const noexcept;
#endif
    /**
     * @brief Equivalent to @ref bson_begin()
     * @include <bson/iterator.h>
     */
    [[nodiscard]] inline iterator begin() const noexcept;

    /**
     * @brief Equivalent to @ref bson_end()
     * @include <bson/iterator.h>
     */
    [[nodiscard]] inline iterator end() const noexcept;

    /**
     * @brief Determine whether this view is non-null
     *
     * @return true If the view refers to a document
     * @return false If the view is null
     */
    [[nodiscard]] inline bool has_value() const noexcept { return data() != nullptr; }

    /**
     * @brief Check whether this view is non-null
     *
     * @return true If the view refers to a document
     * @return false If the view is null
     */
    inline explicit operator bool() const noexcept { return has_value(); }

    /**
     * @brief Determine whether the view is null or refers to an empty document.
     */
    [[nodiscard]] inline bool empty() const noexcept {
        // An empty BSON document has exactly five bytes
        return not has_value() or byte_size() == 5;
    }

    /**
     * @brief Obtain an iterator referring to the first element with the
     * specified key.
     *
     * @param key The key to search for
     * @return iterator An iterator referring to the found element, or the end
     * iterator, or an errant iterator
     *
     * @include <bson/iterator.h>
     */
    [[nodiscard]] inline iterator find(std::string_view key) const noexcept;

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
    [[nodiscard]] static inline bson_view from_data(const bson_byte* b, size_t datalen);

    bool operator==(bson_view) const noexcept;
#endif
} bson_view;

/**
 * @brief The same as a `bson_view`, but explicitly refers to an array-style document
 */
typedef struct bson_array_view {
    const bson_byte* _bson_document_data;

#if mlib_is_cxx()
    /**
     * @brief Alias of @ref bson_iterator
     */
    using iterator = bson_iterator;

    using size_type = std::uint32_t;

    /**
     * @brief Explicit-convert to a `bson_view`
     */
    inline explicit operator bson_view() const noexcept {
        bson_view ret;
        ret._bson_document_data = this->_bson_document_data;
        return ret;
    }

    // Return a pointer to the array data
    [[nodiscard]] inline const bson_byte* data() const noexcept { return ::bson_data(*this); }

    /**
     * @brief Obtain the size of the array data, in bytes
     */
    [[nodiscard]] inline size_type byte_size() const noexcept { return ::bson_size(*this); }

#if mlib_have_cxx20()
    [[nodiscard]] std::span<const bson_byte> bytes() const noexcept;
#endif

    /**
     * @brief Equivalent to @ref bson_begin()
     * @include <bson/iterator.h>
     */
    [[nodiscard]] inline iterator begin() const noexcept;

    /**
     * @brief Equivalent to @ref bson_end()
     * @include <bson/iterator.h>
     */
    [[nodiscard]] inline iterator end() const noexcept;

    /**
     * @brief Determine whether this view is non-null
     *
     * @return true If the view refers to an array
     * @return false If the view is null
     */
    [[nodiscard]] inline bool has_value() const noexcept { return data() != nullptr; }

    /**
     * @brief Check whether this view is non-null
     *
     * @return true If the view refers to an array
     * @return false If the view is null
     */
    inline explicit operator bool() const noexcept { return has_value(); }

    /**
     * @brief Determine whether the view is null or refers to an empty array.
     */
    [[nodiscard]] inline bool empty() const noexcept {
        // An empty BSON array has exactly five bytes
        return not has_value() or byte_size() == 5;
    }

    inline bool operator==(bson_array_view other) const noexcept {
        return bson_view(*this) == bson_view(other);
    }
#endif
} bson_array_view;

mlib_extern_c_begin();

inline const bson_view* _bsonViewNullInst() mlib_noexcept {
    static const bson_view null = {0};
    return &null;
}

inline const bson_array_view* _bsonArrayViewNullInst() mlib_noexcept {
    static const bson_array_view null = {0};
    return &null;
}

#define bson_view_null mlib_parenthesized_expression(*_bsonViewNullInst())
#define bson_array_view_null mlib_parenthesized_expression(*_bsonArrayViewNullInst())

/**
 * @brief View the given pointed-to data as a BSON document.
 */
inline bson_view bson_view_from_data(const bson_byte* const data,
                                     const size_t           data_len,
                                     enum bson_view_errc*   error) mlib_noexcept {
    bson_view           ret = {0};
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
inline bson_view _bson_view_from_ptr(const bson_byte* p) mlib_noexcept {
    if (!p) {
        return bson_view_null;
    }
    int32_t len = (int32_t)_bson_read_u32le(p);
    BV_ASSERT(len >= 0);
    return bson_view_from_data(p, (uint32_t)len, NULL);
}

mlib_extern_c_end();

#if mlib_is_cxx()

namespace bson {

using view = ::bson_view;

struct view_error : std::runtime_error {
    view_error() noexcept
        : std::runtime_error("Invalid BSON document data") {}

    [[nodiscard]] virtual bson_view_errc reason() const noexcept = 0;
};

template <bson_view_errc R>
struct view_error_of : view_error {
    [[nodiscard]] bson_view_errc reason() const noexcept { return R; }
};

}  // namespace bson

inline bson_view bson_view::from_data(const bson_byte* b, size_t datalen) {
    bson_view_errc error;
    auto           v = bson_view_from_data(b, datalen, &error);
    switch (error) {
    case bson_view_errc_okay:
        return v;
#define X(Reason)                                                                                  \
    case Reason:                                                                                   \
        throw bson::view_error_of<Reason>()
        X(bson_view_errc_short_read);
        X(bson_view_errc_invalid_header);
        X(bson_view_errc_invalid_terminator);
    }
#undef X
    __builtin_unreachable();
}

#endif  // C++
