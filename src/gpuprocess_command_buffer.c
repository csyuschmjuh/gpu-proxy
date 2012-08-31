
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

command_t *
command_buffer_get_space_for_command(command_buffer_t *command_buffer, command_id_t command_id)
{
    return NULL;
}

bool
command_buffer_write_command(command_buffer_t *command_buffer, command_t *command)
{
    return true;
}

bool
command_buffer_flush(command_buffer_t *command_buffer)
{
    return true;
}

bool
command_buffer_wait(command_buffer_t *command_buffer)
{
    return true;
}

void
command_buffer_set_active_state (command_buffer_t *command_buffer, 
                                 egl_state_t      *active_state)
{
    command_buffer_service_set_active_state (command_buffer->service,
                                             active_state);
}
