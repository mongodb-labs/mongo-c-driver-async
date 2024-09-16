#include "./uri.hpp"

#include <amongoc/event_emitter.hpp>
#include <amongoc/format.hpp>
#include <amongoc/nano/util.hpp>
#include <amongoc/status.h>
#include <amongoc/string.hpp>

#include <mlib/alloc.h>

#include <boost/url.hpp>
#include <fmt/chrono.h>  // Enable duration formatting
#include <neo/overload.hpp>
#include <neo/tokenize.hpp>
#include <neo/utility.hpp>

#include <cerrno>
#include <chrono>

using namespace amongoc;
using namespace std::literals;
namespace sr = std::ranges;
using boost::urls::pct_string_view;
namespace sys = boost::system;

namespace {
// A status value of EINVAL
const auto status_einval = status(&amongoc_generic_category, EINVAL);

// Convert a boost::system::error_code to an amongoc_status
amongoc_status boost_err_to_status(sys::error_code ec) noexcept {
    if (ec.category() == sys::system_category()) {
        return status(&amongoc_system_category, ec.value());
    } else if (ec.category() == sys::generic_category()) {
        return status(&amongoc_generic_category, ec.value());
    } else if (ec.category().name() == "boost.url.grammar"sv) {
        // URL parsing error
        return status_einval;
    } else {
        return status(&amongoc_unknown_category, ec.value());
    }
}

// Boost.URL StringToken to create strings with our mlib::alocator
struct make_string final : boost::urls::string_token::arg {
    using result_type = amongoc::string;

    make_string(mlib::allocator<> a)
        : buf(a) {}

    result_type buf;

    char* prepare(std::size_t n) {
        buf.resize(n);
        return buf.data();
    }

    result_type result() noexcept { return std::move(buf); }
};
}  // namespace

result<connection_uri> connection_uri::parse(std::string_view                        url,
                                             event_emitter<uri_warning_event> const& warn,
                                             mlib::allocator<> alloc) noexcept {
    sys::result<boost::url_view> got = boost::urls::parse_uri(url);
    if (not got.has_value()) {
        return boost_err_to_status(got.error());
    }
    auto e_scheme = got->scheme();
    if (e_scheme != "mongodb") {
        return status::from(std::errc::protocol_not_supported);
    }
    auto e_host = got->encoded_host();
    if (e_host.empty()) {
        return status_einval;
    }
    if (e_host.find("/") != e_host.npos) {
        return status_einval;
    }
    if (not got->port().empty() and got->port_number() == 0) {
        // Zero ports are not allowed
        return status_einval;
    }

    // The connection URI object that will be returned
    connection_uri ret{alloc};

    // A status value that may be updated during parameter parsing
    status param_parse_status = amongoc_okay;

    // Convert a string-view to a string using the current in-scope allocator
    auto string_from_view = [alloc](auto sv) { return string(sv, alloc); };

    // Split the given string view on commas. Returns a range of tokens
    auto comma_split = [](pct_string_view sv) {
        using is_comma = after<std::equal_to<>, ct_constant<','>>;
        using splitter = neo::simple_token_splitter<neo::charclass_splitter<is_comma>>;
        auto tokens    = neo::tokenizer(auto(sv), splitter{});
        return std::views::transform(auto(tokens), [](auto sub) {
            return pct_string_view(sub.data(), sub.size());
        });
    };

    // Parse a comma-separated list of key-value pairs
    auto parse_mapping = [&](pct_string_view sv) -> map<string, string> {
        map<string, string> ret(alloc);
        auto                parts = comma_split(sv);

        for (pct_string_view part : parts) {
            auto            col_pos = part.find(':');
            pct_string_view key     = part.substr(0, col_pos);
            pct_string_view val     = part.substr(col_pos + 1);
            ret.emplace(key.decode({}, make_string(alloc)),  //
                        val.decode({}, make_string(alloc)));
        }

        return ret;
    };

    // Parse a comma-separated list of strings
    auto parse_seq = [&](pct_string_view sv) -> vector<string> {
        auto           words = comma_split(sv);
        vector<string> ret(alloc);
        sr::copy(std::views::transform(words, string_from_view), std::back_inserter(ret));
        return ret;
    };

    // Handle the `w` parameter specially
    auto handle_w = [&](auto w) -> std::variant<string, int> {
        int  integer;
        auto rc = std::from_chars(w.data(), w.data() + w.size(), integer);
        if (rc.ec != std::errc{} or integer < 0) {
            return string(w, alloc);
        } else {
            return integer;
        }
    };

    // Create a function that appends its arguments to the given sequence container
    auto append_to
        = [](auto& out) { return [&out](auto&& val) { out.emplace_back(mlib_fwd(val)); }; };

    // TODO: Reject param strings that are not key-value pairs
    for (auto qp : got->encoded_params()) {
        auto try_parse_int = [&](pct_string_view sv) -> std::optional<int> {
            int  ret;
            auto res = std::from_chars(sv.data(), sv.data() + sv.size(), ret);
            if (res.ec != std::errc{}) {
                warn.fire(defer_convert([&] {
                    return uri_warning_event(
                        format(alloc, "URI parameter {}: Invalid integer value '{}'", qp.key, sv));
                }));
                return {};
            }
            return ret;
        };

        auto try_parse_bool = [&](pct_string_view val) -> std::optional<bool> {
            if (val == "true") {
                return true;
            } else if (val == "false") {
                return false;
            }
            const std::string_view trues[]   = {"true", "1", "yes", "y", "t"};
            const std::string_view falses[]  = {"false", "0", "-1", "no", "n", "f"};
            auto                   eq_to_val = std::bind_front(std::equal_to{}, val);
            const auto             eq_true   = std::views::transform(trues, eq_to_val);
            const auto             eq_false  = std::views::transform(falses, eq_to_val);
            if (any(eq_true)) {
                return true;
            } else if (any(eq_false)) {
                return false;
            } else {
                warn.fire(defer_convert([&] {
                    return uri_warning_event(
                        format(alloc,
                               "URI parameter {}: Invalid boolean constant “{}”",
                               qp.key,
                               val));
                }));
            }
            return {};  // nullopt
        };

        // Special handler for the `tls`/`ssl` parameter
        auto toggle_tls = [&](auto sv) {
            auto b = try_parse_bool(sv);
            if (not b.has_value()) {
                return;
            }
            if (ret.params.tls.has_value() and *ret.params.tls != *b) {
                // Conflicting values of the TLS option
                param_parse_status = status_einval;
            }
            ret.params.tls = b;
        };

        // Create a clamping function that generates a warning if the bounds are violated
        auto clamp_and_warn = [&]<typename I>(I min, I max) {
            return [=](I ival) -> I {
                if (ival < min or ival > max) {
                    warn.fire(defer_convert([&] {
                        return uri_warning_event(
                            format(alloc,
                                   "URI parameter “{}”: Value {} is outside the supported range "
                                   "(min: {}, max: {})",
                                   qp.key,
                                   ival,
                                   min,
                                   max));
                    }));
                }
                return std::clamp(ival, min, max);
            };
        };

        // Assigns to an optional, generating a warning if the optional already has a value.
        auto opt_assign = [&]<typename T>(std::optional<T>& out) {
            return [&](T value) {
                if (out.has_value()) {
                    warn.fire(defer_convert([&] {
                        if constexpr (fmt::formattable<T>) {
                            return uri_warning_event(
                                format(alloc,
                                       "URI parameter “{}” was specified multiple "
                                       "times (previously “{}”, now “{}”)",
                                       qp.key,
                                       *out,
                                       value));
                        } else {
                            return uri_warning_event(
                                format(alloc,
                                       "URI parameter “{}” was specified multiple "
                                       "times",
                                       qp.key));
                        }
                    }));
                }
                out = mlib_fwd(value);
                return neo::unit{};
            };
        };

        // Parse a positive duration value
        auto try_parse_duration_ms
            = atop(opt_fmap<construct<std::chrono::milliseconds>>{},
                   atop(opt_fmap(clamp_and_warn(0, INT_MAX)), try_parse_int));

        // Handle a parameter with the given expected key, and an associated callback.
        // returns `true` if the parameter was recognized, otherwise `false`
        auto handle_param = [&](std::string_view check_key, auto&& callback) {
            auto icase_char_eq = over(std::equal_to{}, ascii_tolower);
            if (sr::equal(check_key, qp.key, icase_char_eq)) {
                callback(qp.value);
                return true;
            }
            return false;
        };

        if (false  //
            or handle_param("appname", atop(opt_assign(ret.params.appname), string_from_view))
            or handle_param("authMechanism",
                            atop(opt_assign(ret.params.authMechanism), string_from_view))
            or handle_param("authMechanismProperties",
                            atop(assign(ret.params.authMechanismProperties), parse_mapping))
            or handle_param("authSource", atop(opt_assign(ret.params.authSource), string_from_view))
            or handle_param("connectTimeoutMS",
                            atop(assign(ret.params.connectTimeoutMS), try_parse_duration_ms))
            or handle_param("compressors", atop(assign(ret.params.compressors), parse_seq))
            or handle_param("directConnection",
                            atop(opt_fmap(opt_assign(ret.params.directConnection)), try_parse_bool))
            or handle_param("heartbeatFrequencyMS",
                            atop(opt_fmap(opt_assign(ret.params.heartbeatFrequencyMS)),
                                 atop(opt_fmap(clamp_and_warn(500ms,
                                                              std::chrono::milliseconds(INT_MAX))),
                                      try_parse_duration_ms)))
            or handle_param("journal",
                            atop(opt_fmap(opt_assign(ret.params.journal)), try_parse_bool))
            or handle_param("loadBalanced",
                            atop(opt_fmap(opt_assign(ret.params.loadBalanced)), try_parse_bool))
            or handle_param("maxPoolSize",
                            atop(opt_fmap(opt_assign(ret.params.maxPoolSize)), try_parse_int))
            or handle_param("minPoolSize",
                            atop(opt_fmap(opt_assign(ret.params.minPoolSize)), try_parse_int))
            or handle_param("maxIdleTimeMS",
                            atop(opt_fmap(opt_assign(ret.params.maxIdleTimeMS)),
                                 try_parse_duration_ms))
            or handle_param("replicaSet", atop(opt_assign(ret.params.replicaSet), string_from_view))
            or handle_param("readConcernLevel",
                            atop(opt_assign(ret.params.readConcernLevel), string_from_view))
            or handle_param("readPreferenceTags",
                            atop(append_to(ret.params.readPreferenceTags), parse_mapping))
            or handle_param("tls", toggle_tls)                                                    //
            or handle_param("tlsInsecure", atop(assign(ret.params.tlsInsecure), try_parse_bool))  //
            or handle_param("timeoutMS",
                            atop(opt_fmap(opt_assign(ret.params.timeoutMS)), try_parse_duration_ms))
            or handle_param("ssl", toggle_tls)
            or handle_param("w", atop(opt_assign(ret.params.w), handle_w))
            or handle_param("wTimeout",
                            atop(opt_fmap(opt_assign(ret.params.wTimeoutMS)),
                                 try_parse_duration_ms))
            or handle_param("wTimeoutMS",
                            atop(opt_fmap(opt_assign(ret.params.wTimeoutMS)),
                                 try_parse_duration_ms))  //
            or handle_param("zlibCompressionLevel",
                            atop(opt_fmap(opt_assign(ret.params.zlibCompressionLevel)),
                                 atop(opt_fmap(clamp_and_warn(-1, 9)), try_parse_int)))  //
        ) {
            // Handled this parameter
        } else {
            // Unknown parameter name
            warn.fire(defer_convert([&] -> uri_warning_event {
                return uri_warning_event{
                    amongoc::format(alloc, "Unknown URI parameter “{}”", qp.key)};
            }));
        }
        if (param_parse_status.is_error()) {
            // There was a hard-error during parameter parsing. Stop now.
            return param_parse_status;
        }
    }

    if (got->has_userinfo()) {
        // We need to add auth to this URI
        uri_auth auth{alloc};
        auth.username = got->user(make_string(alloc));
        auth.password = got->password(make_string(alloc));
        ret.auth      = mlib_fwd(auth);
        if (got->encoded_path() != neo::oper::any_of("/", "")) {
            // Non-empty path. This is the database we will use for auth
            ret.auth->database.emplace(got->path(make_string(alloc)));
            ret.auth->database->erase(ret.auth->database->begin());
        }
    }
    // TODO: Multi-host URIs
    uri_host h;
    // Init the port
    h.port = got->has_port() ? got->port_number() : -1;
    // Init the host
    switch (got->host_type()) {
    case boost::urls::host_type::name:
        h.host.emplace<string>(got->host(make_string(alloc)));
        break;
    case boost::urls::host_type::ipv4:
        h.host.emplace<uri_host::v4_address>(got->host_ipv4_address().to_uint());
        break;
    case boost::urls::host_type::ipv6:
        h.host = uri_host::v6_address(got->host_ipv6_address().to_bytes());
        break;
    case boost::urls::host_type::none:
    case boost::urls::host_type::ipvfuture:
        // XXX: When might this actually happen?
        std::terminate();
        break;
    }
    return mlib_fwd(ret);
}
