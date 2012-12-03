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

    /* pre-allocated vertex and indices memory */
    char pre_1k_mem [MEM_1K_SIZE][1024];
    char pre_2k_mem [MEM_2K_SIZE][2048];
    char pre_4k_mem [MEM_4K_SIZE][4096];
    char pre_8k_mem [MEM_8K_SIZE][8192];
    char pre_16k_mem [MEM_16K_SIZE][16384];
    char pre_32k_mem [MEM_32K_SIZE][32768];
    int  last_1k_index;
    int  last_2k_index;
    int  last_4k_index;
    int  last_8k_index;
    int  last_16k_index;
    int  last_32k_index;

    sem_t server_signal;
    sem_t client_signal;
    GLuint generated_result_id;
};

private client_t *
client_get_thread_local ();

private void
client_destroy_thread_local ();

private int
client_get_unpack_alignment ();

private command_t *
client_get_space_for_command (command_type_t command_type);

private command_t *
client_try_get_space_for_command_with_extra_space (command_type_t command_type,
                                                   size_t extra);

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

private bool
client_destroy (client_t *client);

private void
client_init (client_t *);

private bool
client_has_valid_state (client_t *client);

private egl_state_t *
client_get_current_state (client_t *client);

#endif /* CLIENT_H */
