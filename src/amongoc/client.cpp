#include "./client.hpp"
#include "amongoc/bson/build.h"

#include <amongoc/client.h>

#include <amongoc/box.h>
#include <amongoc/coroutine.hpp>
#include <amongoc/emitter.h>
#include <amongoc/loop.hpp>
#include <amongoc/nano/nano.hpp>

#include <chrono>
#include <memory>

using namespace amongoc;

struct _amongoc_client_cxx {
    explicit _amongoc_client_cxx(tcp_connection_rw_stream&& conn)
        : _client(get_allocator(*conn.loop), std::move(conn)) {}

    client<tcp_connection_rw_stream, cxx_allocator<>> _client;
};

emitter amongoc_client::command(const bson_view& doc) noexcept {
    return amongoc_client_command(*this, doc);
}

emitter amongoc_client_connect(amongoc_loop* loop, const char* name, const char* svc) noexcept {
    auto addr   = *co_await async_resolve(*loop, name, svc);
    auto socket = *co_await async_connect(*loop, std::move(addr));
    co_return unique_box::from(get_allocator(*loop),
                               amongoc_client{new _amongoc_client_cxx{std::move(socket)}},
                               [](amongoc_client& cl) -> void { amongoc_client_destroy(cl); });
}

emitter amongoc_client_command(amongoc_client cl, bson_view doc) noexcept {
    result<bson_doc> resp = co_await cl._impl->_client.send_op_msg(doc);
    if (resp.has_value()) {
        bson_mut m = std::move(resp).value().release();
        co_return unique_box::from(cl.get_allocator(), m, [](bson_mut& m) { bson_mut_delete(m); });
    }
    co_return resp.error();
}

void amongoc_client_destroy(amongoc_client cl) noexcept { delete cl._impl; }

amongoc_loop* amongoc_client_get_event_loop(amongoc_client cl) noexcept {
    return cl._impl->_client.socket().loop;
}
