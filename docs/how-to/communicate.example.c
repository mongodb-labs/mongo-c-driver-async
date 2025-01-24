#include <amongoc/amongoc.h>

#include <bson/format.h>
#include <bson/iterator.h>
#include <bson/mut.h>
#include <bson/view.h>

/**
 * @brief Shared state for the application. This is passed through the app as pointer stored
 * in a box
 */
typedef struct app_state {
    // The connection to a server
    amongoc_client* client;
} app_state;

/** after_hello()
 * @brief Handle the `hello` response from the server
 *
 * @param state_ptr Pointer to the `app_state`
 * @param resp_data A `bson_mut` object that contains the response message
 * @return amongoc_box Returns `amongoc_nil`
 */
amongoc_box after_hello(amongoc_box state_ptr, amongoc_status*, amongoc_box resp_data) {
    (void)state_ptr;
    bson_view resp = bson_view_from(amongoc_box_cast(bson_doc, resp_data));
    // Just print the response message
    fprintf(stdout, "Got response: ");
    bson_write_repr(stderr, resp);
    fputs("\n", stdout);
    amongoc_box_destroy(resp_data);
    return amongoc_nil;
}
// end.

/** after_connect_say_hello()
 * @brief Handle the connection to a server. Sends a "hello" message
 *
 * @param state_ptr Pointer to the `app_state`
 * @param cl_box An `amongoc_client*`
 * @return amongoc_emitter
 */
amongoc_emitter after_connect_say_hello(amongoc_box state_ptr, amongoc_status, amongoc_box cl_box) {
    printf("Connected to server\n");
    // Store the connection in our app state
    amongoc_box_take(amongoc_box_cast(app_state*, state_ptr)->client, cl_box);

    // Create a "hello" command
    bson_doc doc = bson_new();
    bson_mut mut = bson_mutate(&doc);
    bson_insert(&mut, "hello", "1");
    bson_insert(&mut, "$db", "test");
    amongoc_emitter em = amongoc_client_command(amongoc_box_cast(app_state*, state_ptr)->client,
                                                bson_view_from(mut));
    bson_delete(doc);

    em = amongoc_then(em, amongoc_async_forward_errors, state_ptr, after_hello);
    return em;
}
// end.

int main(int argc, char const* const* argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <uri>\n", argv[0]);
        return 1;
    }
    const char* const uri = argv[1];

    amongoc_loop   loop;
    amongoc_status status = amongoc_default_loop_init(&loop);
    amongoc_if_error (status, msg) {
        fprintf(stderr, "Error setting up the event loop: %s\n", msg);
        return 2;
    }

    struct app_state state = {0};

    amongoc_emitter em = amongoc_client_new(&loop, uri);
    struct timespec dur;
    dur.tv_sec  = 5;
    dur.tv_nsec = 0;
    em          = amongoc_timeout(&loop, em, dur);

    em = amongoc_let(em,
                     amongoc_async_forward_errors,
                     amongoc_box_pointer(&state),
                     after_connect_say_hello);

    amongoc_operation op = amongoc_tie(em, &status);
    amongoc_start(&op);
    amongoc_default_loop_run(&loop);
    amongoc_operation_delete(op);

    // Destroy the connection since we are done with it (this is a no-op for a null connection)
    amongoc_client_delete(state.client);
    amongoc_default_loop_destroy(&loop);

    // Final status
    amongoc_if_error (status, msg) {
        fprintf(stderr, "An error occurred: %s\n", msg);
        return 2;
    } else {
        printf("Okay\n");
        return 0;
    }
}
// end.
