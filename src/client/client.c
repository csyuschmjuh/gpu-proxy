#define _GNU_SOURCE
#include <sched.h>

#include "config.h"
#include "client.h"

#include "caching_client.h"
#include "caching_client_private.h"
#include "command.h"
#include "name_handler.h"

#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>

__thread client_t* thread_local_client
    __attribute__(( tls_model ("initial-exec"))) = NULL;

__thread bool client_thread
    __attribute__(( tls_model ("initial-exec"))) = true;

__thread bool initializing_client
    __attribute__(( tls_model ("initial-exec"))) = false;

__thread bool initialized
    __attribute__(( tls_model ("initial-exec"))) = false;

mutex_static_init (client_thread_mutex);

static buffer_t pilot_buffer;
static bool pilot_buffer_created = false;
static mutex_t pilot_command_mutex;
static unsigned int pilot_token = 0;

static sem_t *
get_pilot_server_signal ()
{
    static sem_t signal; 
    static bool sem_initialized = false;
    
    if ( sem_initialized == false) {
        sem_init (&signal, 0, 0);
        sem_initialized = true;
    }

    return &signal;
}

static thread_t
get_pilot_server ()
{
    static thread_t pilot_thread = 0;
    return pilot_thread;
}

void *
start_pilot_server_thread_func (void *ptr)
{
    client_thread = false;
    pilot_server_t *server = pilot_server_new (&pilot_buffer);
    server->server_signal = get_pilot_server_signal ();

    prctl (PR_SET_TIMERSLACK, 1);

    pthread_t id = pthread_self ();
    
    int online_cpus = sysconf (_SC_NPROCESSORS_ONLN);
    int available_cpus = sysconf (_SC_NPROCESSORS_CONF);

    if (online_cpus > 1) {
        cpu_set_t cpu_set;
        CPU_ZERO (&cpu_set);
        if (pthread_getaffinity_np (id, sizeof (cpu_set_t), &cpu_set) == 0) {

            /* find first cpu to run on */
            int cpu = 0;
            int i;
            for (i = 1; i < available_cpus; i++) {
                if (CPU_ISSET (i, &cpu_set)) {
                    cpu = i;
                    break;
                }
            }
            /* force server to run on cpu1 */
            if (cpu == 0)
                cpu = 1;
            if (cpu != 0) {
                for (i = 0; i < available_cpus; i++) {
                    if (i != cpu)
                        CPU_CLR (i, &cpu_set);
                }
                CPU_SET (cpu, &cpu_set);
                pthread_setaffinity_np (id, sizeof (cpu_set_t), &cpu_set);
            }
        }
    }

    pilot_server_start_work_loop (server);

    pilot_server_destroy(server);
    return NULL;
}

void
client_start_pilot_server (client_t *client)
{
    thread_t pilot_thread = get_pilot_server ();
    if (pilot_thread == 0) {
        pthread_create (&pilot_thread, NULL, start_pilot_server_thread_func, client);
    }
}

static void
client_fill_dispatch_table (dispatch_table_t *client);

static bool
on_client_thread ()
{
    if (!client_thread)
	return client_thread;

    if (initialized)
        return client_thread;

    mutex_lock (client_thread_mutex);
    if (initialized)
        return client_thread;
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
    client_thread = false;
    client_t *client = (client_t *)ptr;
    server_t *server = server_new (&client->buffer);
    server->client_thread = client->client_thread;

    server->server_signal = &client->server_signal;
    server->client_signal = &client->client_signal;

    mutex_unlock (client->server_started_mutex);
    prctl (PR_SET_TIMERSLACK, 1);

    pthread_t id = pthread_self ();
    
    int online_cpus = sysconf (_SC_NPROCESSORS_ONLN);
    int available_cpus = sysconf (_SC_NPROCESSORS_CONF);

    if (online_cpus > 1) {
        cpu_set_t cpu_set;
        CPU_ZERO (&cpu_set);
        if (pthread_getaffinity_np (id, sizeof (cpu_set_t), &cpu_set) == 0) {

            /* find first cpu to run on */
            int cpu = 0;
            int i;
            for (i = 1; i < available_cpus; i++) {
                if (CPU_ISSET (i, &cpu_set)) {
                    cpu = i;
                    break;
                }
            }
            /* force server to run on cpu1 */
            if (cpu == 0)
                cpu = 1;
            if (cpu != 0) {
                for (i = 0; i < available_cpus; i++) {
                    if (i != cpu)
                        CPU_CLR (i, &cpu_set);
                }
                CPU_SET (cpu, &cpu_set);
                pthread_setaffinity_np (id, sizeof (cpu_set_t), &cpu_set);
            }
        }
    }

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
   
    /* FIXME:
     * We did some experiments, If on PC with quard core (show 8 CPUs)
     * without pin client thread is faster, we can see client thread
     * being scheduled on different cores, except the core server thread
     * is running, but on ARM with hotplug, it seems that the kernel
     * will sometime schedule the client and server to run on the same core
     * which slows down the performance.  So here is the HACK 
     */
    int available_cpus = sysconf (_SC_NPROCESSORS_CONF);

    if (available_cpus <= 4) {
        pthread_t id = pthread_self ();
        cpu_set_t cpu_set;
        CPU_ZERO (&cpu_set);
        CPU_SET (0, &cpu_set);
        pthread_setaffinity_np (id, sizeof (cpu_set_t), &cpu_set);
    }
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

    buffer_create (&client->buffer, 1024, "command");

    // We initialize the base dispatch table synchronously here, so that we
    // don't have to worry about the server thread trying to initialize it
    // at the same time.
    dispatch_table_get_base ();

    client_fill_dispatch_table (&client->dispatch);

    client->token = 0;

    sem_init (&client->server_signal, 0, 0);
    sem_init (&client->client_signal, 0, 0);

    client->active_state = NULL;
   
    client_start_server (client);
    initializing_client = false;

    /* initialize share pilot channel */
    if (pilot_buffer_created == false) {
	buffer_create (&pilot_buffer, 512, "pilot");
        mutex_init (pilot_command_mutex);
        pilot_buffer_created = true;
    }

    thread_t pilot_server = get_pilot_server ();
    if (! pilot_server)
        client_start_pilot_server (client);

    client->client_thread = pthread_self ();
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

    sem_destroy (&client->server_signal);
    sem_destroy (&client->client_signal);

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
client_get_space_for_size (client_t *client,
                           size_t size)
{
    size_t available_space;
    command_t *write_location;

    if (size > buffer_size (&client->buffer))
        return NULL;

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
    /* Command size is never bigger than the buffer, no NULL check. */
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

    while (client->buffer.last_token < token) {
        sem_wait (&client->client_signal);
    }
}

inline void
client_run_command_async (command_t *command)
{
    client_t *client = client_get_thread_local ();

    buffer_write_advance (&client->buffer, command->size);

    if (client->buffer.fill_count == command->size) {
        sem_post (&client->server_signal);
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

int
client_get_unpack_row_length ()
{
    client_t *client = client_get_thread_local ();
    if (!client->active_state)
        return 0;

    return client->active_state->unpack_row_length;
}

int
client_get_unpack_skip_pixels ()
{
    client_t *client = client_get_thread_local ();
    if (!client->active_state)
        return 0;

    return client->active_state->unpack_skip_pixels;
}

int
client_get_unpack_skip_rows ()
{
    client_t *client = client_get_thread_local ();
    if (!client->active_state)
        return 0;

    return client->active_state->unpack_skip_rows;
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

command_t *
client_get_register_space_for_size (client_t *client,
                                    size_t size)
{
    size_t available_space;
    command_t *write_location;

    if (size > buffer_size (&pilot_buffer))
        return NULL;

    write_location = (command_t *) buffer_write_address (&pilot_buffer,
                                                         &available_space);
    while (! write_location || available_space < size) {
        sched_yield ();
        write_location = (command_t *) buffer_write_address (&pilot_buffer,
                                                             &available_space);
    }

    return write_location;
}

command_t *
client_get_register_space_for_command (command_type_t command_type)
{
    assert (command_type >= 0 && command_type < COMMAND_MAX_COMMAND);

    client_t *client = client_get_thread_local ();
    size_t command_size = command_get_size (command_type);
    command_t *command = client_get_register_space_for_size (client, command_size);
    /* Command size is never bigger than the buffer, no NULL check. */
    command->type = command_type;
    command->size = command_size;
    command->token = 0;
    return command;
}

command_t *
client_get_space_for_register_command (command_type_t command_type)
{
    assert (command_type >= 0 && command_type < COMMAND_MAX_COMMAND);

    client_t *client = client_get_thread_local ();
    size_t command_size = command_get_size (command_type);
    command_t *command = client_get_register_space_for_size (client, command_size);
    /* Command size is never bigger than the buffer, no NULL check. */
    command->type = command_type;
    command->size = command_size;
    command->token = 0;
    return command;
}

static inline void
client_run_register_command_async (command_t *command)
{
    buffer_write_advance (&pilot_buffer, command->size);

    if (pilot_buffer.fill_count == command->size) {
        sem_t *pilot_signal = get_pilot_server_signal ();
        sem_post (pilot_signal);
    }
}

static void
client_run_register_command (command_t *command)
{
    pilot_token ++;

    /* Overflow case */
    if (pilot_token == 0)
        pilot_token = 1;

    command->token = pilot_token;
    client_run_register_command_async (command);

    while (pilot_buffer.last_token < pilot_token) {
        sched_yield ();
    }
}

bool
client_register_lock_surface (client_t *client, EGLDisplay display, EGLSurface surface)
{
    command_t *command = client_get_register_space_for_command (COMMAND_REGISTERSURFACE);
    command_registersurface_init (command, display, surface, client->client_thread);
    client_run_register_command (command);
    return ((command_registersurface_t *)command)->result;
}

bool
client_register_lock_context (client_t *client, EGLDisplay display, EGLContext context)
{
    command_t *command = client_get_register_space_for_command (COMMAND_REGISTERCONTEXT);
    command_registersurface_init (command, display, context, client->client_thread);
    client_run_register_command (command);
    return  ((command_registercontext_t *)command)->result;
}

bool
client_register_lock_image (client_t *client, EGLDisplay display, EGLImageKHR image)
{
    mutex_lock (pilot_command_mutex);
    command_t *command = client_get_register_space_for_command (COMMAND_REGISTERIMAGE);
    command_registerimage_init (command, display, image, client->client_thread);
    client_run_register_command (command);
    return ((command_registerimage_t *)command)->result;
}

void
client_lock_pilot_mutex ()
{
    mutex_lock (pilot_command_mutex);
}

void
client_unlock_pilot_mutex ()
{
    mutex_unlock (pilot_command_mutex);
}

#include "client_autogen.c"
