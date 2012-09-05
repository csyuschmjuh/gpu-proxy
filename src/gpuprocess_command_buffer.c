
#include <stdlib.h>

#include "config.h"

#include "gpuprocess_command_buffer.h"

// The size of the buffer (in bytes). Note that for some buffers
// such as the memory-mirrored ring buffer the actual buffer size
// may be larger.
#define BUFFER_SIZE 10

command_buffer_t *
command_buffer_create()
{
    command_buffer_t *command_buffer;
    command_buffer = (command_buffer_t *)malloc(sizeof(command_buffer_t));

    buffer_create(command_buffer->buffer, BUFFER_SIZE);

    command_buffer->service = command_buffer_service_initialize(command_buffer->buffer);
    command_buffer->token = 0;

    return command_buffer;
}

bool
command_buffer_destroy(command_buffer_t *command_buffer)
{
    buffer_free(command_buffer->buffer);
    command_buffer->buffer = NULL;
    command_buffer_service_destroy(command_buffer->service);
    free(command_buffer);
}

static command_t *
command_buffer_get_space_for_size(command_buffer_t *command_buffer, size_t size)
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
command_buffer_get_space_for_command(command_buffer_t *command_buffer, command_id_t command_id)
{
    switch (command_id) {
        case COMMAND_NO_OP: break;
        case COMMAND_SET_TOKEN:
            return command_buffer_get_space_for_size(command_buffer, sizeof(command_set_token_t));
            break;
        }

    return NULL;
}

unsigned int
command_buffer_insert_token(command_buffer_t *command_buffer)
{
    command_t *set_token_command =
        command_buffer_get_space_for_command(command_buffer, COMMAND_SET_TOKEN);
    /* FIXME: Check max size and wrap. */
    unsigned int token = command_buffer->token++;

    command_set_token_init((command_set_token_t *)set_token_command, token);

    buffer_write_advance(command_buffer->buffer, set_token_command->size);

    return token;
}

bool
command_buffer_wait_for_token(command_buffer_t *command_buffer, unsigned int token)
{
    while (command_buffer->buffer->last_token < token) {
        /* FIXME: Do not wait forever. */
    }

    return true;
}

bool
command_buffer_write_command(command_buffer_t *command_buffer, command_t *command)
{
    buffer_write_advance (command_buffer->buffer, command->size);

    return true;
}

bool
command_buffer_flush(command_buffer_t *command_buffer)
{
    return true;
}

void
command_buffer_set_active_state (command_buffer_t *command_buffer,
                                 v_link_list_t    *active_state)
{
    command_buffer_service_set_active_state (command_buffer->service,
                                             active_state);
}
