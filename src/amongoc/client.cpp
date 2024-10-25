
#include "./client/impl.hpp"

#include <amongoc/box.h>
#include <amongoc/client.h>
#include <amongoc/connection_pool.hpp>
#include <amongoc/coroutine.hpp>
#include <amongoc/emitter.h>
#include <amongoc/loop.h>
#include <amongoc/loop.hpp>
#include <amongoc/nano/nano.hpp>
#include <amongoc/uri.hpp>
#include <amongoc/wire/client.hpp>
#include <amongoc/wire/proto.hpp>

#include <bson/doc.h>

#include <mlib/alloc.h>
#include <mlib/str.h>

using namespace amongoc;

emitter _amongoc_client_new(amongoc_loop* loop, mlib_str_view uri_str) noexcept {
    // Note: We copy the URI here before making the connect operation, because
    // we want to hold a copy of the URI string.
    auto uri = connection_uri::parse(uri_str, loop->get_allocator());
    co_await ramp_end;
    if (not uri.has_value()) {
        co_return uri.error();
    }
    auto alloc = loop->get_allocator().rebind<_amongoc_client_impl>();
    auto cl    = unique_box::from(  //
        alloc,
        amongoc_client{alloc.new_(*loop, *mlib_fwd(uri))},
        just_invokes<&amongoc_client_delete>{});
    // Await a connection from the pool, to ensure that the connection is valid
    co_await cl.as<amongoc_client>().impl->_pool.checkout();
    // The connection is okay. Return it now.
    co_return cl;
}

static amongoc_emitter _command(amongoc_client cl, auto doc) noexcept {
    co_await ramp_end;
    auto resp = co_await wire::simple_request(pool_client(cl.impl->_pool), bson_view(doc));
    co_return unique_box::from(cl.get_allocator(),
                               mlib_fwd(resp).release(),
                               just_invokes<&bson_delete>{});
}

emitter amongoc_client_command(amongoc_client cl, bson_view doc) noexcept {
    return _command(cl, bson::document(doc, cl.get_allocator()));
}

emitter amongoc_client_command_nocopy(amongoc_client cl, bson_view doc) noexcept {
    return _command(cl, doc);
}

void amongoc_client_delete(amongoc_client cl) noexcept {
    cl.get_allocator().rebind<_amongoc_client_impl>().delete_(cl.impl);
}

amongoc_loop* amongoc_client_get_event_loop(amongoc_client cl) noexcept {
    return &cl.impl->_pool.loop();
}

extern inline mlib_allocator amongoc_client_get_allocator(amongoc_client cl) noexcept;
