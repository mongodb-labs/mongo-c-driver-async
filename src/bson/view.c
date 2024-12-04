#include <bson/iterator.h>
#include <bson/view.h>

#include <mlib/config.h>

#include <string.h>

extern inline bson_view bson_view_from_data(const bson_byte* const     data,
                                            const size_t               len,
                                            enum bson_view_errc* const error) mlib_noexcept;

extern inline bson_view _bson_view_from_ptr(const bson_byte*) mlib_noexcept;

extern inline bson_iterator _bson_iterator_error(enum bson_iter_errc err) mlib_noexcept;

extern inline const bson_view*       _bsonViewNullInst() mlib_noexcept;
extern inline const bson_array_view* _bsonArrayViewNullInst() mlib_noexcept;

void _bson_assert_fail(const char* expr, const char* file, int line) {
    fprintf(stderr,
            "bson/view ASSERTION FAILED at %s:%d: Expression [%s] evaluated to "
            "false\n",
            file,
            line,
            expr);
    abort();
}
