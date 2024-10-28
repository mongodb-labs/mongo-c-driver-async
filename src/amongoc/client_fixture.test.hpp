#include <amongoc/async.h>
#include <amongoc/box.h>
#include <amongoc/client.h>
#include <amongoc/default_loop.h>
#include <amongoc/emitter_result.h>
#include <amongoc/loop_fixture.test.hpp>
#include <amongoc/operation.h>
#include <amongoc/status.h>

#include <mlib/alloc.h>

#include <catch2/catch_test_macros.hpp>

#include <test_params.test.hpp>

#include <system_error>

namespace amongoc::testing {

struct client_fixture : loop_fixture {
    amongoc_client client{nullptr};

    ~client_fixture() { amongoc_client_delete(client); }

    client_fixture() {
        auto          uri = parameters.require_uri();
        auto          em  = amongoc_client_new(&loop.get(), uri.data());
        std::timespec dur{};
        dur.tv_sec = 3;
        em         = ::amongoc_timeout(&loop.get(), em, dur);
        auto r     = this->run_to_completion(em);
        if (r.status.is_error()) {
            throw std::system_error(r.status.as_error_code(), r.status.message());
        }
        this->client = r.value.take<amongoc_client>();
    }
};

}  // namespace amongoc::testing
