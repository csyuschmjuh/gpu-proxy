
#ifndef GPUPROCESS_SERVER_H
#define GPUPROCESS_SERVER_H

#include "command.h"
#include "compiler_private.h"
#include "egl_states.h"
#include "ring_buffer.h"
#include "server_dispatch_table.h"
#include "thread_private.h"
#include "types_private.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _server server_t;
struct _server {
    server_dispatch_table_t dispatch;

    mutex_t thread_started_mutex;
    buffer_t *buffer;
    thread_t thread;
    bool threaded;
};

private void
server_init (server_t *server,
             buffer_t *buffer);

private server_t *
server_new (buffer_t *buffer);

private bool
server_destroy (server_t *server);

private void
server_start_work_loop (server_t *server);

#ifdef __cplusplus
}
#endif

#endif /* GPUPROCESS_SERVER_H */

