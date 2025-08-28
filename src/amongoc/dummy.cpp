#include <amongoc/collection.h>
#include <amongoc/operation.h>

static void use(auto... xs) { (printf("%p", (void*)xs), ...); }

extern void amongoc_function_referrer_dummy() {
    // This simply forces the definitions of these functions to be emitted in
    // at least one translation unit, so that it can be found by the C linker.
    // If these are not present, then linking to the library may fail since
    // no translation unit contains the function definitions.
    use(::amongoc_operation_delete, ::amongoc_cursor_delete);
}
