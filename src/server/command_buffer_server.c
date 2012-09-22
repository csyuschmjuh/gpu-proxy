
#include "ring_buffer.h"
#include "command_buffer_server.h"
#include "egl_server_private.h"
#include "thread_private.h"
#include <time.h>

mutex_static_init (server_thread_started_mutex);

static void
command_buffer_server_handle_set_token (command_buffer_server_t *server,
                                        command_set_token_t *command)
{
    server->buffer->last_token = command->token;
}

static void
command_buffer_server_handle_command (command_buffer_server_t *server,
                                      command_t *command)
{
    switch (command->id) {
        case COMMAND_NO_OP: break;
        case COMMAND_SET_TOKEN:
            command_buffer_server_handle_set_token (server,
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
    command_buffer_server_t *command_buffer_server = (command_buffer_server_t *)ptr;

    /* populate dispatch table, create global egl_states structures */
    _server_init (command_buffer_server);
    /* This signals the producer thread to start producing. */
    mutex_unlock (server_thread_started_mutex);

    /* FIXME: initialize GL state and start consuming commands in the loop. */
    /* FIXME: add exit condition of the loop. */
    while (true) {
        size_t data_left_to_read;
        command_t *read_command = (command_t *) buffer_read_address (command_buffer_server->buffer,
                                                                     &data_left_to_read);

        /* The buffer is empty, so wait until there's something to read. */
        while (! read_command) {
            sleep_nanoseconds (100);
            read_command = (command_t *) buffer_read_address (command_buffer_server->buffer,
                                                              &data_left_to_read);
        }

        command_buffer_server_handle_command (command_buffer_server, read_command);

        buffer_read_advance (command_buffer_server->buffer, read_command->size);
    }
}

command_buffer_server_t *
command_buffer_server_new (buffer_t *buffer)
{
    command_buffer_server_t *command_buffer_server = malloc (sizeof (command_buffer_server_t));
    command_buffer_server_init (command_buffer_server, buffer, true);
    return command_buffer_server;
}

void
command_buffer_server_init (command_buffer_server_t *server,
                            buffer_t *buffer,
                            bool threaded)
{
    server->threaded = threaded;
    server->buffer = buffer;

    if (threaded) {
        /* We use a mutex here to wait until the thread has started. */
        mutex_lock (server_thread_started_mutex);
        pthread_create (&server->thread, NULL, server_thread_func, server);
        mutex_lock (server_thread_started_mutex);
    } else {
        _server_init (server);
    }
}

bool
command_buffer_server_destroy (command_buffer_server_t *server)
{
    server->buffer = NULL;

    if (server->threaded) {
        pthread_join (server->thread, NULL);
        /* FIXME: GL termination. */
        if (server->active_state) {
            _server_remove_state (&server->active_state);
            server->active_state = NULL;
        }
    }

    free (server);
    return true;
}

void
command_buffer_server_set_active_state (command_buffer_server_t *server,
                                        link_list_t *active_state)
{
    server->active_state = active_state;
}
