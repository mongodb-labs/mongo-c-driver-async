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
#include "./config.h"
#include "./emitter.h"
#include "./loop.h"
#include "./status.h"

AMONGOC_EXTERN_C_BEGIN

/**
 * @brief Function type for the amongoc_then transformation callback
 *
 * @param userdata The userdata box given when amongoc_then was called
 * @param status The status of the original operation. Can be modified to change
 * the status of the composed result
 * @param value The result value from the input operation. It is the function's
 * responsibility to destroy this box.
 *
 * @return The callback should return the new boxed value to become the result
 * of the composed operation
 */
typedef amongoc_box (*amongoc_then_transformer)(amongoc_box     userdata,
                                                amongoc_status* status,
                                                amongoc_box     value) AMONGOC_NOEXCEPT;

typedef amongoc_emitter (*amongoc_let_transformer)(amongoc_box    userdata,
                                                   amongoc_status status,
                                                   amongoc_box    value) AMONGOC_NOEXCEPT;

/// Flags to control the behavior of amongoc_then()
enum amongoc_then_flags {
    // Default option for then(). No special behavior
    amongoc_then_default = 0,
    /**
     * @brief If given, and the input operation completes with a non-zero result
     * status, the transformation function will not be called, instead the result
     * will be forwarded immediately without change
     */
    amongoc_then_forward_errors = 1,
};

/**
 * @brief Transform the result of an asychronous operation after it completes,
 * returning a new operation that resolves with the result of the transformation
 *
 * @param em The input operation to be transformed
 * @param flags Flags to control the then() behavior
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
amongoc_emitter amongoc_then(amongoc_emitter          em,
                             enum amongoc_then_flags  flags,
                             amongoc_box              userdata,
                             amongoc_then_transformer tr) AMONGOC_NOEXCEPT;

amongoc_emitter amongoc_let(amongoc_emitter em,
                            enum amongoc_then_flags,
                            amongoc_box             userdata,
                            amongoc_let_transformer tr) AMONGOC_NOEXCEPT;

/**
 * @brief Attach a timeout to an operation
 *
 * @param loop The event loop that will handle the timeout
 * @param em The operation to be executed
 * @param timeout_us The timeout duration (microseconds)
 * @return amongoc_emitter An emitter that resolves with a result, depending on whether the timeout
 * occurred.
 *
 * If the timeout is hit, then the result status will be ETIMEDOUT and the result value is nothing.
 * Otherwise, the result status and result value from the base operation will be forwarded.
 *
 * If the timeout occurs, then the associated operation will be cancelled.
 */
amongoc_emitter
amongoc_timeout_us(amongoc_loop* loop, amongoc_emitter em, int64_t timeout_us) AMONGOC_NOEXCEPT;

/**
 * @brief Create an emitter that resolves immediately with the given status and value
 *
 * @param st The resolve status
 * @param value The result value
 * @return amongoc_emitter An emitter that will immediately resolve to its handler
 * when its associated operation is started.
 */
amongoc_emitter amongoc_just(amongoc_status st, amongoc_box value) AMONGOC_NOEXCEPT;

/**
 * @brief Create a "detached" operation from an emitter. This returns a simple operation
 * object that can be started. The final result from the emitter will simply be destroyed
 * when it resolves.
 */
amongoc_operation amongoc_detach(amongoc_emitter emit) AMONGOC_NOEXCEPT;

/**
 * @brief Create an operation from an emitter which will store the final result
 * status and value of the emitter in the pointed-to locations
 *
 * @param em The emitter to be connected
 * @param status Storage destination for the output status (optional)
 * @param value Storage destination for the output result (optional)
 * @return amongoc_operation An operation than, when complete, will update `*status`
 * and `*value` with the result of the emitter.
 *
 * If either parameter is a null pointer, then the associated object will be ignored.
 *
 * @note The pointed-to locations must remain valid until the operation is complete or is destroyed
 */
amongoc_operation
amongoc_tie(amongoc_emitter em, amongoc_status* status, amongoc_box* value) AMONGOC_NOEXCEPT;

AMONGOC_EXTERN_C_END
