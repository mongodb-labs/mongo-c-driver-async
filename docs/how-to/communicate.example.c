#include <amongoc/amongoc.h>

#include "bson/view.h"
#include <bson/iterator.h>
#include <bson/mut.h>

#include "mlib/str.h"

/**
 * @brief Shared state for the application. This is passed through the app as pointer stored
 * in a box
 */
typedef struct app_state {
    // The connection to a server
    amongoc_client* client;
} app_state;

/**
 * @brief Write the content of a BSON document in JSON-like format to the given output
 */
static void print_bson(FILE* into, bson_view doc, mlib_str_view indent);

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
    print_bson(stdout, resp, mlib_str_view_from(""));
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

    amongoc_if_error (status, msg) {
        fprintf(stderr, "An error occurred: %s\n", msg);
        return 2;
    } else {
        printf("Okay\n");
        return 0;
    }
}
// end.

static void print_bson(FILE* into, bson_view doc, mlib_str_view indent) {
    fprintf(into, "{\n");
    bson_foreach(it, doc) {
        mlib_str_view str = bson_key(it);
        fprintf(into, "%*s  \"%s\": ", (int)indent.len, indent.data, str.data);
        bson_value_ref val = bson_iterator_value(it);
        switch (val.type) {
        case bson_type_eod:
        case bson_type_double:
            fprintf(into, "%f,\n", val.double_);
            break;
        case bson_type_utf8:
            fprintf(into, "\"%s\",\n", val.utf8.data);
            break;
        case bson_type_document:
        case bson_type_array: {
            mlib_str  i2     = mlib_str_append(indent, "  ");
            bson_view subdoc = bson_iterator_value(it).document;
            print_bson(into, subdoc, mlib_str_view_from(i2));
            mlib_str_delete(i2);
            fprintf(into, ",\n");
            break;
        }
        case bson_type_undefined:
            fprintf(into, "[undefined],\n");
            break;
        case bson_type_bool:
            fprintf(into, val.bool_ ? "true,\n" : "false,\n");
            break;
        case bson_type_null:
            fprintf(into, "null,\n");
            break;
        case bson_type_int32:
            fprintf(into, "%d,\n", val.int32);
            break;
        case bson_type_int64:
            fprintf(into, "%ld,\n", val.int64);
            break;
        case bson_type_timestamp:
        case bson_type_decimal128:
        case bson_type_maxkey:
        case bson_type_minkey:
        case bson_type_oid:
        case bson_type_binary:
        case bson_type_datetime:
        case bson_type_regex:
        case bson_type_dbpointer:
        case bson_type_code:
        case bson_type_symbol:
        case bson_type_codewscope:
            fprintf(into, "[[printing unimplemented for this type]],\n");
            break;
        }
    }
    fprintf(into, "%*s}", (int)indent.len, indent.data);
}
