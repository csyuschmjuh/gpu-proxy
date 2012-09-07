
#ifndef GPUPROCESS_COMMAND_BUFFER_H
#define GPUPROCESS_COMMAND_BUFFER_H

#include "compiler_private.h"
#include "command.h"
#include "command_buffer_server.h"
#include "ring_buffer.h"
#include "types_private.h"
#include "egl_states.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the commands in the buffer using the following sequence
 * of calls:
 *   COMMAND_NAME_t *command =
 *       COMMAND_NAME_T(command_buffer_get_space_for_command (command_buffer, command_id));
 *   COMMAND_NAME_initialize (command, parameter1, parameter2, ...);
 *   command_buffer_write_command (command_buffer, command);
 */

/* Each command should have this header struct as the first element in
 * their structs.
 */
typedef struct command_buffer {
    buffer_t *buffer;
    command_buffer_server_t *server;
    unsigned int token;
} command_buffer_t;

private command_buffer_t *
command_buffer_create ();

private bool
command_buffer_destroy (command_buffer_t *command_buffer);

private command_t *
command_buffer_get_space_for_command (command_buffer_t *command_buffer,
                                      command_id_t command_id);

private bool
command_buffer_write_command (command_buffer_t *command_buffer,
                              command_t *command);

private bool
command_buffer_flush (command_buffer_t *command_buffer);

private unsigned int
command_buffer_insert_token (command_buffer_t *command_buffer);

private bool
command_buffer_wait_for_token (command_buffer_t *command_buffer,
                               unsigned int token);

private void
command_buffer_set_active_state (command_buffer_t *command_buffer,
                                 v_link_list_t     *active_state);

#ifdef __cplusplus
}
#endif

#endif /* GPUPROCESS_COMMAND_BUFFER_H */
