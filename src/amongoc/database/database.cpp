#include <amongoc/client.h>
#include <amongoc/client/impl.hpp>
#include <amongoc/database.h>
#include <amongoc/database/impl.hpp>
#include <amongoc/string.hpp>

#include <bson/make.hpp>

#include <mlib/alloc.h>
#include <mlib/utility.hpp>

#include <new>

using namespace amongoc;

amongoc_database _amongoc_database_new(amongoc_client cl, mlib_str_view db_name) noexcept try {
    auto ptr = cl.get_allocator().rebind<_amongoc_database_impl>().new_(cl,
                                                                        string(db_name,
                                                                               cl.get_allocator()));
    return amongoc_database{ptr};
} catch (const std::bad_alloc&) {
    return {};
}

void amongoc_database_delete(::amongoc_database db) noexcept {
    mlib::delete_via_associated_allocator(db);
}

const char* amongoc_database_get_name(amongoc_database_const db) noexcept {
    return db->database_name.data();
}

amongoc_client amongoc_database_get_client(amongoc_database_const db) noexcept {
    return db->client;
}

static emitter _amc_db_aggregate(mlib::allocator<>               a,
                                 amongoc_database                db,
                                 const bson_view*                pipeline,
                                 size_t                          pipeline_len,
                                 const amongoc_aggregate_params* params) {
    static const amongoc_aggregate_params dflt_params{};
    params or (params = &dflt_params);

    using namespace bson::make;
    const auto batch_size = params->batch_size;
    const auto command
        = doc(pair("aggregate", 1),
              pair("$db", db->database_name),
              pair("pipeline", range(std::span(pipeline, pipeline_len))),
              optional_pair("allowDiskUse", params->allow_disk_use),
              optional_pair("bypassDocumentValidation", params->bypass_document_validation),
              pair("cursor", doc(optional_pair("batchSize", batch_size))),
              optional_pair("collation", params->collation),
              optional_pair("comment", params->comment),
              optional_pair("hint", params->hint),
              optional_pair("let", params->let))
              .build(a);
    co_await ramp_end;
    const bson::document resp = co_await db->client.impl->simple_request(command);
    std::terminate();  // TODO
}

// TODO: Complete and document this API
amongoc_emitter amongoc_database_aggregate(amongoc_database                db,
                                           bson_view const*                pipeline,
                                           size_t                          pipeline_len,
                                           amongoc_aggregate_params const* params) mlib_noexcept {
    return ::_amc_db_aggregate(db->get_allocator(), db, pipeline, pipeline_len, params);
}
