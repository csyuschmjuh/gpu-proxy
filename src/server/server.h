
#ifndef GPUPROCESS_SERVER_H
#define GPUPROCESS_SERVER_H

#include "command.h"
#include "compiler_private.h"
#include "dispatch_private.h"
#include "egl_states.h"
#include "ring_buffer.h"
#include "thread_private.h"
#include "types_private.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _server server_t;
typedef struct _server {
    server_dispatch_table_t dispatch;

    buffer_t *buffer;
    thread_t thread;
    bool threaded;
} server_t;

private void
server_init (server_t *server,
             buffer_t *buffer,
             bool threaded);

private server_t *
server_new (buffer_t *buffer);

private bool
server_destroy (server_t *server);

#ifdef __cplusplus
}
#endif

#endif /* GPUPROCESS_SERVER_H */

