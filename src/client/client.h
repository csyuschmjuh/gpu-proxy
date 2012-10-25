#ifndef CLIENT_H
#define CLIENT_H

typedef struct _client client_t;

#include "dispatch_table.h"
#include "egl_states.h"
#include "name_handler.h"
#include "ring_buffer.h"
#include "server.h"
#include "types_private.h"

#define CLIENT(object) ((client_t *) (object))

/* Initialize the commands in the buffer using the following sequence
 * of calls:
 *   COMMAND_NAME_t *command =
 *       COMMAND_NAME_T(client_get_space_for_command (command_type));
 *   COMMAND_NAME_initialize (command, parameter1, parameter2, ...);
 *   client_write_command (command);
 */
struct _client {
    dispatch_table_t dispatch;

    name_handler_t *name_handler;

    void (*post_hook)(client_t *client, command_t *command);

    buffer_t buffer;
    unsigned int token;

    link_list_t *active_state;

    mutex_t server_started_mutex;
    thread_t server_thread;
    bool initializing;
};

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
should_use_base_dispatch ();

private void
client_start_server (client_t *client);

private void
client_start_server (client_t *client);

private client_t *
client_new ();

private void
client_init (client_t *);

private bool
client_has_valid_state (client_t *client);

#endif /* CLIENT_H */
