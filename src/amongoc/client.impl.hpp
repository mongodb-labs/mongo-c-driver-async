#pragma once

#include <amongoc/connection_pool.hpp>
#include <amongoc/loop.h>
#include <amongoc/uri.hpp>

struct _amongoc_client_impl {
    explicit _amongoc_client_impl(amongoc_loop& loop, amongoc::connection_uri&& uri)
        : _pool(loop, mlib_fwd(uri)) {}

    amongoc::connection_pool _pool;
};
