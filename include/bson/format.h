#pragma once

#include <bson/view.h>

#include <mlib/config.h>
#include <mlib/stream.h>

mlib_extern_c_begin();

/**
 * @brief Options to control the output of BSON data
 */
typedef struct bson_fmt_options {
    // The initial indent to be added to the output
    unsigned initial_indent;
    // Base indentation to be added at each newline (except the initial)
    unsigned subsequent_indent;
    // Additional indentation to be added for each nested object
    unsigned nested_indent;
} bson_fmt_options;

/**
 * @brief Format a BSON document into a human-readable string.
 *
 * @param out The output target for the string
 * @param doc The document to be formatted
 * @param opts Options to control the formatting
 */
void bson_write_repr(mlib_ostream out, bson_view doc, bson_fmt_options const* opts) mlib_noexcept;

#define bson_write_repr(...) MLIB_ARGC_PICK(_bson_write_repr, __VA_ARGS__)
#define _bson_write_repr_argc_2(Out, Doc)                                                          \
    bson_write_repr(mlib_ostream_from((Out)), bson_view_from((Doc)), NULL)
#define _bson_write_repr_argc_3(Out, Doc, Opts)                                                    \
    bson_write_repr(mlib_ostream_from((Out)), bson_view_from((Doc)), (Opts))

mlib_extern_c_end();
