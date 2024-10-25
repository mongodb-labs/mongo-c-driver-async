#include <amongoc/client.h>
#include <amongoc/connection_pool.hpp>
#include <amongoc/database.h>
#include <amongoc/string.hpp>

#include <mlib/alloc.h>

struct _amongoc_database_impl {
    ::amongoc_client client;
    amongoc::string  database_name;

    mlib::allocator<> get_allocator() const noexcept {
        return ::amongoc_client_get_allocator(client);
    }
};
