#include "config.h"
#include "caching_client.h"

__thread client_t* thread_local_caching_client
    __attribute__(( tls_model ("initial-exec"))) = NULL;

__thread bool caching_client_thread
    __attribute__(( tls_model ("initial-exec"))) = false;

mutex_static_init (caching_client_thread_mutex);

bool
on_caching_client_thread ()
{
    static bool initialized = false;
    if (initialized)
        return caching_client_thread;

    mutex_lock (caching_client_thread_mutex);
    caching_client_thread = true;
    initialized = true;
    mutex_unlock (caching_client_thread_mutex);

    return caching_client_thread;
}

client_t *
caching_client_new ()
{
    caching_client_t *client = (caching_client_t *)malloc (sizeof (caching_client_t));

    client->base.name_handler = name_handler_create ();
    client->base.token = 0;
    client->active_state = NULL;

    buffer_create (&client->base.buffer);
    client_start_server (&client->base);

    return &client->base;
}

static bool
caching_client_destroy (caching_client_t *client)
{
    /* TODO: Send a message to the server to signal a stop. */
    pthread_join (client->base.server_thread, NULL);

    buffer_free (&client->base.buffer);

    name_handler_destroy (client->base.name_handler);
    free (client);

    return true;
}

client_t *
caching_client_get_thread_local ()
{
    if (unlikely (! thread_local_caching_client))
        thread_local_caching_client = client_new ();
    return thread_local_caching_client;
}

void
caching_client_destroy_thread_local ()
{
    if (! thread_local_caching_client)
        return;

     caching_client_destroy ((caching_client_t *)thread_local_caching_client);
     thread_local_caching_client = NULL;
}


void
caching_client_run_command (command_t *command)
{
    client_t *client = caching_client_get_thread_local ();
    unsigned int token = ++client->token;

    /* Overflow case */
    if (token == 0)
	token = 1;
    
    command->token = token;
    caching_client_run_command_async (command);

    while (client->buffer.last_token < token)
        sched_yield ();
}

void
caching_client_run_command_async (command_t *command)
{
    client_t *client = caching_client_get_thread_local ();
    buffer_write_advance (&client->buffer, command->size);
}

bool
caching_client_flush (client_t *client)
{
    return true;
}
