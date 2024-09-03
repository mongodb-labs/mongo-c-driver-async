#include "./connection.hpp"
#include "./string.hpp"
#include "amongoc/bson/build.h"

#include <amongoc/connection.h>

#include <amongoc/box.h>
#include <amongoc/coroutine.hpp>
#include <amongoc/emitter.h>
#include <amongoc/loop.hpp>
#include <amongoc/nano/nano.hpp>

#include <chrono>
#include <memory>

using namespace amongoc;

struct _amongoc_connection_cxx {
    explicit _amongoc_connection_cxx(tcp_connection_rw_stream&& conn)
        : _conn(get_allocator(*conn.loop), std::move(conn)) {}

    raw_connection<tcp_connection_rw_stream, allocator<>> _conn;
};

emitter amongoc_connection::command(const bson_view& doc) noexcept {
    return amongoc_conn_command(*this, doc);
}

emitter _connect(amongoc_loop* loop, string name, string svc) noexcept {
    auto addr   = *co_await async_resolve(*loop, name.data(), svc.data());
    auto socket = *co_await async_connect(*loop, std::move(addr));

    auto alloc = loop->get_allocator().rebind<_amongoc_connection_cxx>();

    co_return unique_box::from(  //
        alloc,
        amongoc_connection{alloc.new_(std::move(socket))},
        [](amongoc_connection& cl) -> void { amongoc_conn_destroy(cl); });
}

emitter amongoc_conn_connect(amongoc_loop* loop, const char* name, const char* svc) noexcept {
    // Copy the name/svc into a string to outlive the operation state.
    return _connect(loop, string(name, loop->get_allocator()), string(svc, loop->get_allocator()));
}

static nanosender_of<emitter_result> auto _command(amongoc_connection cl, auto doc) noexcept {
    return cl._impl->_conn.send_op_msg(doc)
        | amongoc::then([cl](result<bson::document>&& r) -> emitter_result {
               if (r.has_value()) {
                   bson_mut m = std::move(r).value().release();
                   return emitter_result(  //
                       0,
                       unique_box::from(get_allocator(cl), m, [](bson_mut& m) {
                           bson_mut_delete(m);
                       }));
               } else {
                   return emitter_result(r.error());
               }
           });
}

emitter amongoc_conn_command(amongoc_connection cl, bson_view doc) noexcept {
    return as_emitter(cl.get_allocator(), _command(cl, bson::document(doc, cl.get_allocator())))
        .release();
}

emitter amongoc_conn_command_nocopy(amongoc_connection cl, bson_view doc) noexcept {
    return as_emitter(cl.get_allocator(), _command(cl, doc)).release();
}

void amongoc_conn_destroy(amongoc_connection cl) noexcept {
    cl.get_allocator().rebind<_amongoc_connection_cxx>().delete_(cl._impl);
}

amongoc_loop* amongoc_conn_get_event_loop(amongoc_connection cl) noexcept {
    return cl._impl->_conn.socket().loop;
}
