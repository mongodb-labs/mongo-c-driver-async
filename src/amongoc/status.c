#include <amongoc/status.h>

extern inline amongoc_status const* _amongocStatusGetOkayStatus(void) mlib_noexcept;
extern inline const char* amongoc_message(amongoc_status st, char* buf, size_t) mlib_noexcept;
