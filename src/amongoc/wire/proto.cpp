#include "./proto.hpp"

#include <bson/doc.h>
#include <bson/format.h>
#include <bson/types.h>
#include <bson/value_ref.h>
#include <bson/view.h>

#include <mlib/config.h>

#include <fmt/core.h>

mlib_diagnostic_push();
mlib_gcc_warning_disable("-Wstringop-overflow");
#include <fmt/chrono.h>
mlib_diagnostic_pop();

#include <chrono>
#include <exception>
#include <variant>

using namespace amongoc;

static_assert(wire::message_type<wire::any_op_msg_message>);
static_assert(wire::message_type<wire::any_message>);

// Common send-message case
template co_task<mlib::unit>        wire::send_message(mlib::allocator<>,
                                                tcp_connection_rw_stream&,
                                                int,
                                                const wire::one_bson_view_op_msg);
template co_task<wire::any_message> wire::recv_message(mlib::allocator<>,
                                                       tcp_connection_rw_stream&);

void wire::trace::message_header(std::string_view prefix,
                                 std::size_t      messageLength,
                                 int              requestID,
                                 std::int32_t     opcode) {
    std::string_view op_name;
    switch (opcode) {
    case 2013:
        op_name = "OP_MSG";
        break;
    default:
        op_name = "<unknown opcode>";
        break;
    }

    fmt::print(stderr, "{} {} #{} ({} bytes)\n", prefix, op_name, requestID, messageLength);
    std::fflush(stderr);
}

void wire::trace::message_body_section(int nth, bson_view body) {
    fmt::print(stderr, "  Section #{} body: ", nth);
    bson_fmt_options opts{};
    opts.subsequent_indent = 2;
    opts.nested_indent     = 2;
    bson_write_repr(stderr, body, &opts);
    fmt::print(stderr, "\n");
}

void wire::trace::message_content(const wire::any_message& msg) {
    msg.visit_content([](auto const& content) { trace::message_content(content); });
}

void wire::trace::message_section(int nth, const wire::any_section& sec) {
    sec.visit([&](auto const& s) { trace::message_section(nth, s); });
}

void wire::trace::message_exception(const char* msg) {
    try {
        std::rethrow_exception(std::current_exception());
    } catch (std::exception const& e) {
        fmt::print(stderr, "[wire] {}: exception: {}\n", msg, e.what());
    } catch (...) {
        fmt::print(stderr, "[wire] {}: exception: (unknown type)\n", msg);
    }
}
