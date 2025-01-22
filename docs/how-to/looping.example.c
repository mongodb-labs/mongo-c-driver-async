#include <amongoc/amongoc.h>

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief State for the program
 */
typedef struct {
    // The countdown to zero
    int countdown;
    // The event loop for the operation
    amongoc_loop* loop;
    // Fibonacci state
    uint64_t a, b;
} state;

/** loop_step()
 * @brief Handle each step of our looping operation
 *
 * @param state_ptr Pointer to the `state` for the program
 */
amongoc_emitter loop_step(amongoc_box state_ptr, amongoc_status prev_status, amongoc_box prev_res) {
    (void)prev_res;
    (void)prev_status;
    // Print our status
    state* s = amongoc_box_cast(state*, state_ptr);
    // Compute the next sum and shift our numbers
    uint64_t cur = s->a;
    uint64_t sum = s->a + s->b;
    s->a         = s->b;
    s->b         = sum;
    fprintf(stderr, "%d seconds remain, current value: %lu\n", s->countdown, cur);
    // Check if we are done
    if (s->countdown == 0) {
        // No more looping to do. Return a final result
        return amongoc_just(amongoc_box_uint64(cur));
    }
    // Decrement the counter and start a sleep of one second
    --s->countdown;
    struct timespec dur = {.tv_sec = 1};
    amongoc_emitter em  = amongoc_schedule_later(s->loop, dur);
    // Connect the sleep to this function so that we will be called again after
    // the delay has elapsed. Return this as the new operation for the loop.
    return amongoc_let(em, amongoc_async_forward_errors, state_ptr, loop_step);
}
// end.

int main(int argc, char const* const* argv) {
    // Parse a delay integer from the command-line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <delay>\n", argv[0]);
        return 2;
    }
    int delay = atoi(argv[1]);
    if (((delay == 0) && strcmp(argv[1], "0")) || delay < 0) {
        fprintf(stderr, "Expected <delay> to be a positive integer\n");
        return 2;
    }

    // Create a default loop
    amongoc_loop loop;
    amongoc_default_loop_init(&loop);

    // Seed the initial sum
    state app_state = {.countdown = delay, .loop = &loop, .a = 0, .b = 1};

    // Start the loop
    amongoc_emitter em = loop_step(amongoc_box_pointer(&app_state), amongoc_okay, amongoc_nil);
    // Tie the final result for later, and start the program
    amongoc_status    status;
    amongoc_box       result;
    amongoc_operation op = amongoc_tie(em, &status, &result, mlib_default_allocator);
    amongoc_start(&op);
    // Run the program within the event loop
    amongoc_default_loop_run(&loop);
    amongoc_operation_delete(op);
    amongoc_default_loop_destroy(&loop);

    if (amongoc_is_error(status)) {
        amongoc_declmsg(msg, status);
        fprintf(stderr, "error: %s\n", msg);
        amongoc_box_destroy(result);
        return 2;
    } else {
        // Get the value returned with `amongoc_just` in `loop_step`
        printf("Got final value: %lu\n", amongoc_box_cast(uint64_t, result));
    }
    return 0;
}
