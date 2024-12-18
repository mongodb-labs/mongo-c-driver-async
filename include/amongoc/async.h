/**
 * @file async.h
 * @brief Asynchronous utilities and algorithms
 * @date 2024-08-06
 *
 * This file contains various asynchronous utilities and algorithms for composing
 * asynchronous operations
 */
#pragma once

#include "./box.h"
#include "./emitter.h"
#include "./loop.h"
#include "./status.h"

#include <mlib/alloc.h>
#include <mlib/config.h>

mlib_extern_c_begin();

/**
 * @brief Function type for the `amongoc_then` transformation callback
 *
 * @param userdata The userdata box given when `amongoc_then` was called
 * @param status The status of the original operation. Can be modified to change
 * the status of the composed result
 * @param value The result value from the input operation.
 *
 * @return The callback should return the new boxed value to become the result
 * of the composed operation
 */
typedef amongoc_box (*amongoc_then_transformer)(amongoc_box     userdata,
                                                amongoc_status* status,
                                                amongoc_box     value);

/**
 * @brief Function type for the `amongoc_let` transformation callback
 *
 * @param userdata The userdata box given when `amongoc_let` was called
 * @param status The status of the input operation.
 * @param value The result value from the input operation.
 *
 * @return The callback should return a new `amongoc_emitter` value to provide the result
 * of the composed operation
 */
typedef amongoc_emitter (*amongoc_let_transformer)(amongoc_box    userdata,
                                                   amongoc_status status,
                                                   amongoc_box    value);

/// Flags to control the behavior of asynchronous utilities
enum amongoc_async_flags {
    // Default option. No special behavior.
    amongoc_async_default = 0,
    /**
     * @brief If given, and the input operation completes with an error status, the
     * transformation function will not be called, instead the result will be forwarded immediately
     * without change
     */
    amongoc_async_forward_errors = 1,
};

/**
 * @brief Transform the result of an asychronous operation after it completes,
 * returning a new operation that resolves with the result of the transformation
 *
 * @param em The input operation to be transformed
 * @param flags Flags to control the then() behavior
 * @param alloc The allocator for for the operation
 * @param userdata Arbitrary userdata that will be forwarded to the transform.
 * @param tr The transformation function to be invoked.
 * @return amongoc_emitter A new emitter that resolves with the result of applying
 * the transform
 *
 * @note On the lifetime of `userdata`: If the transformation function is invoked,
 * then the userdata becomes the resposibility of the transformation callback.
 * If the returned composed operation is otherwise destroyed without ever calling
 * `tr`, then the `userdata` box will be destroyed with `amongoc_box_destroy`.
 */
amongoc_emitter amongoc_then(amongoc_emitter em,
                             enum amongoc_async_flags,
                             mlib_allocator           alloc,
                             amongoc_box              userdata,
                             amongoc_then_transformer tr) mlib_noexcept;

#if mlib_is_cxx()
extern "C++" {
inline amongoc_emitter _amongoc_then_cxx(amongoc_emitter          em,
                                         amongoc_then_transformer tr) mlib_noexcept {
    return ::amongoc_then(em, amongoc_async_default, mlib_default_allocator, amongoc_nil, tr);
}
inline amongoc_emitter _amongoc_then_cxx(amongoc_emitter          em,
                                         amongoc_box              userdata,
                                         amongoc_then_transformer tr) mlib_noexcept {
    return ::amongoc_then(em, amongoc_async_default, mlib_default_allocator, userdata, tr);
}
inline amongoc_emitter _amongoc_then_cxx(amongoc_emitter          em,
                                         amongoc_async_flags      flags,
                                         amongoc_then_transformer tr) mlib_noexcept {
    return ::amongoc_then(em, flags, mlib_default_allocator, amongoc_nil, tr);
}
inline amongoc_emitter _amongoc_then_cxx(amongoc_emitter          em,
                                         mlib_allocator           alloc,
                                         amongoc_box              userdata,
                                         amongoc_then_transformer tr) mlib_noexcept {
    return ::amongoc_then(em, amongoc_async_default, alloc, userdata, tr);
}
inline amongoc_emitter _amongoc_then_cxx(amongoc_emitter          em,
                                         amongoc_async_flags      flags,
                                         amongoc_box              userdata,
                                         amongoc_then_transformer tr) mlib_noexcept {
    return ::amongoc_then(em, flags, mlib_default_allocator, userdata, tr);
}
inline amongoc_emitter _amongoc_then_cxx(amongoc_emitter          em,
                                         amongoc_async_flags      flags,
                                         mlib_allocator           alloc,
                                         amongoc_box              userdata,
                                         amongoc_then_transformer tr) mlib_noexcept {
    return ::amongoc_then(em, flags, alloc, userdata, tr);
}
}
#define amongoc_then(...) _amongoc_then_cxx(__VA_ARGS__)
#else
#define amongoc_then(...) MLIB_PASTE(_amongocThenArgc_, MLIB_ARG_COUNT(__VA_ARGS__))(__VA_ARGS__)
// clang-format off
// 2-args: An emitter and continuation
#define _amongocThenArgc_2(Emitter, Then)                                                          \
    amongoc_then((Emitter), amongoc_async_default, mlib_default_allocator, amongoc_nil, (Then))
// 3-args: One of:
//   - then(em, userdata, cb)
//   - then(em, flags, cb)
#define _amongocThenArgc_3(Emitter, UdOrFlags, Then)                                                  \
    mlib_generic(_amongoc_then_cxx, _amongoc_then_em_ud_cb, (UdOrFlags), \
        amongoc_box:              _amongoc_then_em_ud_cb, \
        enum amongoc_async_flags: _amongoc_then_em_fl_cb \
    )(Emitter, UdOrFlags, Then)
// 4-args: One of:
//      - then(em, alloc, userdata, cb)
//      - then(em, flags, userdata, cb)
#define _amongocThenArgc_4(Emitter, AllocOrFlags, Userdata, Then) \
    mlib_generic(_amongoc_then_cxx, _amongoc_then_em_al_ud_cb, (AllocOrFlags), \
        mlib_allocator:           _amongoc_then_em_al_ud_cb, \
        enum amongoc_async_flags: _amongoc_then_em_fl_ud_cb\
    )((Emitter, AllocOrFlags, Userdata, Then))
#define _amongocThenArgc_5 amongoc_then
// clang-format on
static inline amongoc_emitter _amongoc_then_em_ud_cb(amongoc_emitter          em,
                                                     amongoc_box              userdata,
                                                     amongoc_then_transformer tr) mlib_noexcept {
    return amongoc_then(em, amongoc_async_default, mlib_default_allocator, userdata, tr);
}
static inline amongoc_emitter _amongoc_then_em_fl_cb(amongoc_emitter          em,
                                                     enum amongoc_async_flags fl,
                                                     amongoc_then_transformer tr) mlib_noexcept {
    return amongoc_then(em, fl, mlib_default_allocator, amongoc_nil, tr);
}
static inline amongoc_emitter _amongoc_then_em_al_ud_cb(amongoc_emitter          em,
                                                        mlib_allocator           alloc,
                                                        amongoc_box              userdata,
                                                        amongoc_then_transformer tr) mlib_noexcept {
    return amongoc_then(em, amongoc_async_default, alloc, userdata, tr);
}
static inline amongoc_emitter _amongoc_then_em_fl_ud_cb(amongoc_emitter          em,
                                                        enum amongoc_async_flags flags,
                                                        amongoc_box              userdata,
                                                        amongoc_then_transformer tr) mlib_noexcept {
    return amongoc_then(em, flags, mlib_default_allocator, userdata, tr);
}
#endif

/**
 * @brief Transform the result of an asynchronous operation and continue to a
 * new asynchronous operation.
 *
 * @param em An input operation to be transformed
 * @param flags Flags to control the algorithm behavior
 * @param alloc The allocator for the operation
 * @param userdata Arbitrary userdata that is forwarder to the transformer
 * @param tr The transformation function that to be invoked.
 * @return amongoc_emitter A new emitter for the composed operaiton.
 * The returned emitter will resolve based on the result of the emitter that is returned by the
 * transformer `tr`.
 */
amongoc_emitter amongoc_let(amongoc_emitter          em,
                            enum amongoc_async_flags flags,
                            mlib_allocator           alloc,
                            amongoc_box              userdata,
                            amongoc_let_transformer  tr) mlib_noexcept;

#if mlib_is_cxx()
extern "C++" {
inline amongoc_emitter _amongoc_let_cxx(amongoc_emitter         em,
                                        amongoc_let_transformer tr) mlib_noexcept {
    return ::amongoc_let(em, amongoc_async_default, mlib_default_allocator, amongoc_nil, tr);
}
inline amongoc_emitter _amongoc_let_cxx(amongoc_emitter         em,
                                        amongoc_box             userdata,
                                        amongoc_let_transformer tr) mlib_noexcept {
    return ::amongoc_let(em, amongoc_async_default, mlib_default_allocator, userdata, tr);
}
inline amongoc_emitter _amongoc_let_cxx(amongoc_emitter         em,
                                        amongoc_async_flags     flags,
                                        amongoc_let_transformer tr) mlib_noexcept {
    return ::amongoc_let(em, flags, mlib_default_allocator, amongoc_nil, tr);
}
inline amongoc_emitter _amongoc_let_cxx(amongoc_emitter         em,
                                        mlib_allocator          alloc,
                                        amongoc_box             userdata,
                                        amongoc_let_transformer tr) mlib_noexcept {
    return ::amongoc_let(em, amongoc_async_default, alloc, userdata, tr);
}
inline amongoc_emitter _amongoc_let_cxx(amongoc_emitter         em,
                                        amongoc_async_flags     flags,
                                        amongoc_box             userdata,
                                        amongoc_let_transformer tr) mlib_noexcept {
    return ::amongoc_let(em, flags, mlib_default_allocator, userdata, tr);
}
inline amongoc_emitter _amongoc_let_cxx(amongoc_emitter         em,
                                        amongoc_async_flags     flags,
                                        mlib_allocator          alloc,
                                        amongoc_box             userdata,
                                        amongoc_let_transformer tr) mlib_noexcept {
    return ::amongoc_let(em, flags, alloc, userdata, tr);
}
}
#define amongoc_let(...) _amongoc_let_cxx(__VA_ARGS__)
#else
#define amongoc_let(...) MLIB_PASTE(_amongocLetArgc_, MLIB_ARG_COUNT(__VA_ARGS__))(__VA_ARGS__)
// clang-format off
// 2-args: An emitter and continuation
#define _amongocLetArgc_2(Emitter, Then)                                                          \
    amongoc_let((Emitter), amongoc_async_default, mlib_default_allocator, amongoc_nil, (Then))
// 3-args: One of:
//   - then(em, userdata, cb)
//   - then(em, flags, cb)
#define _amongocLetArgc_3(Emitter, UdOrFlags, Then)                                                  \
    mlib_generic(_amongoc_let_cxx, _amongoc_let_em_ud_cb, (UdOrFlags), \
        amongoc_box:              _amongoc_let_em_ud_cb, \
        enum amongoc_async_flags: _amongoc_let_em_fl_cb \
    )(Emitter, UdOrFlags, Then)
// 4-args: One of:
//      - then(em, alloc, userdata, cb)
//      - then(em, flags, userdata, cb)
#define _amongocLetArgc_4(Emitter, AllocOrFlags, Userdata, Then) \
    mlib_generic(_amongoc_let_cxx, _amongoc_let_em_al_ud_cb, (AllocOrFlags), \
        mlib_allocator:           _amongoc_let_em_al_ud_cb, \
        enum amongoc_async_flags: _amongoc_let_em_fl_ud_cb\
    )((Emitter, AllocOrFlags, Userdata, Then))
#define _amongocLetArgc_5 amongoc_let
// clang-format on
static inline amongoc_emitter _amongoc_let_em_ud_cb(amongoc_emitter         em,
                                                    amongoc_box             userdata,
                                                    amongoc_let_transformer tr) mlib_noexcept {
    return amongoc_let(em, amongoc_async_default, mlib_default_allocator, userdata, tr);
}
static inline amongoc_emitter _amongoc_let_em_fl_cb(amongoc_emitter          em,
                                                    enum amongoc_async_flags fl,
                                                    amongoc_let_transformer  tr) mlib_noexcept {
    return amongoc_let(em, fl, mlib_default_allocator, amongoc_nil, tr);
}
static inline amongoc_emitter _amongoc_let_em_al_ud_cb(amongoc_emitter         em,
                                                       mlib_allocator          alloc,
                                                       amongoc_box             userdata,
                                                       amongoc_let_transformer tr) mlib_noexcept {
    return amongoc_let(em, amongoc_async_default, alloc, userdata, tr);
}
static inline amongoc_emitter _amongoc_let_em_fl_ud_cb(amongoc_emitter          em,
                                                       enum amongoc_async_flags flags,
                                                       amongoc_box              userdata,
                                                       amongoc_let_transformer  tr) mlib_noexcept {
    return amongoc_let(em, flags, mlib_default_allocator, userdata, tr);
}
#endif

/**
 * @brief Create an emitter that resolves immediately with the given status and value
 *
 * @param st The resolve status
 * @param value The result value
 * @return amongoc_emitter An emitter that will immediately resolve to its handler
 * when its associated operation is started.
 */
amongoc_emitter
amongoc_just(amongoc_status st, amongoc_box value, mlib_allocator alloc) mlib_noexcept;

/**
 * @brief Create a continuation that replaces an emitter's result with the given
 * status and result value
 *
 * @param in The input operation to be transformed
 * @param flags Flags to control continuation behavior
 * @param st The new status of the operation
 * @param value The new result value of the operation
 * @param alloc Allocator for state
 * @return amongoc_emitter An emitter that will complete with `st`+`value`
 * according to the behavior set by `flags`.
 */
amongoc_emitter amongoc_then_just(amongoc_emitter          in,
                                  enum amongoc_async_flags flags,
                                  amongoc_status           st,
                                  amongoc_box              value,
                                  mlib_allocator           alloc) mlib_noexcept;

/**
 * @brief Schedule a completion on the given event loop.
 *
 * @param loop The event loop upon which to schedule the completion
 * @return amongoc_emitter An emitter that will call `amongoc_handler_complete()` from
 *      within the event loop. The emitter always resolves with zero status and
 *      an amongoc_nil value
 */
amongoc_emitter amongoc_schedule(amongoc_loop* loop);

/**
 * @brief Schedule the completion of an operation after a duration has elapsed
 *
 * @param loop The event loop on which to schedule the wait
 * @param d The duration to wait
 * @return amongoc_emitter An emitter that will `amongoc_handler_complete` after the
 *      given duration has elapsed.
 *
 * @note The emitter may complete early with non-zero status in the case of an error or cancellation
 */
amongoc_emitter amongoc_schedule_later(amongoc_loop* loop, struct timespec d);

/**
 * @brief Attach a timeout to an operation
 *
 * @param loop The event loop that will handle the timeout
 * @param em The operation to be executed
 * @param d The timeout duration
 * @return amongoc_emitter An emitter that resolves with a result, depending on whether the timeout
 * occurred.
 *
 * If the timeout is hit, then the result status will be ETIMEDOUT and the result value is nothing.
 * Otherwise, the result status and result value from the base operation will be forwarded.
 *
 * If the timeout occurs, then the associated operation will be cancelled.
 */
amongoc_emitter
amongoc_timeout(amongoc_loop* loop, amongoc_emitter em, struct timespec d) mlib_noexcept;

/**
 * @brief Create an emitter that immediately resolves with ENOMEM
 */
amongoc_emitter amongoc_alloc_failure() mlib_noexcept;

/**
 * @brief Create an operation from an emitter which will store the final result
 * status and value of the emitter in the pointed-to locations
 *
 * @param em The emitter to be connected
 * @param status Storage destination for the output status (optional)
 * @param value Storage destination for the output result (optional)
 * @param alloc Allocator to use for dynamic operation state
 * @return amongoc_operation An operation that, when complete, will update `*status`
 * and `*value` with the result of the emitter.
 *
 * If either parameter is a null pointer, then the associated object will be ignored.
 *
 * @note The pointed-to locations must remain valid until the operation is complete or is destroyed
 */
amongoc_operation amongoc_tie(amongoc_emitter em,
                              amongoc_status* status,
                              amongoc_box*    value,
                              mlib_allocator  alloc) mlib_noexcept;

/**
 * @brief Create a "detached" operation from an emitter. This returns a simple operation
 * object that can be started. The final result from the emitter will simply be destroyed
 * when it resolves.
 */
amongoc_operation amongoc_detach(amongoc_emitter emit, mlib_allocator alloc) mlib_noexcept;

mlib_extern_c_end();
