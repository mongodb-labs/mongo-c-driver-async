#pragma once

#include <amongoc/handler.hpp>
#include <amongoc/loop.h>

struct amongoc_loop::sched_snd {
    amongoc_loop* loop;

    using sends_type = decltype(nullptr);

    template <typename R>
    struct op {
        amongoc_loop*           loop;
        [[no_unique_address]] R _recv;

        void start() noexcept {
            loop->vtable->call_soon(  //
                loop,
                amongoc_okay,
                amongoc_nil,
                amongoc::unique_handler::from(loop->get_allocator(),
                                              [this](amongoc::emitter_result&&) mutable {
                                                  static_cast<R&&>(_recv)(nullptr);
                                              })
                    .release());
        }
    };

    template <typename R>
    op<R> connect(R&& r) const noexcept {
        return op<R>{loop, mlib_fwd(r)};
    }
};

constexpr amongoc_loop::sched_snd amongoc_loop::schedule() noexcept { return {this}; }
