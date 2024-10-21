#pragma once

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
