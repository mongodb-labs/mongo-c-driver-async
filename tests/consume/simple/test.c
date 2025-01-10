#include <amongoc/amongoc.h>

#undef NDEBUG
#include <assert.h>

int main() {
    int             val = 42;
    amongoc_emitter em
        = amongoc_just(amongoc_okay, amongoc_box_pointer(&val), mlib_default_allocator);
    amongoc_status    status;
    amongoc_box       box;
    amongoc_operation op = amongoc_tie(em, &status, &box, mlib_default_allocator);
    amongoc_start(&op);
    amongoc_operation_delete(op);
    printf("%p", &amongoc_insert_one);  // Checks linking
    assert(status.code == 0);
    assert(status.category == &amongoc_generic_category);
    assert(*amongoc_box_cast(int*, box) == 42);
    return 0;
}
