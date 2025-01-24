#include <amongoc/amongoc.h>

#include <stdio.h>

// Application state
typedef struct {
    // The client object
    amongoc_client* client;
    // The collection object which we will write into
    amongoc_collection* coll;
} app_state;

static amongoc_emitter on_connect(amongoc_box state, amongoc_status status, amongoc_box client);

int main() {
    amongoc_loop   loop;
    amongoc_status status = amongoc_default_loop_init(&loop);
    amongoc_if_error (status, msg) {
        fprintf(stderr, "Failed to initialize event loop: %s\n", msg);
        return 1;
    }
    // Connect
    amongoc_emitter em = amongoc_client_new(&loop, "mongodb://localhost:27017");

    // Application state object
    app_state state = {0};

    // Set the continuation for the connection:
    em = amongoc_let(em,                            // [1]
                     amongoc_async_forward_errors,  // [2]
                     amongoc_box_pointer(&state),   // [3]
                     on_connect);                   // [4]

    amongoc_operation op = amongoc_tie(em, &status);
    amongoc_start(&op);
    amongoc_default_loop_run(&loop);

    // Clean up:
    amongoc_operation_delete(op);
    amongoc_collection_delete(state.coll);
    amongoc_client_delete(state.client);
    amongoc_default_loop_destroy(&loop);

    // Print the final result
    amongoc_if_error (status, msg) {
        fprintf(stderr, "An error occurred: %s\n", msg);
        return 1;
    }
    printf("okay\n");
    return 0;
}

// Continuation after connection completes
static amongoc_emitter on_connect(amongoc_box state_, amongoc_status status, amongoc_box client) {
    // We don't use the status here
    (void)status;
    // Store the client from the connect operation
    app_state* const state = amongoc_box_cast(app_state*, state_);
    amongoc_box_take(state->client, client);

    // Create a new collection handle
    state->coll = amongoc_collection_new(state->client, "write-test-db", "main");
    if (!state->coll) {
        return amongoc_alloc_failure();
    }

    // Create a document to be inserted
    // Data: { "foo": "bar", "answer": 42 }
    bson_doc doc = bson_new();
    if (!bson_data(doc)) {
        return amongoc_alloc_failure();
    }
    bson_mut mut = bson_mutate(&doc);
    bson_insert(&mut, "foo", "bar");
    bson_insert(&mut, "answer", 42);
    // Insert the single document:
    amongoc_emitter insert_em = amongoc_insert_one(state->coll, doc);
    // Delete our copy of the doc
    bson_delete(doc);
    // Tell the runtime to continue into the next operation:
    return insert_em;
}
// end:on_connect
