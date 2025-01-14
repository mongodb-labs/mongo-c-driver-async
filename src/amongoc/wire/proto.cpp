#include "./proto.hpp"

#include <bson/doc.h>
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

struct _bson_printer {
    std::string indent;

    template <typename... Args>
    void write(fmt::format_string<Args...> fstr, Args&&... args) {
        fmt::print(stderr, fmt::runtime(fstr), args...);
    }

    void print(bson_view doc) {
        write("{{");
        auto iter = doc.begin();
        if (iter == doc.end()) {
            // Empty. Nothing to print
            write(" }}");
            return;
        }
        if (std::next(iter) == doc.end()) {
            // Only one element
            write(" {:?}: ", iter->key());
            iter->value().visit([this](auto x) { this->print_value(x); });
            write(" }}");
            return;
        }
        write("\n");
        for (auto ref : doc) {
            write("{}  {:?}: ", indent, ref.key());
            ref.value().visit([this](auto x) { this->print_value(x); });
            write(",\n");
        }
        write("{}}}", indent);
    }

    void print_value(bson_array_view arr) {
        write("[");
        auto iter = arr.begin();
        if (iter == arr.end()) {
            write("]");
            return;
        }
        write("\n");
        auto indented = _bson_printer{indent + "  "};
        for (auto ref : arr) {
            write("{}    ", indent);
            ref.value().visit([&](auto x) { indented.print_value(x); });
            write(",\n");
        }
        write("  {}]", indent);
    }

    void print_value(std::string_view s) { write("{:?}", s); }
    void print_value(bson_symbol_view s) { write("Symbol({:?})", std::string_view(s.utf8)); }

    auto _as_formattable_time_point(std::int64_t utc_ms_offset) {
#if FMT_USE_UTC_TIME
        // C++20 UTC time point is supported
        return std::chrono::utc_clock::time_point(std::chrono::milliseconds(utc_ms_offset));
#else
        // Fall back to the system clock. This is not certain to give the correct answer,
        // but we're just logging, not saving the world
        return std::chrono::system_clock::time_point(std::chrono::milliseconds(utc_ms_offset));
#endif
    }

    void print_value(::bson_datetime dt) {
        auto tp = _as_formattable_time_point(dt.utc_ms_offset);
        write("Datetime⟨{:%c}⟩", tp);
    }

    void print_value(::bson_timestamp ts) {
        auto tp = _as_formattable_time_point(ts.utc_sec_offset);
        write("Timestamp(⟨{:%c}⟩ : {})", tp, ts.increment);
    }
    void print_value(::bson_code_view c) { write("Code({:?})", std::string_view(c.utf8)); }

    void print_value(::bson_decimal128) { write("[[Unimplemented: Decimal128 printing]]"); }

    void print_value(bson_eod) {
        assert(false && "Should never be called. Tried to print-trace a phony EOD element.");
    }
    void print_value(bool b) { write("{}", b); }
    void print_value(std::int32_t i) { write("{}:i32", i); }
    void print_value(std::int64_t i) { write("{}:i64", i); }
    void print_value(std::uint64_t i) { write("{}:u64", i); }
    void print_value(double i) { write("{}:f64", i); }
    void print_value(bson::null) { write("null"); }
    void print_value(bson::undefined) { write("undefined"); }
    void print_value(bson::minkey) { write("[[min key]]"); }
    void print_value(bson::maxkey) { write("[[max key]]"); }
    void print_value(bson_view subdoc) { _bson_printer{indent + "  "}.print(subdoc); }
    void print_value(bson_dbpointer_view dbp) {
        write("DBPointer(\"{}\", ", std::string_view(dbp.collection));
        print_value(dbp.object_id);
        write(")");
    }
    void print_value(bson_oid oid) {
        write("ObjectID(");
        print_bytes(oid.bytes);
        write(")");
    }
    void print_bytes(auto&& seq) {
        for (auto n : seq) {
            write("{:0>2x}", std::uint8_t(std::byte(n)));
        }
    }
    void print_value(bson_regex_view rx) {
        write("/{}/{}", std::string_view(rx.regex), std::string_view(rx.options));
    }
    void print_value(bson_binary_view bin) {
        write("Binary({}, ", bin.subtype);
        print_bytes(std::ranges::subrange(bin.data, bin.data + bin.data_len));
    }
};

void wire::trace::message_body_section(int nth, bson_view body) {
    fmt::print(stderr, "  Section #{} body: ", nth);
    _bson_printer{"  "}.print(body);
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
