#include "config.h"
#include "client.h"

#include "caching_client.h"
#include "caching_client_private.h"
#include "command.h"

#include <sys/prctl.h>

__thread client_t* thread_local_client
    __attribute__(( tls_model ("initial-exec"))) = NULL;

__thread bool client_thread
    __attribute__(( tls_model ("initial-exec"))) = false;

__thread bool initializing_client
    __attribute__(( tls_model ("initial-exec"))) = false;

mutex_static_init (client_thread_mutex);

static void
client_fill_dispatch_table (dispatch_table_t *client);

static inline void
sleep_nanoseconds (int num_nanoseconds)
{
    struct timespec spec;
    spec.tv_sec = 0;
    spec.tv_nsec = num_nanoseconds;
    nanosleep (&spec, NULL);
}

static bool
on_client_thread ()
{
    static bool initialized = false;
    if (initialized)
        return client_thread;

    mutex_lock (client_thread_mutex);
    if (initialized)
        return client_thread;
    client_thread = true;
    initialized = true;
    mutex_unlock (client_thread_mutex);

    return client_thread;
}

bool
should_use_base_dispatch ()
{
    if (! on_client_thread ())
        return true;
    if (initializing_client)
        return true;
    return false;
}

void *
start_server_thread_func (void *ptr)
{

    client_t *client = (client_t *)ptr;
    server_t *server = server_new (&client->buffer);

    mutex_unlock (client->server_started_mutex);
    prctl (PR_SET_TIMERSLACK, 1);
    server_start_work_loop (server);

    server_destroy(server);
    return NULL;
}

void
client_start_server (client_t *client)
{
    mutex_init (client->server_started_mutex);
    mutex_lock (client->server_started_mutex);
    pthread_create (&client->server_thread, NULL, start_server_thread_func, client);
    mutex_lock (client->server_started_mutex);
    mutex_destroy (client->server_started_mutex);
}

client_t *
client_new ()
{
    client_t *client = (client_t *)malloc (sizeof (client_t));
    client_init (client);

    return client;
}

void
client_init (client_t *client)
{
    prctl (PR_SET_TIMERSLACK, 1);
    initializing_client = true;

    client->post_hook = NULL;

    buffer_create (&client->buffer);

    // We initialize the base dispatch table synchronously here, so that we
    // don't have to worry about the server thread trying to initialize it
    // at the same time.
    dispatch_table_get_base ();

    client_fill_dispatch_table (&client->dispatch);

    client->name_handler = name_handler_create ();
    client->token = 0;

    client_start_server (client);
    initializing_client = false;
}

static void
client_shutdown_server (client_t *client)
{
    command_t *command = client_get_space_for_command (COMMAND_SHUTDOWN);
    command->type = COMMAND_SHUTDOWN;
    client_run_command (command);
}


bool
client_destroy (client_t *client)
{
    client_shutdown_server (client);

    buffer_free (&client->buffer);

    name_handler_destroy (client->name_handler);
    free (client);

    return true;
}

client_t *
client_get_thread_local ()
{
    if (unlikely (! thread_local_client)) {
        thread_local_client = CLIENT (caching_client_new ());
    }
    return thread_local_client;
}

void
client_destroy_thread_local ()
{
    if (! thread_local_client)
        return;

    caching_client_destroy (CACHING_CLIENT (thread_local_client));
    thread_local_client = NULL;
}

name_handler_t *
client_get_name_handler ()
{
    return client_get_thread_local ()->name_handler;
}

static command_t *
client_get_space_for_size (client_t *client,
                           size_t size)
{
    size_t available_space;
    command_t *write_location = (command_t *) buffer_write_address (&client->buffer,
                                                                    &available_space);
    while (! write_location || available_space < size) {
        sched_yield ();
        write_location = (command_t *) buffer_write_address (&client->buffer,
                                                             &available_space);
    }

    return write_location;
}

static bool
client_is_space_full (client_t *client, size_t size)
{
    size_t available_size;
    command_t *write_location = (command_t *) buffer_write_address (&client->buffer, &available_size);
    if (! write_location || available_size < size)
        return true;
    return false;
}

bool
client_is_space_full_for_command (command_type_t command_type)
{
    static bool initialized = false;
    static size_t command_sizes[COMMAND_MAX_COMMAND];

    if (!initialized) {
        int i;
        for (i = 0; i < COMMAND_MAX_COMMAND; i++)
            command_sizes[i] = command_get_size (i);
        initialized = true;
    }

    assert (command_type >= 0 && command_type < COMMAND_MAX_COMMAND);
    return client_is_space_full (client_get_thread_local(), 
                                 command_sizes[command_type]);
}

command_t *
client_get_space_for_command (command_type_t command_type)
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

    assert (command_type >= 0 && command_type < COMMAND_MAX_COMMAND);
    command = client_get_space_for_size (client_get_thread_local (),
                                         command_sizes[command_type]);
    command->type = command_type;
    command->size = command_sizes[command_type];
    command->token = 0;
    return command;
}

void
client_run_command (command_t *command)
{
    client_t *client = client_get_thread_local ();
    unsigned int token = ++client->token;

    /* Overflow case */
    if (token == 0)
        token = 1;

    command->token = token;
    client_run_command_async (command);

    while (client->buffer.last_token < token)
        sleep_nanoseconds (500);
}

void
client_run_command_async (command_t *command)
{
    client_t *client = client_get_thread_local ();

    if (client->post_hook)
        client->post_hook (client, command);

    buffer_write_advance (&client->buffer, command->size);
}

bool
client_flush (client_t *client)
{
    return true;
}

int
client_get_unpack_alignment ()
{
    client_t *client = client_get_thread_local ();
    if (!client->active_state)
        return 4;

    egl_state_t *egl_state = (egl_state_t *)client->active_state->data;
    return egl_state->state.unpack_alignment;
}

bool
client_has_valid_state (client_t *client)
{
    return client->active_state &&
           ((egl_state_t *) client->active_state->data)->active;
}

egl_state_t *
client_get_current_state (client_t *client)
{
    return (egl_state_t *) client->active_state->data;
}

gles2_state_t *
client_get_current_gl_state (client_t *client)
{
    return &((egl_state_t *) client->active_state->data)->state;
}

#include "client_autogen.c"
