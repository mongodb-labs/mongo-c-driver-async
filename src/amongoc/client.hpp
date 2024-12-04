#pragma once

#include <amongoc/client.h>
#include <amongoc/connection_pool.hpp>
#include <amongoc/uri.hpp>

struct amongoc_client {
    explicit amongoc_client(amongoc_loop& loop, amongoc::connection_uri&& uri)
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

    mlib::allocator<> get_allocator() const noexcept { return _pool.get_allocator(); }
};
