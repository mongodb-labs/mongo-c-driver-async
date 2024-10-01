#include <bson/utf8.h>
#include <bson/view.h>

#include <mlib/config.h>

#include <string.h>

extern mlib_constexpr bson_view bson_view_from_data(const bson_byte* const     data,
                                                    const size_t               len,
                                                    enum bson_view_errc* const error) mlib_noexcept;

extern mlib_constexpr uint32_t bson_iterator_data_size(const bson_iterator) mlib_noexcept;

extern mlib_constexpr bson_view _bson_view_from_ptr(const bson_byte*) mlib_noexcept;

extern mlib_constexpr bson_view bson_iterator_document(bson_iterator        it,
                                                       enum bson_view_errc* error) mlib_noexcept;

extern mlib_constexpr bson_iterator _bson_iterator_error(enum bson_iter_errc err) mlib_noexcept;

void _bson_assert_fail(const char* expr, const char* file, int line) {
    fprintf(stderr,
            "bson/view ASSERTION FAILED at %s:%d: Expression [%s] evaluated to "
            "false\n",
            file,
            line,
            expr);
    abort();
}

static inline void _test_generic_find(bson_view v, bson_utf8_view key) {
    bson_find(v, "key");
    bson_find(v, key);
}
