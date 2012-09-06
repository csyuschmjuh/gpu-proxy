
#include "command_buffer_server.h"
#include "egl_server_private.h"
#include "thread_private.h"

mutex_static_init (server_thread_started_mutex);

static void
command_buffer_server_handle_set_token (command_buffer_server_t *command_buffer_server,
                                        command_set_token_t *command)
{
    command_buffer_server->buffer->last_token = command->token;
}

static void
command_buffer_server_handle_command (command_buffer_server_t *command_buffer_server,
                                      command_t *command)
{
    switch (command->id) {
        case COMMAND_NO_OP: break;
        case COMMAND_SET_TOKEN:
            command_buffer_server_handle_set_token (command_buffer_server,
                                                    (command_set_token_t *)command);
            break;
        }
}

static void *
server_thread_func (void *ptr)
{
    command_buffer_server_t *command_buffer_server = (command_buffer_server_t *)ptr;

    /* populate dispatch table, create global egl_states structures */
    _server_init ();
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
command_buffer_server_initialize (buffer_t *buffer)
{
    command_buffer_server_t *command_buffer_server;

    command_buffer_server = (command_buffer_server_t *) malloc (sizeof (command_buffer_server_t));
    command_buffer_server->buffer = buffer;

    /* We use a mutex here to wait until the thread has started. */
    mutex_lock (server_thread_started_mutex);
    pthread_create (command_buffer_server->thread, NULL, server_thread_func, command_buffer_server);
    mutex_lock (server_thread_started_mutex);

    return command_buffer_server;
}

bool
command_buffer_server_destroy (command_buffer_server_t *command_buffer_server)
{
    command_buffer_server->buffer = NULL;
    free (command_buffer_server);

    pthread_join (*command_buffer_server->thread, NULL);
    /* FIXME: GL termination. */
    if (command_buffer_server->active_state) {
        _server_remove_state (command_buffer_server->active_state);
        command_buffer_server->active_state = NULL;
    }

    return true;
}

void
command_buffer_server_set_active_state (command_buffer_server_t *command_buffer_server,
                                        v_link_list_t *active_state)
{
    command_buffer_server->active_state = active_state;
}
