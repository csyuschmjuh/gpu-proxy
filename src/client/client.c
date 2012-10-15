#include "config.h"
#include "client.h"
#include <sys/prctl.h>

__thread client_t* thread_local_client
    __attribute__(( tls_model ("initial-exec"))) = NULL;

__thread bool client_thread
    __attribute__(( tls_model ("initial-exec"))) = false;

mutex_static_init (client_thread_mutex);

bool
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

static void *
start_server_thread_func (void *ptr)
{
    client_t *client = (client_t *)ptr;
    server_t *server = server_new (&client->buffer);

    mutex_unlock (client->server_started_mutex);
    prctl (PR_SET_TIMERSLACK, 1);
    server_start_work_loop (server);

    /* TODO: Clean up the server here. */
    return NULL;
}

static void
client_start_server (client_t *client)
{
    mutex_init (client->server_started_mutex);
    mutex_lock (client->server_started_mutex);
    pthread_create (&client->server_thread, NULL, start_server_thread_func, client);
    mutex_lock (client->server_started_mutex);
    mutex_destroy (client->server_started_mutex);
}

static client_t *
client_new ()
{
    prctl (PR_SET_TIMERSLACK, 1);
    client_t *client = (client_t *)malloc (sizeof (client_t));

    client->name_handler = name_handler_create ();
    client->token = 0;

    buffer_create (&client->buffer);
    client_start_server (client);

    return client;
}

static bool
client_destroy (client_t *client)
{
    /* TODO: Send a message to the server to signal a stop. */
    pthread_join (client->server_thread, NULL);

    buffer_free (&client->buffer);

    name_handler_destroy (client->name_handler);
    free (client);

    return true;
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
        sleep_nanoseconds (500);
        write_location = (command_t *) buffer_write_address (&client->buffer,
                                                             &available_space);
    }

    return write_location;
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

    command->token = token;
    client_run_command_async (command);

    while (client->buffer.last_token < token)
        sleep_nanoseconds (500);
}

void
client_run_command_async (command_t *command)
{
    client_t *client = client_get_thread_local ();
    buffer_write_advance (&client->buffer, command->size);
}

bool
client_flush (client_t *client)
{
    return true;
}
