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
                                                amongoc_box     value) mlib_noexcept;

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
                                                   amongoc_box    value) mlib_noexcept;

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
