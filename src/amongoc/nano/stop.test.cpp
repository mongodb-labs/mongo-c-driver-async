#include "./stop.hpp"

#include "./util.hpp"

#include <catch2/catch_test_macros.hpp>

#include <thread>

using namespace amongoc;

static_assert(stoppable_token<in_place_stop_token>);
static_assert(stoppable_source<in_place_stop_source>);
static_assert(stoppable_token<null_stop_token>);

TEST_CASE("Stop/Simple") {
    in_place_stop_source stop;
    bool                 did_stop = false;
    auto                 tk       = stop.get_token();
    auto                 cb       = create_stop_callback(tk, [&] {
        CHECK_FALSE(did_stop);
        did_stop = true;
    });
    CHECK_FALSE(stop.stop_requested());
    CHECK_FALSE(did_stop);
    CHECK(stop.request_stop());
    CHECK(did_stop);
    CHECK_FALSE(stop.request_stop());  // Should not call the callback again
}

TEST_CASE("Stop/Scoped Callbacks") {
    in_place_stop_source stop;
    bool                 did_stop = false;
    {
        auto cb = create_stop_callback(stop.get_token(), [&] { did_stop = true; });
        // Callback is destroyed and disconnected
    }
    stop.request_stop();
    // The callback was disconnected, so it will not execute when we requested the stop
    CHECK_FALSE(did_stop);
}

TEST_CASE("Stop/Executes Immediately") {
    in_place_stop_source stop;
    stop.request_stop();
    bool did_stop = false;
    auto cb       = create_stop_callback(stop.get_token(), [&] {
        CHECK_FALSE(did_stop);
        did_stop = true;
    });
    CHECK(did_stop);
    CHECK_FALSE(stop.request_stop());
}

struct stopper {
    std::function<void()> fn;

    void operator()() { fn(); }
};

TEST_CASE("Stop/Disconnect During Stop") {
    in_place_stop_source                                         stop;
    std::optional<stop_callback_t<in_place_stop_token, stopper>> cb;
    bool                                                         did_stop = false;

    cb.emplace(defer_convert([&] {
        return create_stop_callback(stop.get_token(), stopper{[&] {
                                        cb.reset();
                                        did_stop = true;
                                    }});
    }));

    SECTION("Same-thread") {
        CHECK(stop.request_stop());
        CHECK(did_stop);
        CHECK_FALSE(cb.has_value());
    }
}

TEST_CASE("Stop/Racing Stop") {
    for (int i = 0; i < 30; ++i) {
        in_place_stop_source                                         stop;
        std::optional<stop_callback_t<in_place_stop_token, stopper>> cb;
        std::atomic_bool                                             did_stop = false;
        cb.emplace(defer_convert([&] {
            return create_stop_callback(stop.get_token(), stopper{[&] {
                                            cb.reset();
                                            did_stop = true;
                                        }});
        }));
        std::thread thr{[&] {
            bool stopped = stop.request_stop();
            assert(stopped);
            assert(did_stop);
        }};
        while (not stop.stop_requested()) {
            ;  // spin
        }
        while (not did_stop) {
            ;  // spin
        }
        REQUIRE_FALSE(cb.has_value());
        thr.join();
    }
}

TEST_CASE("Stop/Racing Stop 2") {
    for (int i = 0; i < 1'0; ++i) {
        in_place_stop_source                                         stop;
        std::optional<stop_callback_t<in_place_stop_token, stopper>> cb;
        std::atomic_bool                                             did_stop = false;
        cb.emplace(defer_convert([&] {
            return create_stop_callback(stop.get_token(), stopper{[&] { did_stop = true; }});
        }));
        std::thread thr{[&] {
            // Destroy the callback in a separate thread from the one that is requesting the stop
            cb.reset();
        }};
        CHECK(stop.request_stop());
        // It is unspecified whether the callback executed.
        thr.join();
    }
}

TEST_CASE("Stop/Bind Token") {
    in_place_stop_source src;
    bool                 called = false;
    auto                 b      = bind_stop_token(src.get_token(), [&]() { called = true; });
    CHECK(get_stop_token(b) == src.get_token());
    b();
    CHECK(called);
}
