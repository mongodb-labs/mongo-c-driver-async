#pragma once

#include <amongoc/aggregate_params.h>
#include <amongoc/emitter.h>

#include <bson/value_ref.h>
#include <bson/view.h>

#include <mlib/config.h>
#include <mlib/time.h>

#include <stdint.h>

struct amongoc_collection;
struct amongoc_database;

mlib_extern_c_begin();

amongoc_emitter amongoc_collection_aggregate(struct amongoc_collection*      coll,
                                             bson_view const*                pipeline,
                                             size_t                          pipeline_len,
                                             const amongoc_aggregate_params* params) mlib_noexcept;

amongoc_emitter amongoc_database_aggregate(struct amongoc_database*        db,
                                           bson_view const*                pipeline,
                                           size_t                          pipeline_len,
                                           const amongoc_aggregate_params* params) mlib_noexcept;

mlib_extern_c_end();

#if mlib_is_cxx()
amongoc_emitter _amongoc_aggregate_on(::amongoc_collection*           coll,
                                      bson_view const*                pipeline,
                                      size_t                          pipeline_len,
                                      const amongoc_aggregate_params* params) noexcept;

amongoc_emitter _amongoc_aggregate_on(::amongoc_database*             db,
                                      bson_view const*                pipeline,
                                      size_t                          pipeline_len,
                                      const amongoc_aggregate_params* params) noexcept;
#endif  // C++

#define amongoc_aggregate(First, ...)                                                              \
    mlib_generic(_amongoc_aggregate_on,                                                            \
                 amongoc_collection_aggregate,                                                     \
                 (First),                                                                          \
                 struct amongoc_collection* : amongoc_collection_aggregate,                        \
                 struct amongoc_database* : amongoc_database_aggregate)((First), __VA_ARGS__)
