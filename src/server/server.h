
#ifndef GPUPROCESS_SERVER_H
#define GPUPROCESS_SERVER_H

typedef struct _server server_t;

#include "command.h"
#include "compiler_private.h"
#include "egl_states.h"
#include "hash.h"
#include "ring_buffer.h"
#include "dispatch_table.h"
#include "thread_private.h"
#include "types_private.h"
#include <pthread.h>

typedef void (*command_handler_t)(server_t *server, command_t *command);

struct _server {
    dispatch_table_t dispatch;

    /* This is an optimization to avoid a giant switch statement. */
    command_handler_t handler_table[COMMAND_MAX_COMMAND];

    mutex_t thread_started_mutex;
    buffer_t *buffer;
    thread_t thread;
    bool threaded;
    HashTable *names_cache;
    void (*command_post_hook)(server_t *server, command_t *command);

    mutex_t* signal_mutex;
    signal_t* signal;
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

#endif /* GPUPROCESS_SERVER_H */

