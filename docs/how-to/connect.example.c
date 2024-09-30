#include <amongoc/amongoc.h>

/**
 * @brief Shared state for the application. This is passed through the app as pointer stored
 * in a box
 */
typedef struct app_state {
    // The connection to a server
    amongoc_client client;
} app_state;

/**
 * @brief Write the content of a BSON document in JSON-like format to the given output
 */
static void print_bson(FILE* into, bson_view doc, const char* indent);

/** after_hello()
 * @brief Handle the `hello` response from the server
 *
 * @param state_ptr Pointer to the `app_state`
 * @param resp_data A `bson_mut` object that contains the response message
 * @return amongoc_box Returns `amongoc_nil`
 */
amongoc_box after_hello(amongoc_box state_ptr, amongoc_status*, amongoc_box resp_data) {
    (void)state_ptr;
    bson_view resp = bson_view_of(amongoc_box_cast(bson_mut)(resp_data));
    // Just print the response message
    fprintf(stdout, "Got response: ");
    print_bson(stdout, resp, "");
    fputs("\n", stdout);
    amongoc_box_destroy(resp_data);
    return amongoc_nil;
}
// end.

/** after_connect_say_hello()
 * @brief Handle the connection to a server. Sends a "hello" message
 *
 * @param state_ptr Pointer to the `app_state`
 * @param cl_box An `amongoc_client`
 * @return amongoc_emitter
 */
amongoc_emitter after_connect_say_hello(amongoc_box state_ptr, amongoc_status, amongoc_box cl_box) {
    printf("Connected to server\n");
    // Store the connection in our app state
    amongoc_box_take(amongoc_box_cast(app_state*)(state_ptr)->client, cl_box);

    // Create a "hello" command
    bson_mut doc = bson_mut_new();
    bson_insert_utf8(&doc,
                     bson_begin(doc),
                     bson_utf8_view_from_cstring("hello"),
                     bson_utf8_view_from_cstring("1"));
    bson_insert_utf8(&doc,
                     bson_end(doc),
                     bson_utf8_view_from_cstring("$db"),
                     bson_utf8_view_from_cstring("test"));
    amongoc_emitter em = amongoc_client_command(amongoc_box_cast(app_state*)(state_ptr)->client,
                                                bson_view_of(doc));
    bson_mut_delete(doc);

    em = amongoc_then(em,
                      amongoc_async_forward_errors,
                      mlib_default_allocator,
                      state_ptr,
                      after_hello);
    return em;
}
// end.

int main(int argc, char const* const* argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <uri>\n", argv[0]);
        return 1;
    }
    const char* const uri = argv[1];

    amongoc_loop loop;
    amongoc_default_loop_init(&loop);

    struct app_state state = {0};

    amongoc_emitter em = amongoc_client_new(&loop, uri);
    struct timespec dur;
    dur.tv_sec  = 5;
    dur.tv_nsec = 0;
    em          = amongoc_timeout(&loop, em, dur);

    em = amongoc_let(em,
                     amongoc_async_forward_errors,
                     mlib_default_allocator,
                     amongoc_box_pointer(&state),
                     after_connect_say_hello);

    amongoc_status    fin_status = amongoc_okay;
    amongoc_operation op         = amongoc_tie(em, &fin_status, NULL, mlib_default_allocator);
    amongoc_start(&op);
    amongoc_default_loop_run(&loop);
    amongoc_operation_destroy(op);

    // Destroy the connection since we are done with it (this is a no-op for a null connection)
    amongoc_client_destroy(state.client);
    amongoc_default_loop_destroy(&loop);

    if (amongoc_is_error(fin_status)) {
        char* m = amongoc_status_strdup_message(fin_status);
        fprintf(stderr, "An error occurred: %s\n", m);
        free(m);
        return 2;
    } else {
        printf("Okay\n");
        return 0;
    }
}
// end.

// Concat two strings, allocating storage dynamically (must free the returned string)
static char* astrcat(const char* a, const char* b) {
    size_t len = strlen(a) + strlen(b);
    char*  buf = (char*)calloc(len + 1, 1);
    strcpy(buf, a);
    strcat(buf, b);
    return buf;
}

static void print_bson(FILE* into, bson_view doc, const char* indent) {
    bson_iterator it = bson_begin(doc);
    fprintf(into, "{\n");
    for (; !bson_iterator_done(it); it = bson_next(it)) {
        bson_utf8_view str = bson_iterator_key(it);
        fprintf(into, "%s  \"%s\": ", indent, str.data);
        switch (bson_iterator_type(it)) {
        case BSON_TYPE_EOD:
        case BSON_TYPE_DOUBLE:
            fprintf(into, "%f,\n", bson_iterator_double(it));
            break;
        case BSON_TYPE_UTF8:
            fprintf(into, "\"%s\",\n", bson_iterator_utf8(it).data);
            break;
        case BSON_TYPE_DOCUMENT:
        case BSON_TYPE_ARRAY: {
            char*     i2     = astrcat(indent, "  ");
            bson_view subdoc = bson_iterator_document(it, NULL);
            print_bson(into, subdoc, "  ");
            free(i2);
            fprintf(into, ",\n");
            break;
        }
        case BSON_TYPE_UNDEFINED:
            fprintf(into, "[undefined],\n");
            break;
        case BSON_TYPE_BOOL:
            fprintf(into, bson_iterator_bool(it) ? "true,\n" : "false,\n");
            break;
        case BSON_TYPE_NULL:
            fprintf(into, "null,\n");
            break;
        case BSON_TYPE_INT32:
            fprintf(into, "%d,\n", bson_iterator_int32(it));
            break;
        case BSON_TYPE_INT64:
            fprintf(into, "%ld,\n", bson_iterator_int64(it));
            break;
        case BSON_TYPE_TIMESTAMP:
        case BSON_TYPE_DECIMAL128:
        case BSON_TYPE_MAXKEY:
        case BSON_TYPE_MINKEY:
        case BSON_TYPE_OID:
        case BSON_TYPE_BINARY:
        case BSON_TYPE_DATE_TIME:
        case BSON_TYPE_REGEX:
        case BSON_TYPE_DBPOINTER:
        case BSON_TYPE_CODE:
        case BSON_TYPE_SYMBOL:
        case BSON_TYPE_CODEWSCOPE:
            fprintf(into, "[[printing unimplemented for this type]],\n");
            break;
        }
    }
    fprintf(into, "%s}", indent);
}
