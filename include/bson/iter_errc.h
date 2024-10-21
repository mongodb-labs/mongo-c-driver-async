#pragma once

/**
 * @brief Reasons that advancing a bson_iterator may fail.
 */
enum bson_iter_errc {
    /**
     * @brief No error occurred (this value is equal to zero)
     */
    bson_iter_errc_okay = 0,
    /**
     * @brief There is not enough data in the buffer to find the next element.
     */
    bson_iter_errc_short_read = 1,
    /**
     * @brief The type tag on the element is not a recognized value.
     */
    bson_iter_errc_invalid_type,
    /**
     * @brief The element has an encoded length, but the length is too large for
     * the remaining buffer.
     */
    bson_iter_errc_invalid_length,
    /**
     * @brief The pointed-to subdocument/array element is missing its null terminator
     */
    bson_iter_errc_invalid_document,
};
