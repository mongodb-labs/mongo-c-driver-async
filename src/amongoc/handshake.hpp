#pragma once

#include <amongoc/asio/read_write.hpp>
#include <amongoc/bson/build.h>
#include <amongoc/coroutine.hpp>
#include <amongoc/string.hpp>
#include <amongoc/vector.hpp>
#include <amongoc/wire.hpp>

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
    string               connectionId{get_allocator()};
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
 * @brief Issue a MongoDB handshake on the given stream
 *
 * @param alloc An allocator for the operation
 * @param strm The connection that will be used
 * @return Handshake response data
 */
template <writable_stream Stream>
    requires mlib::has_mlib_allocator<Stream>
co_task<handshake_response> handshake(Stream&&                        strm,
                                      std::optional<std::string_view> application_name) {
    const allocator<> a   = mlib::get_allocator(strm);
    auto              cmd = create_handshake_command(a, application_name);
    co_await wire::send_op_msg_one_section(a, strm, 0, cmd);
    wire::any_message resp = co_await wire::recv_message(a, strm);
    auto              body = resp.expect_one_body_section_op_msg();
    co_return handshake_response::parse(a, body);
}

extern template co_task<handshake_response> handshake(tcp_connection_rw_stream&,
                                                      std::optional<std::string_view>);

}  // namespace amongoc
