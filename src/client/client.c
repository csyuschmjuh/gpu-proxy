#include "config.h"
#include "client.h"

#include "caching_client.h"
#include "caching_client_private.h"
#include "command.h"
#include "name_handler.h"

#include <sys/prctl.h>
#include <sched.h>
#include <sys/types.h>

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
    name_handler_init ();
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

    server->server_signal_mutex = &client->server_signal_mutex;
    server->server_signal = &client->server_signal;
    server->client_signal_mutex = &client->client_signal_mutex;
    server->client_signal = &client->client_signal;

    mutex_unlock (client->server_started_mutex);
    prctl (PR_SET_TIMERSLACK, 1);

    pthread_t id = pthread_self ();
    //int scheduler = sched_getscheduler (pid);
    struct sched_param param;
    param.sched_priority = 99;
    pthread_setschedparam (id, SCHED_FIFO, (const struct sched_param *)&param);
    int priority = sched_get_priority_max (SCHED_FIFO);
    pthread_setschedprio (pthread_self(), priority);
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

    buffer_create (&client->buffer, 512, "command");

    // We initialize the base dispatch table synchronously here, so that we
    // don't have to worry about the server thread trying to initialize it
    // at the same time.
    dispatch_table_get_base ();

    client_fill_dispatch_table (&client->dispatch);

    client->token = 0;

    pthread_mutex_init (&client->server_signal_mutex, NULL);
    pthread_cond_init (&client->server_signal, NULL);
    pthread_mutex_init (&client->client_signal_mutex, NULL);
    pthread_cond_init (&client->client_signal, NULL);

    client->last_1k_index = 0;
    client->last_2k_index = 0;
    client->last_4k_index = 0;
    client->last_8k_index = 0;
    client->last_16k_index = 0;
    client->last_32k_index = 0;

    client->active_state = NULL;

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

    pthread_mutex_destroy (&client->server_signal_mutex);
    pthread_cond_destroy (&client->server_signal);
    pthread_mutex_destroy (&client->client_signal_mutex);
    pthread_cond_destroy (&client->client_signal);

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

command_t *
client_try_get_space_for_command_with_extra_space (command_type_t command_type,
                                                   size_t extra)
{
    client_t *client = client_get_thread_local ();

    size_t available_space;
    command_t *command = (command_t *) buffer_write_address (&client->buffer,
                                                             &available_space);
    size_t command_size = command_get_size (command_type) + extra;
    if (! command || available_space < command_size);
        return NULL;

    command->type = command_type;
    command->size = command_size;
    command->token = 0;
    return command;
}

static command_t *
client_get_space_for_size (client_t *client,
                           size_t size)
{
    size_t available_space;
    command_t *write_location;

    write_location = (command_t *) buffer_write_address (&client->buffer,
                                                         &available_space);
    while (! write_location || available_space < size) {
        sched_yield ();
        write_location = (command_t *) buffer_write_address (&client->buffer,
                                                             &available_space);
    }

    return write_location;
}

command_t *
client_get_space_for_command (command_type_t command_type)
{
    assert (command_type >= 0 && command_type < COMMAND_MAX_COMMAND);

    client_t *client = client_get_thread_local ();
    size_t command_size = command_get_size (command_type);
    command_t *command = client_get_space_for_size (client, command_size);
    command->type = command_type;
    command->size = command_size;
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

    /*pthread_mutex_lock (&client->client_signal_mutex);
    if (client->buffer.last_token < token)
        pthread_cond_wait (&client->client_signal, &client->client_signal_mutex);
    pthread_mutex_unlock (&client->client_signal_mutex);*/
}

void
client_run_command_async (command_t *command)
{
    client_t *client = client_get_thread_local ();

    buffer_write_advance (&client->buffer, command->size);

    if (client->buffer.fill_count == command->size) {
        pthread_mutex_lock (&client->server_signal_mutex);
        pthread_cond_signal (&client->server_signal);
        pthread_mutex_unlock (&client->server_signal_mutex);
    }
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

    return client->active_state->unpack_alignment;
}

bool
client_has_valid_state (client_t *client)
{
    return client->active_state && client->active_state->active;
}

egl_state_t *
client_get_current_state (client_t *client)
{
    return client->active_state;
}

#include "client_autogen.c"
