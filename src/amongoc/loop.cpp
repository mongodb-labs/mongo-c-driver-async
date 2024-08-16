#include <amongoc/box.h>
#include <amongoc/emitter.h>
#include <amongoc/handler.h>
#include <amongoc/loop.h>
#include <amongoc/operation.h>
#include <amongoc/status.h>

#include "./nano/simple.hpp"

using namespace amongoc;

emitter amongoc_schedule_later(amongoc_loop* loop, int64_t duration_us) {
    return unique_emitter::from_connector([=](unique_handler h) {
               return unique_operation::from_starter([=, h = AM_FWD(h)] mutable {
                   loop->vtable->call_later(loop, duration_us, amongoc_nil, h.release());
               });
           })
        .release();
}

emitter amongoc_schedule(amongoc_loop* loop) { return as_emitter(loop->schedule()).release(); }
