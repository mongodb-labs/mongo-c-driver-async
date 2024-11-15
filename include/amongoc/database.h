#pragma once

#include <amongoc/aggregate_params.h>
#include <amongoc/client.h>
#include <amongoc/emitter.h>

#include <bson/view.h>

#include <mlib/alloc.h>
#include <mlib/config.h>
#include <mlib/delete.h>

typedef struct amongoc_database amongoc_database;

mlib_extern_c_begin();

/**
 * @brief Obtain a manipulator for the a named database on the given client
 */
#define amongoc_database_new(Client, DbName) _amongoc_database_new(Client, mlib_as_str_view(DbName))
amongoc_database* _amongoc_database_new(amongoc_client* cl, mlib_str_view db_name) mlib_noexcept;

/**
 * @brief Delete a client-side database handle. Is a no-op for null handles.
 */
void amongoc_database_delete(amongoc_database* db) mlib_noexcept;
mlib_assoc_deleter(amongoc_database*, amongoc_database_delete);

amongoc_client* amongoc_database_get_client(amongoc_database const* db) mlib_noexcept;
const char*     amongoc_database_get_name(amongoc_database const* db) mlib_noexcept;

amongoc_emitter amongoc_database_aggregate(amongoc_database*               db,
                                           bson_view const*                pipeline,
                                           size_t                          pipeline_len,
                                           amongoc_aggregate_params const* params) mlib_noexcept;

mlib_extern_c_end();
