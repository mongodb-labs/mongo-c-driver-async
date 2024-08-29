#pragma once

#include "./alloc.h"
#include "./emitter.h"
#include "./loop.h"

#include <mlib/config.h>

struct _amongoc_connection_cxx;
struct bson_view;

typedef struct amongoc_connection amongoc_connection;

mlib_extern_c_begin();

/**
 * @brief Asynchronously connect to a remote server.
 *
 * Upon success, the result object an an amongoc_connection
 *
 * @param loop The event loop to be used
 * @param name The hostname to connect
 * @param svc The port/svc to connect to
 * @return amongoc_emitter
 */
amongoc_emitter
amongoc_conn_connect(amongoc_loop* loop, const char* name, const char* svc) mlib_noexcept;

/// Destroy an amongoc_connection created with amongoc_conn_connect
void amongoc_conn_destroy(amongoc_connection cl) mlib_noexcept;

/**
 * @brief Issue a command on an amongoc_connection. Upon success, resolves
 * with a bson_mut object
 *
 * @param cl The connection on which to send the command
 * @param doc The document to be sent to the server
 * @return amongoc_emitter An emitter that resolves with a bson_mut
 */
amongoc_emitter amongoc_conn_command(amongoc_connection cl, struct bson_view doc) mlib_noexcept;

/**
 * @brief Get the event loop associated with a connection
 *
 * @return A non-null pointer to the event loop used by the connection
 */
amongoc_loop* amongoc_conn_get_event_loop(amongoc_connection cl) mlib_noexcept;

mlib_extern_c_end();

/**
 * @brief An encapsulated amongoc_connection object. This object is pointer-like
 */
struct amongoc_connection {
    struct _amongoc_connection_cxx* _impl;

#if mlib_is_cxx()
    /// Issue a command
    amongoc_emitter command(bson_view const& doc) noexcept;

    // Get the event loop associated with this connection
    amongoc_loop& get_event_loop() const noexcept { return *amongoc_conn_get_event_loop(*this); }

    // Obtain the allocator associated with this connection (from the event loop)
    amongoc::allocator<> get_allocator() const noexcept {
        return this->get_event_loop().get_allocator();
    }
#endif
};

mlib_extern_c_begin();

/**
 * @brief Obtain the memory allocator associated with a connection object
 *
 * @param cl The connection to be queried
 * @return mlib_allocator The allocator for the connection (originates from the event loop)
 */
static inline mlib_allocator amongoc_conn_get_allocator(amongoc_connection cl) mlib_noexcept {
    return amongoc_loop_get_allocator(amongoc_conn_get_event_loop(cl));
}

mlib_extern_c_end();
