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

    client<tcp_connection_rw_stream, allocator<>> _client;
};

emitter amongoc_client::command(const bson_view& doc) noexcept {
    return amongoc_client_command(*this, doc);
}

emitter _connect(amongoc_loop* loop, std::string name, std::string svc) noexcept {
    auto addr   = *co_await async_resolve(*loop, name.data(), svc.data());
    auto socket = *co_await async_connect(*loop, std::move(addr));

    auto alloc = loop->get_allocator().rebind<_amongoc_client_cxx>();

    co_return unique_box::from(  //
        alloc,
        amongoc_client{alloc.new_(std::move(socket))},
        [](amongoc_client& cl) -> void { amongoc_client_destroy(cl); });
}

emitter amongoc_client_connect(amongoc_loop* loop, const char* name, const char* svc) noexcept {
    // Copy the name/svc into a string to outlive the operation state.
    return _connect(loop, std::string(name), std::string(svc));
}

emitter amongoc_client_command(amongoc_client cl, bson_view doc) noexcept {
    nanosender_of<emitter_result> auto s
        = cl._impl->_client.send_op_msg(doc)
        | amongoc::then([cl](result<bson_doc>&& r) -> emitter_result {
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
    return as_emitter(get_allocator(cl), std::move(s)).release();
}

void amongoc_client_destroy(amongoc_client cl) noexcept {
    cl.get_allocator().rebind<_amongoc_client_cxx>().delete_(cl._impl);
}

amongoc_loop* amongoc_client_get_event_loop(amongoc_client cl) noexcept {
    return cl._impl->_client.socket().loop;
}
