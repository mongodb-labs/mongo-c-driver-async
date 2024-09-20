#include "./handshake.hpp"

#include <amongoc/bson/build.h>
#include <amongoc/bson/view.h>

#include <chrono>
#include <optional>
#include <string_view>

using namespace amongoc;

amongoc::handshake_response amongoc::handshake_response::parse(allocator<> a, bson_view msg) {
    handshake_response ret{a};
#define FIELD(FieldName, Cast)                                                                     \
    if (1) {                                                                                       \
        auto iter = msg.find(#FieldName);                                                          \
        if (iter != msg.end())                                                                     \
            ret.FieldName = mlib::invoke(Cast, *iter);                                             \
    } else                                                                                         \
        ((void)0)
    using iter_ref     = bson_iterator::reference;
    auto as_string     = atop(after(construct<string>{}, constant(a)), &iter_ref::utf8);
    auto as_size       = atop(construct<std::size_t>{}, &iter_ref::as_int64);
    auto as_int        = atop(construct<int>{}, &iter_ref::as_int32);
    auto as_string_vec = [&](iter_ref elem) -> vector<string> {
        vector<string> ret{a};
        for (auto child : elem.document()) {
            ret.emplace_back(child.utf8());
        }
        return ret;
    };
    // Common fields
    FIELD(isWritablePrimary, &iter_ref::as_bool);
    FIELD(topologyVersion, as_string);
    FIELD(maxBsonObjectSize, as_size);
    FIELD(maxMessageSizeBytes, as_size);
    FIELD(maxWriteBatchSize, as_size);
    FIELD(localTime,
          atop(construct<time_point>{},
               atop(construct<std::chrono::milliseconds>{}, &iter_ref::datetime_utc_ms)));
    FIELD(logicalSessionTimeoutMinutes, atop(construct<std::chrono::minutes>{}, as_size));
    FIELD(connectionId, as_string);
    FIELD(minWireVersion, as_int);
    FIELD(maxWireVersion, as_int);
    FIELD(readOnly, &iter_ref::as_bool);
    FIELD(compression, as_string_vec);
    FIELD(saslSupportedMechs, as_string_vec);

    // Replication fields
    FIELD(hosts, as_string_vec);
    FIELD(setName, as_string);
    FIELD(setVersion, as_string);
    FIELD(secondary, &iter_ref::as_bool);
    FIELD(passives, as_string_vec);
    FIELD(arbiters, as_string_vec);
    FIELD(primary, as_string);
    FIELD(arbiterOnly, &iter_ref::as_bool);
    FIELD(passive, &iter_ref::as_bool);
    FIELD(hidden, &iter_ref::as_bool);
    FIELD(me, as_string);
    FIELD(electionId, as_string);

    // Sharding fields
    FIELD(msg, as_string);

    // TODO: lastWrite, tags
    return ret;
}

bson::document amongoc::create_handshake_command(allocator<>                     alloc,
                                                 std::optional<std::string_view> app_name) {
    bson::document hello{alloc};
    hello.emplace_back("hello", 1);
    hello.emplace_back("$db", "admin");
    {
        auto client = hello.push_subdoc("client");
        if (app_name.has_value()) {
            auto app = hello.push_subdoc("application");
            app.emplace_back("name", *app_name);
        }
        {
            auto dr = client.push_subdoc("driver");
            dr.emplace_back("name", "amongoc");
            dr.emplace_back("version", "experimental-dev");
        }
        {
            auto os = client.push_subdoc("os");
            switch (neo::operating_system) {
            case neo::operating_system_t::windows:
                os.emplace_back("type", "Windows");
                break;
#undef linux
            case neo::operating_system_t::linux:
                os.emplace_back("type", "Linux");
                break;
            case neo::operating_system_t::macos:
                os.emplace_back("type", "Darwin");
                break;
            case neo::operating_system_t::freebsd:
                os.emplace_back("type", "FreeBSD");
                break;
            case neo::operating_system_t::openbsd:
                os.emplace_back("type", "OpenBSD");
                break;
            case neo::operating_system_t::unknown:
            default:
                os.emplace_back("type", "unknown");
                break;
            }
        }
        // TODO: Add useful compiler information
    }
    return hello;
}
