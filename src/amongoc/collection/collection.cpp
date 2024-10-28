#include <amongoc/aggregate.h>
#include <amongoc/box.h>
#include <amongoc/client.h>
#include <amongoc/collection.h>
#include <amongoc/collection/impl.hpp>
#include <amongoc/nano/util.hpp>
#include <amongoc/status.h>
#include <amongoc/write_error.h>

#include <bson/parse.hpp>
#include <bson/value_ref.h>

#include <mlib/alloc.h>
#include <mlib/utility.hpp>

#include <iterator>
#include <new>
#include <string_view>

using namespace amongoc;

extern inline void amongoc_write_result_delete(amongoc_write_result r) mlib_noexcept;

extern inline amongoc_emitter amongoc_delete_one(amongoc_collection*          coll,
                                                 bson_view                    filter,
                                                 amongoc_delete_params const* params) noexcept;

extern inline amongoc_emitter
amongoc_delete_many(amongoc_collection*                 coll,
                    bson_view                           filter,
                    struct amongoc_delete_params const* params) noexcept;

extern inline amongoc_emitter amongoc_insert_one(amongoc_collection           coll,
                                                 bson_view                    doc,
                                                 amongoc_insert_params const* params) mlib_noexcept;

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

extern inline void amongoc_cursor_delete(amongoc_cursor c) mlib_noexcept;

constexpr const amongoc_status_category_vtable amongoc_crud_category = {
    .name = [] { return "amongoc.crud"; },
    .strdup_message =
        [](int c) {
            switch (static_cast<::amongoc_crud_errc>(c)) {
            case ::amongoc_crud_okay:
                return strdup("okay");
            case ::amongoc_crud_write_errors:
                return strdup("The operation resulted in one or more write errors");
            default:
                return strdup("Unknown error");
            }
        },
    .is_error        = nullptr,
    .is_cancellation = nullptr,
    .is_timeout      = nullptr,
};

static ::amongoc_cursor _parse_cursor(::amongoc_collection& coll, int batch_size, bson_view resp) {
    amongoc_cursor curs{};
    bson_view      batch{};
    {
        using namespace bson::parse;
        using bson::parse::any;
        using bson::parse::doc;
        using bson::parse::require;
        must_parse(resp,
                   doc{
                       require("cursor",
                               must(doc{
                                   must(any(field("firstBatch", must(store(batch))),
                                            field("nextBatch", must(store(batch))))),
                                   require("id", store(curs.cursor_id)),
                                   // Ignore other fields
                                   field("ns", just_accept{}),
                               })),
                   });
    }
    curs._batch_size = batch_size;
    curs.coll        = &coll;
    curs.records     = bson::document(batch, coll.get_allocator()).release();
    return curs;
}

::amongoc_collection* _amongoc_collection_new(amongoc_client cl,
                                              mlib_str_view  db_name,
                                              mlib_str_view  coll_name) noexcept try {
    auto ptr
        = cl.get_allocator().rebind<amongoc_collection>().new_(cl,
                                                               string(db_name, cl.get_allocator()),
                                                               string(coll_name,
                                                                      cl.get_allocator()));
    return ptr;
} catch (const std::bad_alloc&) {
    return nullptr;
}

void amongoc_collection_delete(amongoc_collection* coll) noexcept {
    mlib::delete_via_associated_allocator(coll);
}

extern inline mlib_allocator
amongoc_collection_get_allocator(amongoc_collection const* coll) noexcept;

amongoc_client amongoc_collection_get_client(amongoc_collection const* coll) noexcept {
    return coll->client;
}

emitter amongoc_collection_drop(amongoc_collection*                   coll,
                                const amongoc_collection_drop_params* params) noexcept {
    const static amongoc_collection_drop_params dflt_params{};
    params or (params = &dflt_params);
    using namespace bson::make;
    auto cmd = coll->make_command("drop", optional_pair("comment", params->comment));
    co_await ramp_end;
    co_await coll->simple_request(cmd);
    co_return 0;
}

emitter amongoc_aggregate_on_collection(amongoc_collection*             coll,
                                        const bson_view*                pipeline,
                                        size_t                          pipeline_len,
                                        const amongoc_aggregate_params* params) noexcept {
    static const amongoc_aggregate_params dflt_params{};
    params or (params = &dflt_params);

    using namespace bson::make;
    const auto batch_size = params->batch_size;
    const auto command
        = coll->make_command("aggregate",
                             pair("pipeline", range(std::span(pipeline, pipeline_len))),
                             optional_pair("allowDiskUse", params->allow_disk_use),
                             optional_pair("bypassDocumentValidation",
                                           params->bypass_document_validation),
                             pair("cursor", doc(optional_pair("batchSize", params->batch_size))),
                             optional_pair("collation", params->collation),
                             optional_pair("comment", params->comment),
                             optional_pair("hint", params->hint),
                             optional_pair("let", params->let));
    co_await ramp_end;
    const bson::document resp = co_await coll->simple_request(command);
    const auto           curs = _parse_cursor(*coll, batch_size, resp);
    co_return unique_box::from(coll->get_allocator(), curs, just_invokes<&amongoc_cursor_delete>{});
}

emitter amongoc_count_documents(amongoc_collection*         coll,
                                bson_view                   filter,
                                const amongoc_count_params* params) noexcept {
    static const amongoc_count_params dflt_params = {};
    params or (params = &dflt_params);

    using namespace bson::make;
    bson::document cmd = coll->make_command(  //
        "aggregate",
        pair("pipeline",
             array(doc(pair("$match", filter)),
                   optional_value(params->skip
                                      ? std::make_optional(doc(pair("$skip", params->skip)))
                                      : std::nullopt),
                   optional_value(params->limit
                                      ? std::make_optional(doc(pair("$limit", params->limit)))
                                      : std::nullopt),
                   doc(pair("$group", doc(pair("_id", 1), pair("n", doc(pair("$sum", 1)))))))),
        // Default cursor
        pair("cursor", doc()),
        optional_pair("maxTimeMS", ::mlib_count_milliseconds(params->max_time)),
        optional_pair("hint", params->hint),
        optional_pair("collation", params->collation),
        optional_pair("comment", params->comment));

    co_await ramp_end;
    auto resp = co_await coll->simple_request(cmd);

    using namespace bson::parse;
    using bson::parse::doc;
    std::int32_t n = 0;

    must_parse(resp,
               doc(require("cursor",
                           doc(require("firstBatch",
                                       must(bson::parse::any(
                                           // An empty doc is returned for an empty collection:
                                           doc(reject_others{}),
                                           // Otherwise, we have a single element:
                                           doc(require("0", doc(require("n", must(store(n))))),
                                               // There should only be the "0" field:
                                               reject_others{}))))))));

    co_return unique_box::from(::mlib_terminating_allocator, n);
}

emitter amongoc_delete_ex(amongoc_collection*                 coll,
                          bson_view                           filter,
                          bool                                delete_only_one,
                          const struct amongoc_delete_params* params) noexcept {
    static const amongoc_delete_params dflt_params = {};
    params or (params = &dflt_params);

    using namespace bson::make;
    const auto command
        = coll->make_command("delete",
                             pair("deletes",
                                  array(doc(pair("q", filter),
                                            pair("limit", delete_only_one ? 1 : 0),
                                            optional_pair("collation", params->collation),
                                            optional_pair("hint", params->hint)))),
                             optional_pair("comment", params->comment),
                             optional_pair("let", params->let));
    co_await ramp_end;
    const auto   resp = co_await coll->simple_request(command);
    std::int64_t n    = -1;
    using namespace bson::parse;
    using bson::parse::doc;
    must_parse(resp, doc(require("n", integer(store(n)))));
    co_return ::amongoc_box_int64(n).as_unique();
}

emitter amongoc_distinct(amongoc_collection*            coll,
                         mlib_str_view                  field_name,
                         bson_view                      filter,
                         const amongoc_distinct_params* params) noexcept {
    static const amongoc_distinct_params dflt_params = {};
    params or (params = &dflt_params);

    using namespace bson::make;
    const auto command = coll->make_command(  //
        "distinct",
        pair("key", field_name),
        optional_pair("query", filter),
        optional_pair("collation", params->collation),
        optional_pair("comment", params->comment));
    co_await ramp_end;
    const auto      resp = co_await coll->simple_request(command);
    bson_array_view values;
    {
        using namespace bson::parse;
        using bson::parse::doc;
        must_parse(resp, doc(require("values", must(type<bson_array_view>(store(values))))));
    }
    bool           alloc_okay = false;
    bson_value_vec vec
        = bson_value_vec_new_n(static_cast<std::size_t>(std::ranges::distance(values)),
                               &alloc_okay,
                               coll->get_allocator().c_allocator());
    if (not alloc_okay) {
        throw std::bad_alloc();
    }
    mlib_defer_fail { bson_value_vec_delete(vec); };
    auto out = vec.begin();
    for (auto el : values) {
        *out++ = bson_value_copy(el.value());
    }
    co_return unique_box::from(coll->get_allocator(),
                               vec,
                               just_invokes<&::bson_value_vec_delete>{});
}

emitter amongoc_estimated_document_count(amongoc_collection*         coll,
                                         const amongoc_count_params* params) noexcept {
    static const amongoc_count_params dflt_params = {};
    if (not params) {
        params = &dflt_params;
    }
    using namespace bson;
    using namespace bson::make;

    auto command = coll->make_command("count",
                                      optional_pair("maxTimeMS",
                                                    ::mlib_count_milliseconds(params->max_time)),
                                      optional_pair("comment", params->comment));
    co_await ramp_end;
    const auto resp = co_await coll->simple_request(command);
    using namespace bson::parse;
    std::int64_t n;
    must_parse(resp, parse::doc(require("n", integer(store(n)))));
    co_return ::amongoc_box_int64(n).as_unique();
}

emitter amongoc_find(amongoc_collection*        coll,
                     bson_view                  filter,
                     amongoc_find_params const* params) noexcept {
    static const amongoc_find_params dflt_params{};
    if (not params) {
        params = &dflt_params;
    }

    using namespace bson::make;

    auto batch_size = params->batch_size;

    bson::document command = coll->make_command(  //
        "find",
        pair("filter", filter),
        optional_pair("sort", params->sort),
        optional_pair("projection", params->projection),
        optional_pair("hint", params->hint),
        optional_pair("skip", params->skip),
        optional_pair("limit", params->limit),
        optional_pair("batchSize", params->batch_size),
        // If the limit is set to a negative value, generate a single batch
        pair("singleBatch", params->limit < 0),
        optional_pair("comment", params->comment),
        optional_pair("maxTimeMS", ::mlib_count_milliseconds(params->max_time)),
        optional_pair("max", params->max),
        optional_pair("min", params->min),
        pair("returnKey", params->return_key),
        pair("oplogReplay", params->oplog_replay),
        pair("showRecordId", params->show_record_id),
        pair("tailable", params->cursor_type != ::amongoc_find_cursor_not_tailable),
        pair("noCursorTimeout", params->no_cursor_timeout),
        pair("awaitData", bool(params->cursor_type & ::amongoc_find_cursor_tailable_await)),
        pair("allowPartialResults", params->allow_partial_results),
        optional_pair("collation", params->collation),
        pair("allowDiskUse", params->allow_disk_use),
        optional_pair("let", params->let));
    co_await ramp_end;
    const auto resp = co_await coll->simple_request(command);
    const auto curs = _parse_cursor(*coll, batch_size, resp);

    co_return unique_box::from(coll->get_allocator(), curs, just_invokes<&amongoc_cursor_delete>{});
}

emitter amongoc_cursor_next(amongoc_cursor curs) noexcept {
    auto id   = curs.cursor_id;
    auto coll = curs.coll;
    using bson::make::doc;
    using namespace bson::make;
    ::amongoc_cursor_delete(curs);  // Delete the records
    curs               = {};
    bson::document cmd = doc(pair("getMore", id),
                             pair("$db", coll->database_name),
                             pair("collection", coll->collection_name),
                             optional_pair("batchSize", curs._batch_size))
                             .build(coll->get_allocator());
    co_await ramp_end;
    const auto resp = co_await coll->simple_request(cmd);
    curs            = _parse_cursor(*coll, curs._batch_size, resp);
    co_return unique_box::from(coll->get_allocator(), curs, just_invokes<&amongoc_cursor_delete>{});
}

/**
 * @brief Parse out an amongoc_write_result from a response message
 *
 * @param resp The command response message.
 * @param n_field Pointer to the member of `amongoc_write_result` that will receive
 *  the value of the `n` field.
 * @param alloc An allocator to be used for the object.
 */
static amongoc_write_result _parse_write_result(bson_view resp,
                                                int64_t(amongoc_write_result::*n_field),
                                                mlib::allocator<> alloc) {
    amongoc_write_result ret{};
    using namespace bson::parse;
    bson_value_ref upserted_id{};
    ret.write_errors          = amongoc_write_error_vec_new(alloc.c_allocator());
    int64_t          we_code  = 0;
    int64_t          we_index = 0;
    std::string_view we_str_msg;
    mlib_defer_fail { amongoc_write_result_delete(ret); };

    // Parse rule that extracts and appends a write error
    auto parse_write_error = all{
        // First extract each field
        doc(require("index", integer(store(we_index))),
            require("code", integer(store(we_code))),
            require("errmsg", store(we_str_msg))),
        // Then append to the output vector.
        action([&](auto&&) {
            auto* we = ::amongoc_write_error_vec_push(&ret.write_errors);
            if (not we) {
                throw std::bad_alloc();
            }
            we->code   = static_cast<::amongoc_server_errc>(we_code);
            we->errmsg = mlib_str_copy(we_str_msg, alloc.c_allocator()).str;
            if (not we->errmsg.data) {
                throw std::bad_alloc();
            }
        }),
    };
    must_parse(resp,
               doc(require("n", must(integer(store(ret.*n_field)))),
                   field("nModified", must(integer(store(ret.modified_count)))),
                   field("nMatched", must(integer(store(ret.matched_count)))),
                   field("nUpserted", must(integer(store(ret.upserted_count)))),
                   field("upserted",
                         must(doc(require("0", doc(require("_id", store(upserted_id))))))),
                   field("writeErrors", must(each(parse_write_error)))));
    ret.upserted_id = bson_value_copy(upserted_id, alloc.c_allocator());
    if (upserted_id.has_value()) {
        ret.upserted_count++;
    }
    ret.*n_field -= ret.upserted_count;
    return ret;
}

emitter amongoc_insert_ex(amongoc_collection*          coll,
                          const bson_view*             documents,
                          std::size_t                  n_docs,
                          const amongoc_insert_params* params) noexcept {
    static const amongoc_insert_params default_params = {};
    if (not params) {
        params = &default_params;
    }
    using namespace bson::make;
    const auto command
        = coll->make_command("insert",
                             pair("documents", range(std::span(documents, n_docs))),
                             pair("ordered", params->ordered),
                             pair("bypassDocumentValidation", params->bypass_document_validation));

    co_await ramp_end;
    const auto             resp = co_await coll->simple_request(command);
    ::amongoc_write_result res
        = _parse_write_result(resp, &amongoc_write_result::inserted_count, coll->get_allocator());
    mlib_defer_fail { amongoc_write_result_delete(res); };

    status st;
    st.category = &amongoc_crud_category;
    st.code     = res.write_errors.size ? ::amongoc_crud_write_errors : ::amongoc_crud_okay;

    res.acknowledged = true;
    co_return emitter_result(st,
                             unique_box::from(coll->get_allocator(),
                                              res,
                                              just_invokes<&::amongoc_write_result_delete>{}));
}

static bool _is_update_spec_doc(bson_view s) {
    auto it = s.begin();
    return not it.stop() and it->key().starts_with("$");
}

static emitter _invalid_update_doc_em(mlib::allocator<> a) {
    return amongoc_just(amongoc_status{&amongoc_client_category,
                                       ::amongoc_client_errc_invalid_update_document},
                        amongoc_nil,
                        a.c_allocator());
}

emitter amongoc_replace_one(amongoc_collection*           coll,
                            bson_view                     filter,
                            bson_view                     replacement,
                            const amongoc_replace_params* params) noexcept {
    const static amongoc_replace_params dflt_params{};
    params or (params = &dflt_params);
    params or (params = &dflt_params);
    if (_is_update_spec_doc(replacement)) {
        return _invalid_update_doc_em(coll->get_allocator());
    }
    amongoc_update_params update{};
    update.bypass_document_validation = params->bypass_document_validation;
    update.collation                  = params->collation;
    update.comment                    = params->comment;
    update.hint                       = params->hint;
    update.upsert                     = params->upsert;
    update.let                        = params->let;
    return ::amongoc_update_ex(coll, filter, &replacement, 0, false, &update);
}

emitter amongoc_update_ex(amongoc_collection*          coll,
                          bson_view                    filter,
                          const bson_view*             updates,
                          size_t                       pipeline_len,
                          bool                         is_multi,
                          const amongoc_update_params* params) noexcept {
    const static amongoc_update_params dflt_params = {};
    params or (params = &dflt_params);
    using namespace bson::make;
    const auto command = coll->make_command(
        "update",
        pair("updates",
             array(doc(pair("q", filter),
                       optional_pair("u", pipeline_len == 0 ? updates[0] : bson_view{}),
                       optional_pair("u",
                                     pipeline_len > 0 ? std::make_optional(
                                                            range(std::span(updates, pipeline_len)))
                                                      : std::nullopt),
                       pair("upsert", params->upsert),
                       pair("multi", is_multi),
                       optional_pair("arrayFilters",
                                     params->array_filters
                                         ? std::make_optional(
                                               range(std::span(params->array_filters,
                                                               params->array_filters_len)))
                                         : std::nullopt),
                       optional_pair("collation", params->collation),
                       optional_pair("hint", params->hint)))),
        optional_pair("let", params->let),
        optional_pair("comment", params->comment),
        optional_pair("bypassDocumentValidation", params->bypass_document_validation));
    co_await ramp_end;
    const auto resp = co_await coll->simple_request(command);
    auto       res
        = ::_parse_write_result(resp, &amongoc_write_result::matched_count, coll->get_allocator());
    mlib_defer_fail { amongoc_write_result_delete(res); };

    status st;
    st.category = &amongoc_crud_category;
    st.code     = res.write_errors.size ? ::amongoc_crud_write_errors : ::amongoc_crud_okay;

    co_return emitter_result(st,
                             unique_box::from(coll->get_allocator(),
                                              res,
                                              just_invokes<&::amongoc_write_result_delete>{}));
}

amongoc_emitter amongoc_update_one_with_pipeline(amongoc_collection*          coll,
                                                 bson_view                    filter,
                                                 bson_view const*             pipe,
                                                 size_t                       pipeline_len,
                                                 const amongoc_update_params* params) noexcept {

    return amongoc_update_ex(coll, filter, pipe, pipeline_len, false, params);
}

amongoc_emitter amongoc_update_many_with_pipeline(amongoc_collection*          coll,
                                                  bson_view                    filter,
                                                  bson_view const*             pipe,
                                                  size_t                       pipeline_len,
                                                  const amongoc_update_params* params) noexcept {
    return amongoc_update_ex(coll, filter, pipe, pipeline_len, true, params);
}

amongoc_emitter amongoc_update_many(amongoc_collection*          coll,
                                    bson_view                    filter,
                                    bson_view                    updates,
                                    const amongoc_update_params* params) noexcept {
    if (not _is_update_spec_doc(updates)) {
        return _invalid_update_doc_em(coll->get_allocator());
    }
    return amongoc_update_ex(coll, filter, &updates, 0, true, params);
}

amongoc_emitter amongoc_update_one(amongoc_collection*          coll,
                                   bson_view                    filter,
                                   bson_view                    updates,
                                   const amongoc_update_params* params) noexcept {
    if (not _is_update_spec_doc(updates)) {
        return _invalid_update_doc_em(coll->get_allocator());
    }
    return amongoc_update_ex(coll, filter, &updates, 0, false, params);
}

emitter amongoc_find_and_modify(amongoc_collection*             coll,
                                bson_view                       filter,
                                bool                            remove,
                                const bson_view*                opt_update_or_pipeline,
                                size_t                          pipeline_len,
                                const amongoc_find_plus_params* params) noexcept {

    using namespace bson::make;

    const auto command
        = coll->make_command("findAndModify",
                             pair("query", filter),
                             optional_pair("remove", remove),
                             optional_pair("upsert", params->upsert),
                             pair("new", params->return_document == amongoc_return_document_after),
                             // Single-document replacement or update operation:
                             optional_pair("update",
                                           pipeline_len == 0 and opt_update_or_pipeline
                                               ? *opt_update_or_pipeline
                                               : bson_view()),
                             // An aggregation pipeline:
                             optional_pair("update",
                                           pipeline_len > 0 ? std::make_optional(range(
                                                                  std::span(opt_update_or_pipeline,
                                                                            pipeline_len)))
                                                            : std::nullopt),
                             optional_pair("collation", params->collation),
                             optional_pair("comment", params->comment),
                             optional_pair("let", params->let),
                             optional_pair("bypassDocumentValidation",
                                           params->bypass_document_validation));
    co_await ramp_end;
    const auto resp = co_await coll->simple_request(command);
    using namespace bson::parse;
    using bson::parse::any;
    using bson::parse::doc;
    bson_view view{};
    must_parse(resp, doc(require("value", any(type<bson_null>(), store(view)))));
    co_return unique_box::from(coll->get_allocator(),
                               view ? ::bson_new(view, coll->get_allocator().c_allocator())
                                    : bson_doc{},
                               just_invokes<&::bson_delete>{});
}
