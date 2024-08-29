#pragma once

#include "./alloc.h"
#include "./emitter.h"
#include "./loop.h"

#include <mlib/config.h>

struct _amongoc_client_cxx;
struct bson_view;

typedef struct amongoc_client amongoc_client;

mlib_extern_c_begin();

/**
 * @brief Asynchronously connect a new client to a remote server.
 *
 * Upon success, the result object an an amongoc_client
 *
 * @param loop The event loop to be used
 * @param name The hostname to connect
 * @param svc The port/svc to connect to
 * @return amongoc_emitter
 */
amongoc_emitter
amongoc_client_connect(amongoc_loop* loop, const char* name, const char* svc) mlib_noexcept;

/// Destroy an amongoc_client created with amongoc_client_connect
void amongoc_client_destroy(amongoc_client cl) mlib_noexcept;

/**
 * @brief Issue a command on an amongoc_client. Upon success, resolves
 * with a bson_mut object
 *
 * @param cl The client on which to send the command
 * @param doc The document to be sent to the server
 * @return amongoc_emitter An emitter that resolves with a bson_mut
 */
amongoc_emitter amongoc_client_command(amongoc_client cl, struct bson_view doc) mlib_noexcept;

/**
 * @brief Get the event loop associated with a client
 *
 * @return A non-null pointer to the event loop used by the client
 */
amongoc_loop* amongoc_client_get_event_loop(amongoc_client cl) mlib_noexcept;

mlib_extern_c_end();

/**
 * @brief An encapsulated amongoc_client object. This object is pointer-like
 */
struct amongoc_client {
    struct _amongoc_client_cxx* _impl;

#if mlib_is_cxx()
    /// Issue a command
    amongoc_emitter command(bson_view const& doc) noexcept;

    // Get the event loop associated with this client
    amongoc_loop& get_event_loop() const noexcept { return *amongoc_client_get_event_loop(*this); }

    // Obtain the allocator associated with this client (from the event loop)
    amongoc::cxx_allocator<> get_allocator() const noexcept {
        return this->get_event_loop().get_allocator();
    }
#endif
};

mlib_extern_c_begin();

/**
 * @brief Obtain the memory allocator associated with a client object
 *
 * @param cl The client to be queried
 * @return amongoc_allocator The allocator for the client (originates from the event loop)
 */
static inline amongoc_allocator amongoc_client_get_allocator(amongoc_client cl) mlib_noexcept {
    return amongoc_loop_get_allocator(amongoc_client_get_event_loop(cl));
}

mlib_extern_c_end();
