#pragma once

#include <amongoc/alloc.h>
#include <amongoc/event_emitter.hpp>
#include <amongoc/map.hpp>
#include <amongoc/nano/result.hpp>
#include <amongoc/string.hpp>
#include <amongoc/vector.hpp>

#include <chrono>
#include <cstdint>
#include <variant>

namespace amongoc {

enum class server_monitoring_mode {
    stream,
    poll,
    auto_,
};

/**
 * @brief Event fired during connection URI parsing
 */
struct uri_warning_event {
    // The message for the event
    string message;
};

/**
 * @brief Parameters specifying a URI's host
 */
struct uri_host {
    struct v4_address {
        std::uint32_t v;
    };
    struct v6_address {
        std::array<std::uint8_t, 16> bytes;
    };
    // The host. Either an IPv4 address, and IPv6 address, or a hostname string
    std::variant<v4_address, v6_address, string> host;
    // The port number
    int port;
};

/**
 * @brief Parameters for a URI's authentication
 */
struct uri_auth {
    explicit uri_auth(allocator<> a) noexcept
        : username(a) {}

    // URI username
    string username;
    // URI password
    string password{username.get_allocator()};
    // URI auth database, if specified
    std::optional<string> database;
};

/**
 * @brief Connection parameters specified by the uri-options spec.
 *
 * Not all of these are implemented, but all options are present so that the
 * generate test cases will at least compile.
 *
 * The parameter names are camelCase names in order to match their canonical
 * spelling within the URI string.
 */
struct connection_params {
    explicit connection_params(allocator<> a) noexcept
        // Only construct one object using the allocator. Other parameter objects will inherit this
        // allocator
        : compressors(a) {}

    // note: Keep these alphabetized

    std::optional<string>                    appname;
    std::optional<string>                    authMechanism;
    std::optional<map<string, string>>       authMechanismProperties;
    std::optional<string>                    authSource;
    vector<string>                           compressors;
    std::optional<std::chrono::milliseconds> connectTimeoutMS;
    std::optional<bool>                      directConnection;
    std::optional<std::chrono::milliseconds> heartbeatFrequencyMS;
    std::optional<bool>                      journal;
    std::optional<bool>                      loadBalanced;
    std::optional<std::chrono::milliseconds> localThresholdMS;
    std::optional<unsigned>                  maxConnecting;
    std::optional<std::chrono::milliseconds> maxIdleTimeMS;
    std::optional<int>                       maxPoolSize;
    std::optional<std::chrono::seconds>      maxStalenessSeconds;
    std::optional<int>                       minPoolSize;
    std::optional<string>                    proxyHost;
    std::optional<unsigned>                  proxyPort;
    std::optional<string>                    proxyUsername;
    std::optional<string>                    proxyPassword;
    std::optional<string>                    readConcernLevel;
    std::optional<string>                    readPreference;
    vector<map<string, string>>              readPreferenceTags{compressors.get_allocator()};
    std::optional<string>                    replicaSet;
    std::optional<bool>                      retryReads;
    std::optional<bool>                      retryWrites;
    std::optional<server_monitoring_mode>    serverMonitoringMode;
    std::optional<std::chrono::milliseconds> serverSelectionTimeoutMS;
    std::optional<bool>                      serverSelectionTryOnce;
    std::optional<std::chrono::milliseconds> timeoutMS;
    std::optional<std::chrono::milliseconds> socketTimeoutMS;
    std::optional<unsigned>                  srvMaxHosts;
    std::optional<string>                    srvServiceName;
    std::optional<bool>                      tls;
    std::optional<bool>                      tlsAllowInvalidCertificates;
    std::optional<bool>                      tlsAllowInvalidHostnames;
    std::optional<string>                    tlsCAFile;
    std::optional<string>                    tlsCertificateKeyFile;
    std::optional<string>                    tlsCertificateKeyFilePassword;
    std::optional<bool>                      tlsDisableCertificateRevocationCheck;
    std::optional<bool>                      tlsDisableOCSPEndpointCheck;
    std::optional<bool>                      tlsInsecure;
    std::optional<std::variant<string, int>> w;
    std::optional<std::chrono::milliseconds> waitQueueTimeoutMS;
    std::optional<std::chrono::milliseconds> wTimeoutMS;
    std::optional<int>                       zlibCompressionLevel;
};

/**
 * @brief A parsed connection URI
 */
class connection_uri {
public:
    // Default-construct an empty URI using the given allocator
    explicit connection_uri(allocator<> a) noexcept
        : hosts(a) {}

    /**
     * @brief Parse a URI string
     *
     * @param uri The URI to be parsed
     * @param alloc An allocator for the object state
     */
    static result<connection_uri> parse(std::string_view uri, mlib::allocator<> alloc) noexcept {
        return parse(uri, {}, alloc);
    }

    /**
     * @brief Parse a URI string, and dispatch warning events to the given event emitter
     *
     * @param uri The URI string to parse
     * @param ev An event emitter that will receive warnings during parsing
     * @param alloc AN allocator for the object state
     */
    static result<connection_uri> parse(std::string_view                        uri,
                                        event_emitter<uri_warning_event> const& ev,
                                        allocator<>                             alloc) noexcept;

    // Obtain the allocator associated with this object
    mlib::allocator<> get_allocator() const noexcept { return hosts.get_allocator(); }

    // List of hosts specified by the connection URI
    std::vector<uri_host, mlib::allocator<uri_host>> hosts;
    // Auth parameters for the URI
    std::optional<uri_auth> auth;
    // Other connection parameters for the URI
    connection_params params{get_allocator()};
};

}  // namespace amongoc
