#pragma once

#include <amongoc/aggregate_params.h>
#include <amongoc/client.h>
#include <amongoc/emitter.h>
#include <amongoc/loop.h>
#include <amongoc/status.h>
#include <amongoc/write_error.h>

#include <bson/doc.h>
#include <bson/value.h>
#include <bson/value_ref.h>
#include <bson/view.h>

#include <mlib/alloc.h>
#include <mlib/config.h>
#include <mlib/delete.h>
#include <mlib/str.h>
#include <mlib/time.h>

#include <string.h>

/**
 * @brief A handle to a collection within a database
 */
typedef struct amongoc_collection amongoc_collection;

mlib_extern_c_begin();

/**
 * @brief Obtain a CRUD handle to a collection within a database
 */
#define amongoc_collection_new(Client, DbName, CollName)                                           \
    _amongoc_collection_new(Client, mlib_str_view_from(DbName), mlib_str_view_from(CollName))
amongoc_collection* _amongoc_collection_new(amongoc_client* cl,
                                            mlib_str_view   db_name,
                                            mlib_str_view   coll_name) mlib_noexcept;

/**
 * @brief Delete a collection handle. Is a no-op for null handles.
 */
void amongoc_collection_delete(amongoc_collection*) mlib_noexcept;
mlib_assoc_deleter(amongoc_collection*, amongoc_collection_delete);

/**
 * @brief Obtain the client handle associated with the collection
 */
amongoc_client* amongoc_collection_get_client(amongoc_collection const*) mlib_noexcept;

/**
 * @brief Get the allocator associated with the given collection
 */
inline mlib_allocator
amongoc_collection_get_allocator(amongoc_collection const* coll) mlib_noexcept {
    return amongoc_client_get_allocator(amongoc_collection_get_client(coll));
}

// A status category for CRUD operations
extern const struct amongoc_status_category_vtable amongoc_crud_category;

// Status conditions for CRUD operations
enum amongoc_crud_errc {
    // Not an error
    amongoc_crud_okay,
    // One or more write errors occurred
    amongoc_crud_write_errors,
};

typedef struct amongoc_collection_drop_params {
    bson_value_ref comment;
} amongoc_collection_drop_params;

amongoc_emitter amongoc_collection_drop(amongoc_collection*                   coll,
                                        amongoc_collection_drop_params const* params) mlib_noexcept;

typedef struct amongoc_write_result {
    bool                    acknowledged;
    int64_t                 inserted_count;
    int64_t                 matched_count;
    int64_t                 modified_count;
    int64_t                 deleted_count;
    int64_t                 upserted_count;
    amongoc_write_error_vec write_errors;
    bson_value              upserted_id;

    mlib_declare_member_deleter(&amongoc_write_result::write_errors,
                                &amongoc_write_result::upserted_id);
} amongoc_write_result;

mlib_declare_c_deletion_function(amongoc_write_result_delete, amongoc_write_result);

// *  .o88b.  .d88b.  db    db d8b   db d888888b
// * d8P  Y8 .8P  Y8. 88    88 888o  88 `~~88~~'
// * 8P      88    88 88    88 88V8o 88    88
// * 8b      88    88 88    88 88 V8o88    88
// * Y8b  d8 `8b  d8' 88b  d88 88  V888    88
// *  `Y88P'  `Y88P'  ~Y8888P' VP   V8P    YP
typedef struct amongoc_count_params {
    bson_view      collation;
    bson_value_ref hint;
    int64_t        limit;
    mlib_duration  max_time;
    int64_t        skip;
    bson_value_ref comment;
} amongoc_count_params;

amongoc_emitter amongoc_count_documents(amongoc_collection*         coll,
                                        bson_view                   filter,
                                        const amongoc_count_params* params) mlib_noexcept;

amongoc_emitter amongoc_estimated_document_count(amongoc_collection*         coll,
                                                 const amongoc_count_params* params) mlib_noexcept;

// d8888b. d88888b db      d88888b d888888b d88888b
// 88  `8D 88'     88      88'     `~~88~~' 88'
// 88   88 88ooooo 88      88ooooo    88    88ooooo
// 88   88 88~~~~~ 88      88~~~~~    88    88~~~~~
// 88  .8D 88.     88booo. 88.        88    88.
// Y8888D' Y88888P Y88888P Y88888P    YP    Y88888P

/**
 * @brief Parameters for the deletion of documents from a collection
 */
typedef struct amongoc_delete_params {
    bson_view      collation;
    bson_value_ref hint;
    bson_view      let;
    bson_value_ref comment;
} amongoc_delete_params;

amongoc_emitter amongoc_delete_ex(amongoc_collection* coll,
                                  bson_view           filter,
                                  bool                delete_only_one,
                                  struct amongoc_delete_params const*) mlib_noexcept;

/**
 * @brief Delete a single document matching the given filter
 *
 * @param coll A collection from which to delete
 * @param filter A filter query to select a document to delete
 * @param params Deletion parameters
 * @return amongoc_emitter Resolves to an `int64_t` for the number of documents
 * deleted (either zero or one)
 */
inline amongoc_emitter amongoc_delete_one(amongoc_collection*          coll,
                                          bson_view                    filter,
                                          amongoc_delete_params const* params) mlib_noexcept {
    return amongoc_delete_ex(coll, filter, true, params);
}

/**
 * @brief Delete all documents matching the given filter
 *
 * @param coll A collection from which to delete
 * @param filter A filter query to select documents to delete
 * @param params Deletion parameters
 * @return amongoc_emitter Resolves to an `int64_t` for the number of documents
 * deleted
 */
inline amongoc_emitter
amongoc_delete_many(amongoc_collection*                 coll,
                    bson_view                           filter,
                    struct amongoc_delete_params const* params) mlib_noexcept {
    return amongoc_delete_ex(coll, filter, false, params);
}

// d8888b. d888888b .d8888. d888888b d888888b d8b   db  .o88b. d888888b
// 88  `8D   `88'   88'  YP `~~88~~'   `88'   888o  88 d8P  Y8 `~~88~~'
// 88   88    88    `8bo.      88       88    88V8o 88 8P         88
// 88   88    88      `Y8b.    88       88    88 V8o88 8b         88
// 88  .8D   .88.   db   8D    88      .88.   88  V888 Y8b  d8    88
// Y8888D' Y888888P `8888Y'    YP    Y888888P VP   V8P  `Y88P'    YP

typedef struct amongoc_distinct_params {
    bson_view      collation;
    bson_value_ref comment;
} amongoc_distinct_params;

amongoc_emitter amongoc_distinct(amongoc_collection*            coll,
                                 mlib_str_view                  field_name,
                                 bson_view                      filter,
                                 const amongoc_distinct_params* params) mlib_noexcept;

// d888888b d8b   db .d8888. d88888b d8888b. d888888b
//   `88'   888o  88 88'  YP 88'     88  `8D `~~88~~'
//    88    88V8o 88 `8bo.   88ooooo 88oobY'    88
//    88    88 V8o88   `Y8b. 88~~~~~ 88`8b      88
//   .88.   88  V888 db   8D 88.     88 `88.    88
// Y888888P VP   V8P `8888Y' Y88888P 88   YD    YP

/**
 * @brief Parameters for inserting data with `amongoc_collection_insert`
 */
typedef struct amongoc_insert_params {
    bool           bypass_document_validation;
    bool           ordered;
    bson_value_ref comment;
} amongoc_insert_params;

/**
 * @brief Insert a set of documents into the collection
 *
 * @param coll The colection to be modified
 * @param documents Pointer to an array of document views
 * @param n_docs The number of documents pointed-to by `documents`
 * @param opts Insertion options
 * @return amongoc_emitter An emitter that resolves with `amongoc_write_result`
 */
amongoc_emitter amongoc_insert_ex(amongoc_collection*          coll,
                                  const bson_view*             documents,
                                  size_t                       n_docs,
                                  amongoc_insert_params const* params) mlib_noexcept;

/**
 * @brief Insert a single document into the collection
 */
inline amongoc_emitter amongoc_insert_one(amongoc_collection*          coll,
                                          bson_view                    doc,
                                          amongoc_insert_params const* params) mlib_noexcept {
    return amongoc_insert_ex(coll, &doc, 1, params);
}

// d88888b d888888b d8b   db d8888b.
// 88'       `88'   888o  88 88  `8D
// 88ooo      88    88V8o 88 88   88
// 88~~~      88    88 V8o88 88   88
// 88        .88.   88  V888 88  .8D
// YP      Y888888P VP   V8P Y8888D'

enum amongoc_cursor_type {
    amongoc_find_cursor_not_tailable   = 0,
    amongoc_find_cursor_tailable       = 1,
    amongoc_find_cursor_tailable_await = 2,
};

/**
 * @brief Options for amongoc_collection_find operations
 */
typedef struct amongoc_find_params {
    bool                     allow_disk_use;
    bool                     allow_partial_results;
    int                      batch_size;
    bson_view                collation;
    bson_value_ref           comment;
    bson_value_ref           hint;
    enum amongoc_cursor_type cursor_type;
    bson_view                let;
    int                      limit;
    bson_view                max;
    int64_t                  max_await_time_ms;
    int64_t                  max_scan;
    mlib_duration            max_time;
    bson_view                min;
    bool                     no_cursor_timeout;
    bool                     oplog_replay;
    bson_view                projection;
    bool                     return_key;
    bool                     show_record_id;
    int64_t                  skip;
    bool                     snapshot;
    bson_view                sort;
} amongoc_find_params;

amongoc_emitter amongoc_find(amongoc_collection*        coll,
                             bson_view                  filter,
                             amongoc_find_params const* opts) mlib_noexcept;

// d88888b d888888b d8b   db d8888b.
// 88'       `88'   888o  88 88  `8D   db
// 88ooo      88    88V8o 88 88   88   88
// 88~~~      88    88 V8o88 88   88 C8888D
// 88        .88.   88  V888 88  .8D   88
// YP      Y888888P VP   V8P Y8888D'   VP

enum amongoc_return_document {
    amongoc_return_document_before,
    amongoc_return_document_after,
};

typedef struct amongoc_find_plus_params {
    bool                         bypass_document_validation;
    bson_view                    collation;
    bson_value_ref               comment;
    bson_value_ref               hint;
    bson_view                    let;
    bson_view                    projection;
    enum amongoc_return_document return_document;
    bson_view                    sort;
    bool                         upsert;
} amongoc_find_plus_params;

amongoc_emitter amongoc_find_and_modify(amongoc_collection*             coll,
                                        bson_view                       filter,
                                        bool                            remove,
                                        const bson_view*                opt_update_or_pipeline,
                                        size_t                          pipeline_len,
                                        const amongoc_find_plus_params* params) mlib_noexcept;

inline amongoc_emitter
amongoc_find_one_and_delete(amongoc_collection*             coll,
                            bson_view                       filter,
                            const amongoc_find_plus_params* params) mlib_noexcept {
    return amongoc_find_and_modify(coll,
                                   filter,
                                   true,  // delete
                                   NULL,  // no update
                                   0,     // ''
                                   params);
}

inline amongoc_emitter
amongoc_find_one_and_replace(amongoc_collection*             coll,
                             bson_view                       filter,
                             bson_view                       replacement,
                             const amongoc_find_plus_params* params) mlib_noexcept {
    return amongoc_find_and_modify(coll, filter, false, &replacement, 0, params);
}

inline amongoc_emitter
amongoc_find_one_and_update(amongoc_collection*             coll,
                            bson_view                       filter,
                            bson_view const*                update_or_pipeline,
                            size_t                          pipeline_len,
                            const amongoc_find_plus_params* params) mlib_noexcept {
    return amongoc_find_and_modify(coll, filter, false, update_or_pipeline, pipeline_len, params);
}

// d8888b. d88888b d8888b. db       .d8b.   .o88b. d88888b
// 88  `8D 88'     88  `8D 88      d8' `8b d8P  Y8 88'
// 88oobY' 88ooooo 88oodD' 88      88ooo88 8P      88ooooo
// 88`8b   88~~~~~ 88~~~   88      88~~~88 8b      88~~~~~
// 88 `88. 88.     88      88booo. 88   88 Y8b  d8 88.
// 88   YD Y88888P 88      Y88888P YP   YP  `Y88P' Y88888P

typedef struct amongoc_replace_params {
    bool           bypass_document_validation;
    bson_view      collation;
    bson_value_ref hint;
    bool           upsert;
    bson_view      let;
    bson_value_ref comment;
} amongoc_replace_params;

amongoc_emitter amongoc_replace_one(amongoc_collection*           coll,
                                    bson_view                     filter,
                                    bson_view                     replacement,
                                    const amongoc_replace_params* params) mlib_noexcept;

// db    db d8888b. d8888b.  .d8b.  d888888b d88888b
// 88    88 88  `8D 88  `8D d8' `8b `~~88~~' 88'
// 88    88 88oodD' 88   88 88ooo88    88    88ooooo
// 88    88 88~~~   88   88 88~~~88    88    88~~~~~
// 88b  d88 88      88  .8D 88   88    88    88.
// ~Y8888P' 88      Y8888D' YP   YP    YP    Y88888P

typedef struct amongoc_update_params {
    bson_view const* array_filters;
    size_t           array_filters_len;
    bool             bypass_document_validation;
    bson_view        collation;
    bson_value_ref   hint;
    bool             upsert;
    bson_view        let;
    bson_value_ref   comment;
} amongoc_update_params;

amongoc_emitter amongoc_update_ex(amongoc_collection*          coll,
                                  bson_view                    filter,
                                  bson_view const*             updates,
                                  size_t                       pipeline_len,
                                  bool                         is_multi,
                                  const amongoc_update_params* params) mlib_noexcept;

amongoc_emitter amongoc_update_many(amongoc_collection*          coll,
                                    bson_view                    filter,
                                    bson_view                    updates,
                                    const amongoc_update_params* params) mlib_noexcept;

amongoc_emitter amongoc_update_one(amongoc_collection*          coll,
                                   bson_view                    filter,
                                   bson_view                    updates,
                                   const amongoc_update_params* params) mlib_noexcept;

amongoc_emitter
amongoc_update_many_with_pipeline(amongoc_collection*          coll,
                                  bson_view                    filter,
                                  bson_view const*             pipeline,
                                  size_t                       pipeline_len,
                                  const amongoc_update_params* params) mlib_noexcept;

amongoc_emitter amongoc_update_one_with_pipeline(amongoc_collection*          coll,
                                                 bson_view                    filter,
                                                 bson_view const*             pipeline,
                                                 size_t                       pipeline_len,
                                                 const amongoc_update_params* params) mlib_noexcept;

typedef struct amongoc_cursor {
    int64_t             cursor_id;
    amongoc_collection* coll;
    bson_doc            records;
    int                 _batch_size;

    MLIB_IF_CXX(mlib::allocator<> get_allocator()
                    const noexcept { return ::amongoc_collection_get_allocator(coll); })

    mlib_declare_member_deleter(&amongoc_cursor::records);
} amongoc_cursor;

mlib_declare_c_deletion_function(amongoc_cursor_delete, amongoc_cursor);

amongoc_emitter amongoc_cursor_next(struct amongoc_cursor curs) mlib_noexcept;

mlib_extern_c_end();
