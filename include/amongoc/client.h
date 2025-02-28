#pragma once

#include "./emitter.h"
#include "./loop.h"

#include <mlib/alloc.h>
#include <mlib/config.h>
#include <mlib/str.h>

struct bson_view;

typedef struct amongoc_client amongoc_client;

mlib_extern_c_begin();

/**
 * @brief Asynchronously create a new client for a remote
 *
 * Upon success, the result object an an `amongoc_client` pointer
 *
 * @param loop The event loop to be used
 * @param uri The connection URI string that specifies that peer and connection options
 */
amongoc_emitter amongoc_client_new(amongoc_loop* loop, mlib_str_view uri) mlib_noexcept;
#define amongoc_client_new(Loop, URI) amongoc_client_new((Loop), mlib_str_view_from((URI)))

/// Destroy an amongoc_client created with amongoc_client_new
void amongoc_client_delete(amongoc_client* cl) mlib_noexcept;
mlib_assoc_deleter(amongoc_client*, amongoc_client_delete);

/**
 * @brief Issue a command on an amongoc_client. Upon success, resolves
 * with a bson_doc object
 *
 * @param cl The connection on which to send the command
 * @param doc The document to be sent to the server
 * @return amongoc_emitter An emitter that resolves with a bson_doc
 */
amongoc_emitter amongoc_client_command(amongoc_client* cl, struct bson_view doc) mlib_noexcept;
amongoc_emitter amongoc_client_command_nocopy(amongoc_client*  conn,
                                              struct bson_view doc) mlib_noexcept;

/**
 * @brief Get the event loop associated with a client
 *
 * @return A non-null pointer to the event loop used by the connection
 */
amongoc_loop* amongoc_client_get_event_loop(const amongoc_client* cl) mlib_noexcept;

/**
 * @brief Obtain the memory allocator associated with a client object
 *
 * @param cl The client to be queried
 * @return mlib_allocator The allocator for the client (originates from the event loop)
 */
inline mlib_allocator amongoc_client_get_allocator(amongoc_client const* cl) mlib_noexcept {
    return amongoc_loop_get_allocator(amongoc_client_get_event_loop(cl));
}

mlib_extern_c_end();
