#include <amongoc/box.h>

extern inline void* _amongocBoxInitStorage(amongoc_box*           box,
                                           bool                   allow_inline,
                                           size_t                 size,
                                           amongoc_box_destructor dtor,
                                           mlib_allocator         alloc) mlib_noexcept;

extern inline amongoc_box const* _amongocBoxGetNil() mlib_noexcept;
extern inline void*       _amongocBoxDataPtr(struct _amongoc_box_storage* stor) mlib_noexcept;
extern inline void        amongoc_box_destroy(amongoc_box box) mlib_noexcept;
extern inline const void* _amongocBoxConstDataPtr(const amongoc_box* b) mlib_noexcept;
extern inline const void* _amongocViewDataPtr(const amongoc_view* b) mlib_noexcept;
extern inline void*       _amongocBoxMutDataPtr(amongoc_box* b) mlib_noexcept;
extern inline void _amongoc_box_take_impl(void* dst, size_t sz, amongoc_box* box) mlib_noexcept;
extern inline void amongoc_box_free_storage(amongoc_box box) mlib_noexcept;
