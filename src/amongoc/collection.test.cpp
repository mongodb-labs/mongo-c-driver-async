#include <amongoc/client.h>
#include <amongoc/client_fixture.test.hpp>
#include <amongoc/collection.h>
#include <amongoc/database.h>

#include <bson/doc.h>
#include <bson/make.hpp>
#include <bson/types.h>

#include <mlib/alloc.h>
#include <mlib/utility.hpp>

#include <catch2/catch_test_macros.hpp>

#include <test_params.test.hpp>

#include <iterator>

TEST_CASE_METHOD(amongoc::testing::client_fixture, "Collection/Get+Destroy") {
    auto coll = ::amongoc_collection_new(this->client, "testing-1", "coll-1");
    ::amongoc_collection_delete(coll);
}

struct collection_fixture : amongoc::testing::client_fixture {
    ::amongoc_collection* coll
        = ::amongoc_collection_new(this->client,
                                   amongoc::testing::parameters.app_name.data(),
                                   "test-coll-1");

    ~collection_fixture() {
        this->run_to_completion(::amongoc_collection_drop(this->coll, nullptr));
        ::amongoc_collection_delete(coll);
    }
};

TEST_CASE_METHOD(collection_fixture, "Collection/Insert+Find one") {
    auto d   = bson::make::doc(bson::make::pair("foo", "bar"));
    auto em  = ::amongoc_insert_one(coll, d.build(::mlib_default_allocator), nullptr);
    auto res = this->run_to_completion(em);
    {
        CAPTURE(res.status.message());
        REQUIRE_FALSE(res.status.is_error());
    }

    em  = ::amongoc_find(coll, bson::make::doc().build(::mlib_default_allocator), nullptr);
    res = this->run_to_completion(em);
    {
        CAPTURE(res.status.message());
        REQUIRE_FALSE(res.status.is_error());
    }

    mlib::unique cursor = res.value.take<amongoc_cursor>();
    auto         iter   = bson_begin(cursor->records);
    REQUIRE_FALSE(iter.stop());
    REQUIRE(iter->type() == bson_type_document);
    auto one = *iter;
    {
        auto sub_iter = one.value().document.find("foo");
        CHECKED_IF(not sub_iter.stop()) { CHECK(sub_iter->value().get_utf8() == "bar"); }
    }
    ++iter;
    // There should be no more data
    CHECK(iter.stop());
}

TEST_CASE_METHOD(collection_fixture, "Collection/Insert+Find many") {
    auto d = bson::make::doc(bson::make::pair("foo", "bar")).build(::mlib_default_allocator);
    std::vector<bson::view> docs(10, d);
    auto                    em  = ::amongoc_insert_ex(coll, docs.data(), docs.size(), nullptr);
    auto                    res = this->run_to_completion(em);
    {
        CAPTURE(res.status.message());
        REQUIRE_FALSE(res.status.is_error());
    }

    // Only find three at a time
    auto find_opts       = ::amongoc_find_params{};
    find_opts.batch_size = 3;
    // Find them
    em  = ::amongoc_find(coll, bson::make::doc().build(::mlib_default_allocator), &find_opts);
    res = this->run_to_completion(em);
    {
        CAPTURE(res.status.message());
        REQUIRE_FALSE(res.status.is_error());
    }

    amongoc_cursor   cursor = res.value.take<amongoc_cursor>();
    mlib::scope_exit _      = [&] { ::amongoc_cursor_delete(cursor); };
    auto             iter   = bson_begin(cursor.records);
    REQUIRE_FALSE(iter.stop());
    REQUIRE(iter->type() == bson_type_document);
    auto one = *iter;
    {
        auto sub_iter = one.value().document.find("foo");
        CHECKED_IF(not sub_iter.stop()) { CHECK(sub_iter->value().get_utf8() == "bar"); }
    }
    ++iter;
    // There should be no more data
    CHECK(std::ranges::distance(cursor.records) == find_opts.batch_size);

    em  = ::amongoc_cursor_next(mlib::take(cursor));
    res = this->run_to_completion(em);
    {
        CAPTURE(res.status.message());
        REQUIRE_FALSE(res.status.is_error());
    }
}
