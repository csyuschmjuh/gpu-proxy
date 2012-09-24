#include "config.h"
#include "server.h"

#include "caching_server_private.h"
#include "ring_buffer.h"
#include "server_dispatch_table_helpers.h"
#include "thread_private.h"
#include <time.h>

extern __thread bool on_client_thread;

static void
server_fill_dispatch_table (server_t *server);

static void
server_handle_set_token (server_t *server,
                         command_set_token_t *command)
{
    server->buffer->last_token = command->token;
}

static void
server_handle_command (server_t *server,
                       command_t *command)
{
    switch (command->id) {
        case COMMAND_NO_OP: break;
        case COMMAND_SET_TOKEN:
            server_handle_set_token (server,
                                                    (command_set_token_t *)command);
            break;
        }
}

static inline void
sleep_nanoseconds (int num_nanoseconds)
{
    struct timespec spec;
    spec.tv_sec = 0;
    spec.tv_nsec = num_nanoseconds;
    nanosleep (&spec, NULL);
}

static void *
server_thread_func (void *ptr)
{
    server_t *server = (server_t *)ptr;

    server_fill_dispatch_table (server);

    /* populate dispatch table, create global egl_states structures */
    _server_init (server);

    /* This signals the producer thread to start producing. */
    mutex_unlock (server->thread_started_mutex);

    /* FIXME: initialize GL state and start consuming commands in the loop. */
    /* FIXME: add exit condition of the loop. */
    while (true) {
        size_t data_left_to_read;
        command_t *read_command = (command_t *) buffer_read_address (server->buffer,
                                                                     &data_left_to_read);

        /* The buffer is empty, so wait until there's something to read. */
        while (! read_command) {
            sleep_nanoseconds (100);
            read_command = (command_t *) buffer_read_address (server->buffer,
                                                              &data_left_to_read);
        }

        server_handle_command (server, read_command);

        buffer_read_advance (server->buffer, read_command->size);
    }
}

server_t *
server_new (buffer_t *buffer)
{
    server_t *server = malloc (sizeof (server_t));
    server_init (server, buffer, true);
    on_client_thread = false;
    return server;
}

void
server_init (server_t *server,
             buffer_t *buffer,
             bool threaded)
{
    server->threaded = threaded;
    server->buffer = buffer;

    if (threaded) {
        /* We use a mutex here to wait until the thread has started. */
        mutex_init (server->thread_started_mutex);
        mutex_lock (server->thread_started_mutex);

        pthread_create (&server->thread, NULL, server_thread_func, server);

        mutex_lock (server->thread_started_mutex);
        mutex_destroy (server->thread_started_mutex);
    } else {
         server_fill_dispatch_table (server);
        _server_init (server);
    }
}

bool
server_destroy (server_t *server)
{
    server->buffer = NULL;

    if (server->threaded)
        pthread_join (server->thread, NULL);

    free (server);
    return true;
}

#include "server_dispatch_table_autogen.c"
