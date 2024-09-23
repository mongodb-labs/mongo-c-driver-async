
#include <amongoc/box.h>
#include <amongoc/bson/build.h>
#include <amongoc/connection.h>
#include <amongoc/connection_pool.hpp>
#include <amongoc/coroutine.hpp>
#include <amongoc/emitter.h>
#include <amongoc/loop.h>
#include <amongoc/loop.hpp>
#include <amongoc/nano/nano.hpp>
#include <amongoc/uri.hpp>
#include <amongoc/wire/proto.hpp>

#include <mlib/alloc.h>

using namespace amongoc;

struct _amongoc_client_cxx {
    explicit _amongoc_client_cxx(amongoc_loop& loop, connection_uri&& uri)
        : _pool(loop, mlib_fwd(uri)) {}

    connection_pool _pool;
};

emitter _amc_client_connect(amongoc_loop& loop, connection_uri uri) noexcept {
    auto alloc = loop.get_allocator().rebind<_amongoc_client_cxx>();
    auto cl    = unique_box::from(  //
        alloc,
        amongoc_client{alloc.new_(loop, mlib_fwd(uri))},
        just_invokes<&amongoc_client_destroy>{});
    // Await a connection from the pool, to ensure that the connection is valid
    co_await cl.as<amongoc_client>()._impl->_pool.checkout();
    // The connection is okay. Return it now.
    co_return cl;
}

emitter amongoc_client_new(amongoc_loop* loop, const char* uri_str) noexcept {
    // Note: We copy the URI here before making the connect operation, because
    // we want to hold a copy of the URI string.
    auto uri = connection_uri::parse(uri_str, loop->get_allocator());
    if (not uri.has_value()) {
        return amongoc_just(uri.error(), amongoc_nil, ::mlib_terminating_allocator);
    }
    return _amc_client_connect(*loop, *mlib_fwd(uri));
}

static amongoc_emitter _command(amongoc_client cl, auto doc) noexcept {
    wire::client_interface auto conn = co_await cl._impl->_pool.checkout();
    auto              msg  = wire::op_msg_message{std::array{wire::body_section(bson_view(doc))}};
    wire::any_message resp = co_await conn.request(msg);
    bson::document&&  body = std::move(resp).expect_one_body_section_op_msg();
    co_return unique_box::from(cl.get_allocator(),
                               mlib_fwd(body).release(),
                               just_invokes<&bson_mut_delete>{});
}

emitter amongoc_client_command(amongoc_client cl, bson_view doc) noexcept {
    return _command(cl, doc);
}

emitter amongoc_client_command_nocopy(amongoc_client cl, bson_view doc) noexcept {
    return _command(cl, bson::document(doc, cl.get_allocator()));
}

void amongoc_client_destroy(amongoc_client cl) noexcept {
    cl.get_allocator().rebind<_amongoc_client_cxx>().delete_(cl._impl);
}

amongoc_loop* amongoc_client_get_event_loop(amongoc_client cl) noexcept {
    return &cl._impl->_pool.loop();
}
