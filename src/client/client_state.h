#ifndef CLIENT_STATE_H
#define CLIENT_STATE_H

#include "egl_states.h"
#include "command_buffer.h"
#include "name_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct client_state {
    command_buffer_t *command_buffer;
    name_handler_t *name_handler;
    link_list_t *active_egl_state;
} client_state_t;

private client_state_t *
client_state_get_thread_local ();

private void
client_state_destroy_thread_local ();

private bool
client_state_buffer_available ();

private bool
client_state_active_egl_state_available ();

private bool
client_state_buffer_and_state_available ();

private command_buffer_t *
client_state_get_command_buffer ();

private egl_state_t *
client_state_get_active_egl_state ();

private void
client_state_set_active_egl_state (link_list_t* active_egl_state);

private int
client_state_get_unpack_alignment ();

private name_handler_t *
client_state_get_name_handler ();

#ifdef __cplusplus
}
#endif

#endif /* CLIENT_STATE_H */
