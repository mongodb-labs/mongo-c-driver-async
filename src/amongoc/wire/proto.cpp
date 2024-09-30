#include "./proto.hpp"

#include <amongoc/bson/build.h>
#include <amongoc/bson/view.h>

#include <fmt/base.h>

#include <exception>
#include <variant>

using namespace amongoc;

static_assert(wire::message_type<wire::any_op_msg_message>);
static_assert(wire::message_type<wire::any_message>);

template co_task<wire::any_message> wire::recv_message(allocator<>, tcp_connection_rw_stream&);

const bson::document& wire::any_message::expect_one_body_section_op_msg() const& noexcept {
    if (not std::holds_alternative<any_op_msg_message>(_content)) {
        throw_protocol_error("Expected a single OP_MSG message");
    }
    auto& content = std::get<any_op_msg_message>(_content);
    if (content.sections().size() != 1) {
        throw_protocol_error("Expected a single OP_MSG body section");
    }
    auto& section = content.sections().front();
    // XXX: When more section types are supported, the lambda expression below will
    // need to be changed to handle non-body sections (it should throw in those cases)
    return section.visit(
        [&](body_section<bson::document> const& sec) -> const bson::document& { return sec.body; });
}

void wire::trace::message_header(std::string_view prefix,
                                 std::size_t      messageLength,
                                 int              requestID,
                                 std::int32_t     opcode) {
    std::string_view op_name;
    switch (opcode) {
    case 2013:  // OP_MSG
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
            write(" {}: ", iter->key());
            iter->visit([this](auto x) { this->print_value(x); });
            write(" }}");
            return;
        }
        write("\n");
        for (auto ref : doc) {
            write("{}  {}: ", indent, ref.key());
            ref.visit([this](auto x) { this->print_value(x); });
            write(",\n");
        }
        write("{}}}", indent);
    }

    void print_value(std::string_view s) { write("\"{}\"", s); }

    void print_value(std::int32_t i) { write("{}:i32", i); }
    void print_value(std::int64_t i) { write("{}:i64", i); }
    void print_value(std::uint64_t i) { write("{}:u64", i); }
    void print_value(double i) { write("{}:f64", i); }
    void print_value(bson::null_t) { write("null"); }
    void print_value(bson::undefined_t) { write("undefined"); }
    void print_value(bson_view subdoc) { _bson_printer{indent + "  "}.print(subdoc); }
    void print_value(bson_dbpointer dbp) {
        write("DBPointer(\"{}\", ", dbp.collection);
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
    void print_value(bson_regex rx) { write("/{}/{}", rx.regex, rx.options); }
    void print_value(bson_binary bin) {
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
