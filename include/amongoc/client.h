#pragma once

#include "./config.h"
#include "./emitter.h"
#include "./loop.h"

struct _amongoc_client_cxx;
struct bson_view;

typedef struct amongoc_client amongoc_client;

AMONGOC_EXTERN_C_BEGIN

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
amongoc_client_connect(amongoc_loop* loop, const char* name, const char* svc) AMONGOC_NOEXCEPT;

/// Destroy an amongoc_client created with amongoc_client_connect
void amongoc_client_destroy(amongoc_client cl) AMONGOC_NOEXCEPT;

/**
 * @brief Issue a command on an amongoc_client. Upon success, resolves
 * with a bson_mut object
 *
 * @param cl The client on which to send the command
 * @param doc The document to be sent to the server
 * @return amongoc_emitter An emitter that resolves with a bson_mut
 */
amongoc_emitter amongoc_client_command(amongoc_client cl, struct bson_view doc) AMONGOC_NOEXCEPT;

AMONGOC_EXTERN_C_END

/**
 * @brief An encapsulated amongoc_client object. This object is pointer-like
 */
struct amongoc_client {
    struct _amongoc_client_cxx* _impl;

#ifdef __cplusplus
    /// Issue a command
    amongoc_emitter command(bson_view const& doc) noexcept;
#endif
};
