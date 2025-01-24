#include <amongoc/collection.h>

extern inline amongoc_emitter amongoc_delete_one(amongoc_collection*          coll,
                                                 bson_view                    filter,
                                                 amongoc_delete_params const* params) mlib_noexcept;

extern inline amongoc_emitter
amongoc_delete_many(amongoc_collection*                 coll,
                    bson_view                           filter,
                    struct amongoc_delete_params const* params) mlib_noexcept;

extern inline amongoc_emitter(amongoc_insert_one)(amongoc_collection*          coll,
                                                  bson_view                    doc,
                                                  amongoc_insert_params const* params)
    mlib_noexcept;

extern inline amongoc_emitter
amongoc_find_one_and_delete(amongoc_collection*             coll,
                            bson_view                       filter,
                            const amongoc_find_plus_params* params) mlib_noexcept;

extern inline amongoc_emitter
amongoc_find_one_and_replace(amongoc_collection*             coll,
                             bson_view                       filter,
                             bson_view                       replacement,
                             const amongoc_find_plus_params* params) mlib_noexcept;

extern inline amongoc_emitter
amongoc_find_one_and_update(amongoc_collection*             coll,
                            bson_view                       filter,
                            bson_view const*                update_or_pipeline,
                            size_t                          pipeline_len,
                            const amongoc_find_plus_params* params) mlib_noexcept;

extern inline mlib_allocator
amongoc_collection_get_allocator(amongoc_collection const* coll) mlib_noexcept;
