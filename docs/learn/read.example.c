#include "amongoc/async.h"
#include "amongoc/box.h"
#include "amongoc/client.h"
#include "amongoc/collection.h"
#include "amongoc/default_loop.h"
#include <amongoc/amongoc.h>

#include "bson/format.h"

// Application state
typedef struct {
    // The client
    amongoc_client* client;
    // The collection we are reading from
    amongoc_collection* collection;
    // The name of the database we read from
    const char* db_name;
    // The name of the collection within the database
    const char* coll_name;
    int         batch_num;
} app_state;

static amongoc_emitter on_connect(amongoc_box state, amongoc_status status, amongoc_box client);

int main(int argc, char** argv) {
    // Program parameters
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <uri> <database> <collection>\n", argv[0]);
        return 2;
    }
    // The server URI
    const char* const uri = argv[1];

    // Initialize our state
    app_state state = {
        .db_name   = argv[2],
        .coll_name = argv[3],
    };

    amongoc_loop loop;
    amongoc_if_error (amongoc_default_loop_init(&loop), msg) {
        fprintf(stderr, "Error initializing the event loop: %s\n", msg);
        return 1;
    }

    // Connect
    amongoc_emitter em = amongoc_client_new(&loop, uri);

    // Set the continuation
    em = amongoc_let(em, amongoc_async_forward_errors, amongoc_box_pointer(&state), on_connect);

    // Run the program and collect the result
    amongoc_status    status;
    amongoc_operation op = amongoc_tie(em, &status);
    amongoc_start(&op);
    amongoc_default_loop_run(&loop);
    amongoc_operation_delete(op);
    amongoc_collection_delete(state.collection);
    amongoc_client_delete(state.client);
    amongoc_default_loop_destroy(&loop);

    // Check for errors
    amongoc_if_error (status, msg) {
        fprintf(stderr, "Error: %s\n", msg);
        return 1;
    }
}

static amongoc_emitter on_find(amongoc_box state_, amongoc_status status, amongoc_box cursor_);

// Continuation after connection completes
static amongoc_emitter on_connect(amongoc_box state_, amongoc_status status, amongoc_box client) {
    // We don't use the status
    (void)status;
    // Store the client object
    app_state* state = amongoc_box_cast(app_state*, state_);
    amongoc_box_take(state->client, client);
    // Create a new collection handle
    state->collection = amongoc_collection_new(state->client, state->db_name, state->coll_name);
    // Initiate a read operation.
    amongoc_emitter em = amongoc_find(state->collection, bson_view_null, NULL);
    return amongoc_let(em, amongoc_async_forward_errors, state_, on_find);
}

// Continuation when we get data
static amongoc_emitter on_find(amongoc_box state_, amongoc_status status, amongoc_box cursor_) {
    // We don't use the status
    (void)status;
    app_state* state = amongoc_box_cast(app_state*, state_);
    // Extract the cursor
    amongoc_cursor cursor;
    amongoc_box_take(cursor, cursor_);

    // Print the data
    fprintf(stderr,
            "Got results from database '%s' collection '%s', batch %d: ",
            state->db_name,
            state->coll_name,
            state->batch_num);
    bson_write_repr(stderr, cursor.records);
    fprintf(stderr, "\n");
    state->batch_num++;

    // Do we have more data?
    if (cursor.cursor_id) {
        // More to find
        amongoc_emitter em = amongoc_cursor_next(cursor);
        return amongoc_let(em, amongoc_async_forward_errors, state_, on_find);
    } else {
        // Done
        amongoc_cursor_delete(cursor);
        return amongoc_just();
    }
}
// end:on_find
