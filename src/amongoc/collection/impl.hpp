#pragma once

#include <amongoc/client.h>
#include <amongoc/client/impl.hpp>
#include <amongoc/collection.h>
#include <amongoc/string.hpp>

#include <bson/make.hpp>

#include <mlib/alloc.h>

struct amongoc_collection {
    ::amongoc_client client;
    amongoc::string  database_name;
    amongoc::string  collection_name;

    mlib::allocator<> get_allocator() const noexcept { return client.get_allocator(); }

    /**
     * @brief Generate a OP_MSG command document associated with the collection
     *
     * @param command_name The name of the command. The collection name will be
     * passed as the primary argument
     * @param elements Additional elements to be added to the document
     */
    template <typename... Ts>
    bson::document make_command(std::string_view command_name, const Ts&... elements) {
        using namespace bson::make;
        auto command = doc(pair(command_name, std::string_view(this->collection_name)),
                           pair("$db", std::string_view(this->database_name)),
                           elements...)
                           .build(client.get_allocator());
        return command;
    }

    amongoc::co_task<bson::document> simple_request(bson_view doc) const noexcept {
        return client.impl->simple_request(doc);
    }
};
