#ifndef CLIENT_H
#define CLIENT_H

typedef struct _client client_t;

#include "dispatch_table.h"
#include "egl_state.h"
#include "ring_buffer.h"
#include "server.h"
#include "types_private.h"
#include <semaphore.h>

#define CLIENT(object) ((client_t *) (object))

/* Initialize the commands in the buffer using the following sequence
 * of calls:
 *   COMMAND_NAME_t *command =
 *       COMMAND_NAME_T(client_get_space_for_command (command_type));
 *   COMMAND_NAME_initialize (command, parameter1, parameter2, ...);
 *   client_write_command (command);
 */
#define MEM_1K_SIZE  64
#define MEM_2K_SIZE  64
#define MEM_4K_SIZE  64
#define MEM_8K_SIZE  64
#define MEM_16K_SIZE 32
#define MEM_32K_SIZE 32

struct _client {
    dispatch_table_t dispatch;

    buffer_t buffer;
    unsigned int token;

    egl_state_t *active_state;

    mutex_t server_started_mutex;
    thread_t server_thread;
    bool initializing;
    bool needs_timestamp;
    double timestamp;

    sem_t server_signal;
    sem_t client_signal;
};

private client_t *
client_get_thread_local ();

private void
client_destroy_thread_local ();

private int
client_get_unpack_alignment ();

private int
client_get_unpack_row_length ();

private int
client_get_unpack_skip_pixels ();

private int
client_get_unpack_skip_rows ();

private command_t *
client_get_space_for_size (client_t *client,
                           size_t size);

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

private client_t *
client_new ();

private bool
client_destroy (client_t *client);

private void
client_init (client_t *);

private bool
client_has_valid_state (client_t *client);

private egl_state_t *
client_get_current_state (client_t *client);

private command_t *
client_get_space_for_log_size (client_t *client,
                               size_t size);

private command_t *
client_get_space_for_log_command (command_type_t command_type);

private void
client_run_log_command_async (command_t *command);

private void
client_run_log_command (command_t *command);

private void
client_send_log (double timestamp);

private double
client_get_timestamp ();

private void
clients_list_add_client (client_t *client);

private void
clients_list_remove_client (client_t *client);

private void
clients_list_set_needs_timestamp ();

#endif /* CLIENT_H */
