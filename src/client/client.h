#ifndef CLIENT_H
#define CLIENT_H

#include "egl_states.h"
#include "name_handler.h"
#include "ring_buffer.h"
#include "server.h"
#include "types_private.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the commands in the buffer using the following sequence
 * of calls:
 *   COMMAND_NAME_t *command =
 *       COMMAND_NAME_T(client_get_space_for_command (command_type));
 *   COMMAND_NAME_initialize (command, parameter1, parameter2, ...);
 *   client_write_command (command);
 */
typedef struct client {
    name_handler_t *name_handler;

    buffer_t buffer;
    unsigned int token;

    mutex_t server_started_mutex;
    thread_t server_thread;
} client_t;

private client_t *
client_get_thread_local ();

private void
client_destroy_thread_local ();

private int
client_get_unpack_alignment ();

private name_handler_t *
client_get_name_handler ();

private command_t *
client_get_space_for_command (command_type_t command_type);

private void
client_run_command_async (command_t *command);

private void
client_run_command (command_t *command);

private bool
client_flush (client_t *client);

private bool
on_client_thread ();

private void
client_start_server (client_t *client);

private void
client_start_server (client_t *client);

private client_t *
client_new ();

#ifdef __cplusplus
}
#endif

#endif /* CLIENT_H */
