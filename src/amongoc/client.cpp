#include "./client.hpp"

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
        : _client(std::move(conn)) {}

    client<tcp_connection_rw_stream> _client;
};

emitter amongoc_client::command(const bson_view& doc) noexcept {
    return amongoc_client_command(*this, doc);
}

emitter amongoc_client_connect(amongoc_loop* loop, const char* name, const char* svc) noexcept {
    auto addr   = *co_await async_resolve(*loop, name, svc);
    auto socket = *co_await async_connect(*loop, std::move(addr));
    co_return unique_box::from(amongoc_client{new _amongoc_client_cxx{std::move(socket)}},
                               [](amongoc_client& cl) -> void { amongoc_client_destroy(cl); });
}

emitter amongoc_client_command(amongoc_client cl, bson_view doc) noexcept {
    nanosender_of<result<bson_doc>> auto s = cl._impl->_client.send_op_msg(doc);
    return as_emitter(std::move(s)).release();
}

void amongoc_client_destroy(amongoc_client cl) noexcept { delete cl._impl; }
