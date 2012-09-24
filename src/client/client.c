#include "config.h"
#include "client.h"

__thread client_t* thread_local_client
    __attribute__(( tls_model ("initial-exec"))) = NULL;

__thread bool on_client_thread
    __attribute__(( tls_model ("initial-exec"))) = false;

static client_t *
client_new ()
{
    client_t *client = (client_t *)malloc (sizeof (client_t));

    client->name_handler = name_handler_create ();
    client->active_egl_state = NULL;
    client->token = 0;

    on_client_thread = true;

    buffer_create (&client->buffer);
    client->server = server_new (&client->buffer);

    return client;
}

static bool
client_destroy (client_t *client)
{
    server_destroy (client->server);
    client->server = NULL;

    buffer_free (&client->buffer);

    name_handler_destroy (client->name_handler);
    free (client);
}

client_t *
client_get_thread_local ()
{
    if (unlikely (! thread_local_client))
        thread_local_client = client_new ();
    return thread_local_client;
}

void
client_destroy_thread_local ()
{
    if (! thread_local_client)
        return;

     client_destroy (thread_local_client);
     thread_local_client = NULL;
}

bool
client_active_egl_state_available ()
{
    return client_get_thread_local ()->active_egl_state;
}

egl_state_t *
client_get_active_egl_state ()
{
    return (egl_state_t *) (client_get_thread_local ())->active_egl_state->data;
}

void
client_set_active_egl_state (link_list_t* active_egl_state)
{
    client_get_thread_local ()->active_egl_state = active_egl_state;
}

name_handler_t *
client_get_name_handler ()
{
    return client_get_thread_local ()->name_handler;
}

static inline void
sleep_nanoseconds (int num_nanoseconds)
{
    struct timespec spec;
    spec.tv_sec = 0;
    spec.tv_nsec = num_nanoseconds;
    nanosleep (&spec, NULL);
}

static command_t *
client_get_space_for_size (client_t *client,
                           size_t size)
{
    size_t available_space;
    command_t *write_location = (command_t *) buffer_write_address (&client->buffer,
                                                                    &available_space);
    while (! write_location || available_space < size) {
        /* FIXME: Should we avoid sleeping and ask for space. */
        sleep_nanoseconds (100);
        write_location = (command_t *) buffer_write_address (&client->buffer,
                                                             &available_space);
    }

    return write_location;
}
command_t *
client_get_space_for_command (command_id_t command_id)
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
    command = client_get_space_for_size (client_get_thread_local (),
                                         command_sizes[command_id]);
    command->size = command_sizes[command_id];
    return command;
}

unsigned int
client_insert_token (client_t *client)
{
    command_t *set_token_command = client_get_space_for_command (COMMAND_SET_TOKEN);
    /* FIXME: Check max size and wrap. */
    unsigned int token = client->token++;

    assert (set_token_command != NULL);

    command_set_token_init ((command_set_token_t *)set_token_command, token);

    client_write_command (set_token_command);

    return token;
}

bool
client_wait_for_token (client_t *client,
                       unsigned int token)
{
    while (client->buffer.last_token < token) {
        /* FIXME: Do not wait forever and force flush. */
    }

    return true;
}

bool
client_write_command (command_t *command)
{
    client_t *client = client_get_thread_local ();
    buffer_write_advance (&client->buffer, command->size);

    return true;
}

bool
client_flush (client_t *client)
{
    return true;
}
