#include "./handshake.hpp"

#include <amongoc/wire/error.hpp>

#include <bson/doc.h>
#include <bson/make.hpp>
#include <bson/parse.hpp>
#include <bson/value.h>
#include <bson/view.h>

#include <cstddef>
#include <optional>
#include <string_view>

using namespace amongoc;

amongoc::handshake_response amongoc::handshake_response::parse(allocator<> a, bson_view msg) {
    handshake_response ret{a};

    using namespace bson::parse;

    auto store_size = [&](std::size_t& out) {
        return [&](bson::parse::reference const& el) -> basic_result {
            out = static_cast<std::size_t>(el.value().as_int64());
            return {};
        };
    };

    // Copy a string or string view, imbuing it with our allocator
    auto copy_string = after(construct<string>{}, constant(a));

    // Create a rule that stores a string view to a string, using our allocator to copy
    auto store_string = [&](string& into) {
        return type<std::string_view>(action(atop(assign(into), copy_string)));
    };

    // A rule that appends an array of strings to an output vector
    auto append_strings = [&](vector<string>& vec) {
        return type<bson_array_view>(
            each(type<std::string_view>([&](std::string_view sv) -> basic_result {
                vec.emplace_back(sv);
                return {};
            })));
    };

    must_parse(  //
        msg,
        doc{
            require("isWritablePrimary", store(ret.isWritablePrimary)),
            require("topologyVersion", bson::parse::just_accept{}),  // TODO
            require("maxBsonObjectSize", must(store_size(ret.maxBsonObjectSize))),
            require("maxMessageSizeBytes", must(store_size(ret.maxMessageSizeBytes))),
            require("maxWriteBatchSize", must(store_size(ret.maxWriteBatchSize))),
            require("localTime", bson::parse::just_accept{}),                     // TODO
            require("logicalSessionTimeoutMinutes", bson::parse::just_accept{}),  // TODO
            require("connectionId", must(integer(store(ret.connectionId)))),
            require("minWireVersion", must(integer(store(ret.minWireVersion)))),
            require("maxWireVersion", must(integer(store(ret.maxWireVersion)))),
            require("readOnly", must(store(ret.readOnly))),
            field("compression", must(append_strings(ret.compression))),
            field("saslSupportedMechs", must(append_strings(ret.saslSupportedMechs))),
            field("hosts", must(append_strings(ret.hosts))),
            field("setName", must(store_string(ret.setName))),
            field("setVersion", must(store_string(ret.setVersion))),
            field("secondary", must(integer(store(ret.setVersion)))),
            field("secondary", must(integer(store(ret.setVersion)))),
            field("passives", must(append_strings(ret.passives))),
            field("arbiters", must(append_strings(ret.arbiters))),
            field("primary", must(store_string(ret.primary))),
            field("arbiterOnly", must(integer(store(ret.arbiterOnly)))),
            field("passive", must(integer(store(ret.passive)))),
            field("hidden", must(integer(store(ret.hidden)))),
            field("me", must(store_string(ret.me))),
            field("electionId", must(store_string(ret.electionId))),
            field("msg", must(store_string(ret.msg))),
            // TODO: lastWrite, tags
            bson::parse::just_accept{},
        });

    return ret;
}

bson::document amongoc::create_handshake_command(allocator<>                     alloc,
                                                 std::optional<std::string_view> app_name) {
    using namespace bson::make;
#undef linux
    auto os_type = [] {
        switch (neo::operating_system) {
        case neo::operating_system_t::windows:
            return "Windows";
            break;
        case neo::operating_system_t::linux:
            return "Linux";
            break;
        case neo::operating_system_t::macos:
            return "Darwin";
            break;
        case neo::operating_system_t::freebsd:
            return "FreeBSD";
            break;
        case neo::operating_system_t::openbsd:
            return "OpenBSD";
            break;
        case neo::operating_system_t::unknown:
        default:
            return "unknown";
            break;
        }
    }();
    auto command = doc(pair("hello", std::int32_t(1)),
                       pair("$db", "admin"),
                       pair("client",
                            doc{
                                conditional(app_name.transform([](auto name) {
                                    return pair("application", doc(pair("name", name)));
                                })),
                                pair("driver",
                                     doc{
                                         pair("name", "amongoc"),
                                         pair("version", "experimental-dev"),
                                     }),
                                pair("os", doc(pair("type", os_type))),
                                // TODO: Add useful compiler information
                            }))
                       .build(alloc);
    return command;
}
