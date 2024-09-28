#include "./handshake.hpp"

#include <amongoc/bson/build.h>
#include <amongoc/bson/make.hpp>
#include <amongoc/bson/parse.hpp>
#include <amongoc/bson/view.h>
#include <amongoc/wire/error.hpp>

#include <cstddef>
#include <optional>
#include <string_view>

using namespace amongoc;

amongoc::handshake_response amongoc::handshake_response::parse(allocator<> a, bson_view msg) {
    handshake_response ret{a};

    using bson::parse::doc;
    using bson::parse::each;
    using bson::parse::field;
    using bson::parse::integral;
    using bson::parse::must;
    using bson::parse::store;
    using bson::parse::type;

    auto store_string = [&](auto& into) {
        return type<std::string_view>([&](std::string_view sv) -> bson::parse::accepted {
            into = string(sv, a);
            return {};
        });
    };
    auto store_size = [&](std::size_t& out) {
        return [&](bson::parse::reference const& el) {
            out = static_cast<std::size_t>(el.as_int64());
            return bson::parse::accepted{};
        };
    };
    auto append_strings = [&](vector<string>& vec) {
        return type<bson::view>(
            each(type<std::string_view>([&](std::string_view sv) -> bson::parse::accepted {
                vec.emplace_back(sv);
                return {};
            })));
    };

    auto parse = doc{
        must(field("isWritablePrimary", type<bool>(store(ret.isWritablePrimary)))),
        must(field("topologyVersion", bson::parse::just_accept{})),  // TODO
        must(field("maxBsonObjectSize", must(store_size(ret.maxBsonObjectSize)))),
        must(field("maxMessageSizeBytes", must(store_size(ret.maxMessageSizeBytes)))),
        must(field("maxWriteBatchSize", must(store_size(ret.maxWriteBatchSize)))),
        must(field("localTime", bson::parse::just_accept{})),                     // TODO
        must(field("logicalSessionTimeoutMinutes", bson::parse::just_accept{})),  // TODO
        must(field("connectionId", must(integral(store(ret.connectionId))))),
        must(field("minWireVersion", must(integral(store(ret.minWireVersion))))),
        must(field("maxWireVersion", must(integral(store(ret.maxWireVersion))))),
        must(field("readOnly", must(integral(store(ret.readOnly))))),
        field("compression", must(append_strings(ret.compression))),
        field("saslSupportedMechs", must(append_strings(ret.saslSupportedMechs))),
        field("hosts", must(append_strings(ret.hosts))),
        field("setName", must(store_string(ret.setName))),
        field("setVersion", must(store_string(ret.setVersion))),
        field("secondary", must(integral(store(ret.setVersion)))),
        field("passives", must(append_strings(ret.passives))),
        field("arbiters", must(append_strings(ret.arbiters))),
        field("primary", must(store_string(ret.primary))),
        field("arbiterOnly", must(integral(store(ret.arbiterOnly)))),
        field("passive", must(integral(store(ret.passive)))),
        field("hidden", must(integral(store(ret.hidden)))),
        field("me", must(store_string(ret.me))),
        field("electionId", must(store_string(ret.electionId))),
        field("msg", must(store_string(ret.msg))),
        bson::parse::just_accept{},
    };
    auto result = parse(msg);
    if (not bson::parse::did_accept(result)) {
        auto err = bson::parse::describe_error(result);
        wire::throw_protocol_error(err);
    }

    // TODO: lastWrite, tags
    return ret;
}

bson::document amongoc::create_handshake_command(allocator<>                     alloc,
                                                 std::optional<std::string_view> app_name) {
    using bson::make::conditional;
    using bson::make::doc;
    using std::pair;
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
