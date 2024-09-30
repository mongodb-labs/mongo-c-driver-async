#pragma once

#include <amongoc/coroutine.hpp>
#include <amongoc/string.hpp>
#include <amongoc/vector.hpp>
#include <amongoc/wire/client.hpp>
#include <amongoc/wire/proto.hpp>
#include <amongoc/wire/stream.hpp>

#include <bson/doc.h>

#include <mlib/alloc.h>

#include <asio/buffer.hpp>
#include <neo/platform.hpp>

#include <chrono>

namespace amongoc {

class tcp_connection_rw_stream;

/**
 * @brief A parsed handshake response message.
 *
 * Fields of this struct correspond to their name in the handshake response
 * message expected to be sent by servers.
 *
 * Refer: https://www.mongodb.com/docs/manual/reference/command/hello
 */
struct handshake_response {
    // Initialize with an allocator
    explicit handshake_response(allocator<> a) noexcept
        : topologyVersion(a) {}

    // Get the allocator for this object
    allocator<> get_allocator() const noexcept { return topologyVersion.get_allocator(); }

    // The type of points-in-time
    using time_point = std::chrono::utc_time<std::chrono::milliseconds>;

    // Common response fields
    bool isWritablePrimary = false;
    // TODO: This is not a string
    string               topologyVersion;
    std::size_t          maxBsonObjectSize   = 16 * 1024 * 1024;
    std::size_t          maxMessageSizeBytes = 48'000'000;
    std::size_t          maxWriteBatchSize   = 100'000;
    time_point           localTime;
    std::chrono::minutes logicalSessionTimeoutMinutes{0};
    int                  connectionId;
    int                  minWireVersion{0};
    int                  maxWireVersion{0};
    bool                 readOnly{false};
    vector<string>       compression{get_allocator()};
    vector<string>       saslSupportedMechs{get_allocator()};

    // Replicaset response fields
    vector<string> hosts{get_allocator()};
    string         setName{get_allocator()};
    string         setVersion{get_allocator()};
    bool           secondary = false;
    vector<string> passives{get_allocator()};
    vector<string> arbiters{get_allocator()};
    string         primary{get_allocator()};
    bool           arbiterOnly = false;
    bool           passive     = false;
    bool           hidden      = false;
    string         me{get_allocator()};
    string         electionId{get_allocator()};

    // Sharding fields
    string msg{get_allocator()};

    /**
     * @brief Parse a handshake response message from a BSON document
     *
     * @param a The allocator for message data
     * @param msg A BSON document that contains the handshake response
     */
    static handshake_response parse(allocator<> a, bson_view msg);
};

/**
 * @brief Create a handshake message document
 */
bson::document create_handshake_command(allocator<>                     a,
                                        std::optional<std::string_view> application_name);

/**
 * @brief Issue a MongoDB handshake on the given client
 *
 * @param alloc An allocator for the operation
 * @param strm The connection that will be used
 * @return Handshake response data
 */
template <wire::client_interface C>
co_task<handshake_response> handshake(C& cl, std::optional<std::string_view> app_name) {
    auto a    = cl.get_allocator();
    auto cmd  = create_handshake_command(a, app_name);
    auto msg  = wire::op_msg_message{std::array{wire::body_section{bson_view(cmd)}}};
    auto resp = co_await cl.request(msg);
    auto body = resp.expect_one_body_section_op_msg();
    co_return handshake_response::parse(a, body);
}

}  // namespace amongoc
