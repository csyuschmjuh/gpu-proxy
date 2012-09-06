
#include <stdlib.h>

#include "config.h"

#include "command_buffer.h"

// The size of the buffer (in bytes). Note that for some buffers
// such as the memory-mirrored ring buffer the actual buffer size
// may be larger.
#define BUFFER_SIZE 10

__thread command_buffer_t* thread_local_command_buffer
    __attribute__(( tls_model ("initial-exec"))) = NULL;

static command_buffer_t *
command_buffer_create ()
{
    command_buffer_t *command_buffer;
    command_buffer = (command_buffer_t *)malloc (sizeof (command_buffer_t));

    buffer_create (command_buffer->buffer, BUFFER_SIZE);

    command_buffer->server = command_buffer_server_initialize (command_buffer->buffer);
    command_buffer->token = 0;

    return command_buffer;
}

static bool
command_buffer_destroy (command_buffer_t *command_buffer)
{
    buffer_free (command_buffer->buffer);
    command_buffer->buffer = NULL;
    command_buffer_server_destroy (command_buffer->server);
    free (command_buffer);
}

command_buffer_t *
command_buffer_get_thread_local ()
{
    if (unlikely (! thread_local_command_buffer))
        thread_local_command_buffer = command_buffer_create();
    return thread_local_command_buffer;
}

private void
command_buffer_destroy_thread_local ()
{
    if (! thread_local_command_buffer)
        return;

     command_buffer_destroy (thread_local_command_buffer);
     thread_local_command_buffer = NULL;
}

static command_t *
command_buffer_get_space_for_size (command_buffer_t *command_buffer,
                                   size_t size)
{
    size_t available_space;
    command_t *write_location = (command_t *) buffer_write_address (command_buffer->buffer,
                                                                    &available_space);
    while (! write_location || available_space < size) {
        /* FIXME: Should we avoid sleeping and ask for space. */
        sleep_nanoseconds (100);
        write_location = (command_t *) buffer_write_address (command_buffer->buffer,
                                                             &available_space);
    }

    return write_location;
}
command_t *
command_buffer_get_space_for_command (command_buffer_t *command_buffer,
                                      command_id_t command_id)
{
    command_t *command = NULL;
    static bool initialized = false;
    static size_t command_sizes[COMMAND_MAX_COMMAND];

    if (!initialized) {
        int i;
        for (i = 0; i < COMMAND_MAX_COMMAND; i++)
            command_sizes[i] = command_get_size (i);
        initialized = true;
    }

    assert (command_id >= 0 && command_id < COMMAND_MAX_COMMAND);
    command = command_buffer_get_space_for_size (command_buffer, command_sizes[command_id]);
    command->size = command_sizes[command_id];
    return command;
}

unsigned int
command_buffer_insert_token (command_buffer_t *command_buffer)
{
    command_t *set_token_command =
        command_buffer_get_space_for_command (command_buffer, COMMAND_SET_TOKEN);
    /* FIXME: Check max size and wrap. */
    unsigned int token = command_buffer->token++;

    assert (set_token_command != NULL);

    command_set_token_init ((command_set_token_t *)set_token_command, token);

    command_buffer_write_command (command_buffer, set_token_command);

    return token;
}

bool
command_buffer_wait_for_token (command_buffer_t *command_buffer,
                              unsigned int token)
{
    while (command_buffer->buffer->last_token < token) {
        /* FIXME: Do not wait forever and force flush. */
    }

    return true;
}

bool
command_buffer_write_command (command_buffer_t *command_buffer,
                             command_t *command)
{
    buffer_write_advance (command_buffer->buffer, command->size);

    return true;
}

bool
command_buffer_flush (command_buffer_t *command_buffer)
{
    return true;
}

void
command_buffer_set_active_state (command_buffer_t *command_buffer,
                                 v_link_list_t *active_state)
{
    command_buffer_server_set_active_state (command_buffer->server,
                                            active_state);
}
