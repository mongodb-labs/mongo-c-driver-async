#pragma once

#include <amongoc/database.h>
#include <amongoc/string.hpp>

#include <mlib/alloc.h>

struct amongoc_client;

struct amongoc_database {
    ::amongoc_client& client;
    amongoc::string   database_name;

    mlib::allocator<> get_allocator() const noexcept {
        return ::amongoc_client_get_allocator(&client);
    }
};
