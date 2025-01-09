#include <amongoc/amongoc.h>  // Make all APIs visible

#include <stdio.h>
#include <stdlib.h>
// end:headers

amongoc_box on_connect(amongoc_box userdata, amongoc_status* status, amongoc_box result);

int main(void) {
    amongoc_loop loop;
    amongoc_default_loop_init(&loop);

    // Initiate a connection
    amongoc_emitter em = amongoc_client_new(&loop, "mongodb://localhost:27017");
    // Set the continuation
    em = amongoc_then(em, &on_connect);
    // Run the program
    amongoc_detach_start(em);
    amongoc_default_loop_run(&loop);
    // Clean up
    amongoc_default_loop_destroy(&loop);
    return 0;
}

// on_connect def
amongoc_box on_connect(amongoc_box userdata, amongoc_status* status, amongoc_box result) {
    // We don't use the userdata
    (void)userdata;
    // Check for an error
    if (amongoc_is_error(*status)) {
        char* msg = amongoc_status_strdup_message(*status);
        fprintf(stderr, "Error while connecting to server: %s\n", msg);
        free(msg);
    } else {
        printf("Successfully connected!\n");
        amongoc_client* client;
        amongoc_box_take(client, result);
        // `cl` now stores a valid client. We don't do anything else, so just delete it:
        amongoc_client_delete(client);
    }
    amongoc_box_destroy(result);
    return amongoc_nil;
}
// on_connect end
