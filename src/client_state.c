#include "config.h"
#include "client_state.h"

__thread client_state_t* thread_local_client_state
    __attribute__(( tls_model ("initial-exec"))) = NULL;

static client_state_t *
client_state_create ()
{
    client_state_t *client_state;
    client_state = (client_state_t *)malloc (sizeof (client_state_t));

    client_state->command_buffer = command_buffer_create ();
    client_state->active_egl_state = NULL;
    client_state->unpack_alignment = 0;

    return client_state;
}

static bool
client_state_destroy ()
{
    client_state_t *client_state = client_state_get_thread_local ();

    command_buffer_destroy (client_state->command_buffer);
}

client_state_t *
client_state_get_thread_local ()
{
    if (unlikely (! thread_local_client_state))
        thread_local_client_state = client_state_create();
    return thread_local_client_state;
}

void
client_state_destroy_thread_local ()
{
    if (! thread_local_client_state)
        return;

     client_state_destroy (thread_local_client_state);
     thread_local_client_state = NULL;
}

bool
client_state_buffer_available ()
{
    client_state_t *client_state = client_state_get_thread_local ();

    return client_state->command_buffer;
}

bool
client_state_active_egl_state_available ()
{
    client_state_t *client_state = client_state_get_thread_local ();

    return client_state->active_egl_state;
}

bool
client_state_buffer_and_state_available ()
{
    client_state_t *client_state = client_state_get_thread_local ();

    return client_state->active_egl_state &&
        client_state->command_buffer;
}

command_buffer_t *
client_state_get_command_buffer ()
{
    client_state_t *client_state = client_state_get_thread_local ();

    return client_state->command_buffer;
}

int
client_state_get_unpack_alignment ()
{
    client_state_t *client_state = client_state_get_thread_local ();

    return client_state->unpack_alignment;
}

void
client_state_set_unpack_alignment (int unpack_alignment)
{
    client_state_t *client_state = client_state_get_thread_local ();

    client_state->unpack_alignment = unpack_alignment;
}

egl_state_t *
client_state_get_active_egl_state ()
{
    client_state_t *client_state = client_state_get_thread_local ();

    return (egl_state_t *)client_state->active_egl_state->data;
}

void
client_state_set_active_egl_state (v_link_list_t* active_egl_state)
{
    client_state_t *client_state = client_state_get_thread_local ();

    client_state->active_egl_state = active_egl_state;
}
