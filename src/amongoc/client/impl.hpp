#pragma once

#include <amongoc/connection_pool.hpp>
#include <amongoc/loop.h>
#include <amongoc/uri.hpp>
#include <amongoc/wire/client.hpp>
#include <amongoc/wire/message.hpp>

struct _amongoc_client_impl {
    explicit _amongoc_client_impl(amongoc_loop& loop, amongoc::connection_uri&& uri)
        : _pool(loop, mlib_fwd(uri)) {}

    amongoc::connection_pool _pool;

    /**
     * @brief Obtain a wire client on the connection pool that checks for server errors
     *
     * @return auto
     */
    amongoc::wire::checking_pool_client checking_wire_client();

    amongoc::co_task<bson::document> simple_request(bson_view doc) noexcept {
        return amongoc::wire::simple_request(this->checking_wire_client(), doc);
    }
};
