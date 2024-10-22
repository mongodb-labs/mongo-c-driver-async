#[[
Generates test cases for connection URIs using the spec tests pulled from the
specifications repository.
]]
include(Specifications)

set(gen_dir "${CMAKE_CURRENT_BINARY_DIR}/uri-tests.gen")
file(MAKE_DIRECTORY "${gen_dir}")

set(generated_sources)

function(__code_line line)
    string(CONFIGURE "${line}" line @ONLY)
    set(__code "${__code}\n${line}" PARENT_SCOPE)
endfunction()

function(__json_get out)
    string(JSON tmp GET "${__json}" ${ARGN})
    set("${out}" "${tmp}" PARENT_SCOPE)
endfunction()


set(GenURITests_HEAD [[// clang-format off
// I AM A GENERATED FILE: DO NOT EDIT ME!

#include <amongoc/uri.hpp>
#include <amongoc/test_util.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

using namespace amongoc;

inline std::variant<string, int> w_parameter(int n) {
    return n;
}

inline std::variant<string, int> w_parameter(std::string_view sv) {
    return string(sv, ::mlib_default_allocator);
}

inline void _check_map_item_eq(std::optional<map<string, string>>& omap, std::string_view key, std::string_view expect_value) {
  CHECKED_IF(omap.has_value()) {
    INFO("Checking map member");
    CAPTURE(key);
    CAPTURE(expect_value);
    auto got = omap->find(key);
    CHECKED_IF(got != omap->end()) {
      CHECK(got->second == expect_value);
    }
  }
}

inline void _check_seq_eq(auto& seq, std::initializer_list<std::initializer_list<std::pair<std::string_view, std::string_view>>> pairs) {
    CHECK_THAT(seq, Catch::Matchers::RangeEquals(pairs, [](auto& left_pairs, auto& right_pairs) {
        return std::ranges::equal(left_pairs, right_pairs, [](auto&& pair1, auto&& pair2) -> bool {
            return pair1.first == pair2.first and pair1.second == pair2.second;
        });
    }));
}
inline void _check_seq_eq(auto& seq, std::initializer_list<std::string_view> strings) {
    CHECK_THAT(seq, Catch::Matchers::RangeEquals(strings));
}

consteval server_monitoring_mode smm(std::string_view sv) {
    if (sv == "auto") {
        return server_monitoring_mode::auto_;
    } else if (sv == "stream") {
        return server_monitoring_mode::stream;
    } else if (sv == "poll") {
        return server_monitoring_mode::poll;
    } else {
        throw;
    }
}
]])

set(__param_canon_spelling [[{
    "appname": "appname",
    "authmechanism": "authMechanism",
    "authmechanismproperties": "authMechanismProperties",
    "authsource": "authSource",
    "compressors": "compressors",
    "connecttimeoutms": "connectTimeoutMS",
    "directconnection": "directConnection",
    "heartbeatfrequencyms": "heartbeatFrequencyMS",
    "journal": "journal",
    "loadbalanced": "loadBalanced",
    "localthresholdms": "localThresholdMS",
    "maxconnecting": "maxConnecting",
    "maxidletimems": "maxIdleTimeMS",
    "maxpoolsize": "maxPoolSize",
    "maxstalenessseconds": "maxStalenessSeconds",
    "minpoolsize": "minPoolSize",
    "proxyhost": "proxyHost",
    "proxyport": "proxyPort",
    "proxyusername": "proxyUsername",
    "proxypassword": "proxyPassword",
    "readconcernlevel": "readConcernLevel",
    "readpreference": "readPreference",
    "readpreferencetags": "readPreferenceTags",
    "replicaset": "replicaSet",
    "retryreads": "retryReads",
    "retrywrites": "retryWrites",
    "servermonitoringmode": "serverMonitoringMode",
    "serverselectiontimeoutms": "serverSelectionTimeoutMS",
    "serverselectiontryonce": "serverSelectionTryOnce",
    "timeoutms": "timeoutMS",
    "sockettimeoutms": "socketTimeoutMS",
    "srvmaxhosts": "srvMaxHosts",
    "srvservicename": "srvServiceName",
    "tls": "tls",
    "tlsallowinvalidcertificates": "tlsAllowInvalidCertificates",
    "tlsallowinvalidhostnames": "tlsAllowInvalidHostnames",
    "tlscafile": "tlsCAFile",
    "tlscertificatekeyfile": "tlsCertificateKeyFile",
    "tlscertificatekeyfilepassword": "tlsCertificateKeyFilePassword",
    "tlsdisablecertificaterevocationcheck": "tlsDisableCertificateRevocationCheck",
    "tlsdisableocspendpointcheck": "tlsDisableOCSPEndpointCheck",
    "tlsinsecure": "tlsInsecure",
    "w": "w",
    "waitqueuetimeoutms": "waitQueueTimeoutMS",
    "wtimeoutms": "wTimeoutMS",
    "zlibcompressionlevel": "zlibCompressionLevel"
}]])
set(__param_constructors [[{
    "appname": "",
    "authMechanism": "",
    "authMechanismProperties": "map<string, string>",
    "authSource": "",
    "compressors": "vector<string>",
    "connectTimeoutMS": "std::chrono::milliseconds",
    "directConnection": "bool",
    "heartbeatFrequencyMS": "std::chrono::milliseconds",
    "journal": "bool",
    "loadBalanced": "bool",
    "localThresholdMS": "std::chrono::milliseconds",
    "maxConnecting": "unsigned",
    "maxIdleTimeMS": "std::chrono::milliseconds",
    "maxPoolSize": "unsigned",
    "maxStalenessSeconds": "std::chrono::seconds",
    "minPoolSize": "unsigned",
    "proxyHost": "",
    "proxyPort": "unsigned",
    "proxyUsername": "",
    "proxyPassword": "",
    "readConcernLevel": "",
    "readPreference": "",
    "readPreferenceTags": "map<string, string>",
    "replicaSet": "",
    "retryReads": "bool",
    "retryWrites": "bool",
    "serverMonitoringMode": "smm",
    "serverSelectionTimeoutMS": "std::chrono::milliseconds",
    "serverSelectionTryOnce": "bool",
    "timeoutMS": "std::chrono::milliseconds",
    "socketTimeoutMS": "std::chrono::milliseconds",
    "srvMaxHosts": "unsigned",
    "srvServiceName": "",
    "tls": "bool",
    "tlsAllowInvalidCertificates": "bool",
    "tlsAllowInvalidHostnames": "bool",
    "tlsCAFile": "",
    "tlsCertificateKeyFile": "",
    "tlsCertificateKeyFilePassword": "",
    "tlsDisableCertificateRevocationCheck": "bool",
    "tlsDisableOCSPEndpointCheck": "bool",
    "tlsInsecure": "bool",
    "w": "w_parameter",
    "waitQueueTimeoutMS": "std::chrono::milliseconds",
    "wTimeoutMS": "std::chrono::milliseconds",
    "zlibCompressionLevel": "int"
}]])

function(__uri_test_gen group __json out)
    string(APPEND CMAKE_MESSAGE_INDENT "  ")
    list(APPEND CMAKE_MESSAGE_CONTEXT "${group}")
    __json_get(desc "description")
    message(DEBUG "Test: “${desc}”")
    __json_get(uri "uri")
    __json_get(expect_valid "valid")
    __json_get(expect_warning "warning")
    __json_get(expect_auth "auth")
    __json_get(expect_options "options")
    __json_get(expect_hosts "hosts")
    set(__code "")
    string(APPEND __code "TEST_CASE(\"URI/spec/${group}/${desc}\", \"[uri][uri/${group}]\") {")
    __code_line([[  auto uri_cstr = "@uri@";]])
    __code_line([[  CAPTURE(uri_cstr);]])
    __code_line([[  event_emitter<uri_warning_event> warn;]])
    __code_line([[  bool got_warning = false;]])
    __code_line([[  auto _ = warn.listen(atop(assign(got_warning), constant(true)));]])
    __code_line([[  auto uri = connection_uri::parse(uri_cstr, warn, ::mlib_default_allocator);]])
    if(expect_hosts)
        string(JSON num_hosts LENGTH "${expect_hosts}")
        if(num_hosts GREATER 1)
            __code_line([[  SKIP("TODO: Multi-host URIs");]])
        endif()
    endif()
    if(NOT expect_valid)
        __code_line([[  REQUIRE(uri.has_error());]])
        # Early-return: The URI is invalid
        __code_line("}")
        set("${out}" "${__code}" PARENT_SCOPE)
        return()
    endif()
    # TODO: Handle tests for warnings
    if(expect_warning)
        __code_line([[  CHECK(got_warning);]])
    else()
        __code_line([[  CHECK_FALSE(got_warning);]])
    endif()
    # Check that the URI information matches expectations
    __code_line([[  REQUIRE(uri.has_value());]])
    if(expect_hosts)
        string(JSON num_hosts LENGTH "${expect_hosts}")
        if(NOT num_hosts EQUAL 1)
            __code_line([[  SKIP("Unimplemented: Multi-host URI parsing");]])
        endif()
    endif()
    if(NOT expect_auth STREQUAL "")
        set(__json "${expect_auth}")
        __json_get(expect_username "username")
        __json_get(expect_password "password")
        __json_get(expect_db "db")
        __code_line([[  CHECK(uri->auth.has_value());]])
        __code_line([[  if (uri->auth.has_value()) {]])
        __code_line([[    CHECK(uri->auth->username == "@expect_username@");]])
        __code_line([[    CHECK(uri->auth->password == "@expect_password@");]])
        if(NOT expect_db)
            __code_line([[    CHECK_FALSE(uri->auth->database.has_value());]])
        else()
            __code_line([[    CHECK(uri->auth.has_value());]])
            __code_line([[    CHECK(uri->auth->database == "@expect_db@");]])
        endif()
        __code_line([[  }]])
    else()
        # XXX: WE won't actually want to check that auth is null, because some
        # spec tests have "null" even though the URI conains auth values
        ## __code_line([[  CHECK_FALSE(uri->auth.has_value());]])
    endif()
    if(expect_options AND NOT expect_options STREQUAL "{}")
        set(__json "${expect_options}")
        string(JSON len LENGTH "${expect_options}")
        math(EXPR len "${len}-1")
        foreach(nth RANGE "${len}")
            string(JSON param MEMBER "${expect_options}" "${nth}")
            string(JSON value GET "${expect_options}" "${param}")
            string(JSON ty TYPE "${expect_options}" "${param}")
            string(TOLOWER "${param}" param_lower)
            string(JSON param_canon GET "${__param_canon_spelling}" "${param_lower}")
            string(JSON ctor GET "${__param_constructors}" "${param_canon}")
            if(ty STREQUAL "BOOLEAN")
                # Do explicit boolean comparisons for parameters that are optional<bool>
                if(value)
                    __code_line([[  CHECK(uri->params.@param_canon@ == true);]])
                else()
                    __code_line([[  CHECK(uri->params.@param_canon@ == false);]])
                endif()
            elseif(ty STREQUAL "STRING")
                __code_line([[  CHECK(uri->params.@param_canon@ == @ctor@("@value@"));]])
            elseif(ty STREQUAL "NUMBER")
                __code_line([[  CHECK(uri->params.@param_canon@ == @ctor@(@value@));]])
            elseif(ty STREQUAL "ARRAY")
                # Replace each key-value pair in an object with a braced pair
                string(REGEX REPLACE [[("[^"]*") : ("[^"]*")]] [[{ \1, \2 }]] value "${value}")
                # Replace square brackets with braces
                string(REGEX REPLACE "^\\[(.*)\\]$" "{\\1}" ilist "${value}")
                # Fold newlines
                string(REGEX REPLACE "\n *" " " ilist "${ilist}")
                __code_line([[  _check_seq_eq(uri->params.@param_canon@, @ilist@);]])
            elseif(ty STREQUAL "OBJECT")
                string(JSON len LENGTH "${value}")
                math(EXPR len "${len}-1")
                foreach(sub_nth RANGE "${len}")
                    string(JSON key MEMBER "${value}" "${sub_nth}")
                    string(JSON subval GET "${value}" "${key}")
                    __code_line([[  _check_map_item_eq(uri->params.@param_canon@, "@key@", "@subval@");]])
                endforeach()
            else()
                __code_line([[  SKIP("Unimplemented: Validation of '@param_canon@' URI parameter");]])
            endif()
        endforeach()
    endif()
    __code_line("}")
    set("${out}" "${__code}" PARENT_SCOPE)
endfunction()

#[[
    Generate the content of a C++ test TU content based on the given JSON spec string
]]
function(generate_uri_test_cpp_content out spec_json)
    string(JSON tests GET "${spec_json}" "tests")
    string(JSON len LENGTH "${tests}")
    set(__code "${GenURITests_HEAD}")
    # Loop over each test
    math(EXPR end "${len}-1")
    foreach(nth RANGE "${end}")
        string(JSON one_test GET "${tests}" "${nth}")
        __uri_test_gen("${stem}" "${one_test}" more)
        string(APPEND __code "\n\n${more}")
    endforeach()
endfunction()

#[[
    Generate a URI test file ${out_cpp} based on the content of ${test_json}
]]
function(generate_uri_tests_file out_cpp_path test_json_path)
    message(VERBOSE "Generating [${test_cpp}]")
    message(DEBUG   "  Reading from [${test_json_path}]")
    file(READ "${test_json_file}" spec_json)
    generate_uri_test_cpp_content(content "${spec_json}")
    string(JSON tests GET "${spec_json}" "tests")
    string(JSON len LENGTH "${tests}")
    set(__code "${GenURITests_HEAD}")
    # Loop over each test
    math(EXPR end "${len}-1")
    foreach(nth RANGE "${end}")
        string(JSON one_test GET "${tests}" "${nth}")
        __uri_test_gen("${stem}" "${one_test}" more)
        string(APPEND __code "\n\n${more}")
    endforeach()
    file(WRITE "${test_cpp}" "${__code}")
endfunction()

function(__gen_tests out)
    list(APPEND CMAKE_MESSAGE_CONTEXT "GenURITests")
    set(ret)
    foreach(test_json_file IN LISTS ARGN)
        # Set the name of the file that will contain the tests
        cmake_path(GET test_json_file STEM stem)
        cmake_path(APPEND gen_dir "${stem}.test.cpp" OUTPUT_VARIABLE test_cpp)
        message(VERBOSE "Generating: [${test_cpp}]")
        message(DEBUG   "  From [${test_json_file}]")
        list(APPEND ret "${test_cpp}")
        # Read the tests
        generate_uri_tests_file("${test_cpp}" "${test_json_file}")

    endforeach()
    set("${out}" "${ret}" PARENT_SCOPE)
endfunction()

function(generate_uri_test out_dir)
endfunction()


file(
    GLOB tests_json CONFIGURE_DEPENDS
    "${Specifications_SOURCE_DIR}/source/connection-string/tests/*.json"
    "${Specifications_SOURCE_DIR}/source/uri-options/tests/*.json"
    )
__gen_tests(URITest_SOURCES ${tests_json})
