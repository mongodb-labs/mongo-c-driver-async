
#include "./client.hpp"

#include <amongoc/box.hpp>
#include <amongoc/client.h>
#include <amongoc/connection_pool.hpp>
#include <amongoc/coroutine.hpp>
#include <amongoc/emitter.hpp>
#include <amongoc/loop.h>
#include <amongoc/loop.hpp>
#include <amongoc/nano/nano.hpp>
#include <amongoc/uri.hpp>
#include <amongoc/wire/client.hpp>
#include <amongoc/wire/proto.hpp>

#include <bson/doc.h>

#include <mlib/alloc.h>
#include <mlib/str.h>
#include <mlib/utility.hpp>

using namespace amongoc;

amongoc::wire::checking_pool_client amongoc_client::checking_wire_client() {
    return amongoc::wire::checking_client(amongoc::pool_client(_pool));
}

emitter _amongoc_client_new(amongoc_loop* loop, mlib_str_view uri_str) noexcept {
    // Note: We copy the URI here before making the connect operation, because
    // we want to hold a copy of the URI string.
    auto uri = connection_uri::parse(uri_str, loop->get_allocator());
    co_await ramp_end;
    if (not uri.has_value()) {
        co_return uri.error();
    }
    auto alloc = loop->get_allocator().rebind<amongoc_client>();
    auto cl    = unique_box::from(alloc, alloc.new_(*loop, *mlib_fwd(uri)));
    // Await a connection from the pool, to ensure that the connection is valid
    co_await cl.as<amongoc_client*>()->_pool.checkout();
    // The connection is okay. Return it now.
    co_return cl;
}

static amongoc_emitter _command(amongoc_client& cl, auto doc) noexcept {
    co_await ramp_end;
    auto resp = co_await wire::simple_request(pool_client(cl._pool), bson_view(doc));
    co_return unique_box::from(cl.get_allocator(), mlib_fwd(resp).release());
}

emitter amongoc_client_command(amongoc_client* cl, bson_view doc) noexcept {
    return _command(*cl, bson::document(doc, cl->get_allocator()));
}

emitter amongoc_client_command_nocopy(amongoc_client* cl, bson_view doc) noexcept {
    return _command(*cl, doc);
}

void amongoc_client_delete(amongoc_client* cl) noexcept {
    mlib::delete_via_associated_allocator(cl);
}

amongoc_loop* amongoc_client_get_event_loop(amongoc_client const* cl) noexcept {
    return &cl->_pool.loop();
}

extern inline mlib_allocator amongoc_client_get_allocator(amongoc_client const* cl) noexcept;
