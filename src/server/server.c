#include "config.h"
#include "server.h"

#include "ring_buffer.h"
#include "dispatch_table.h"
#include "server_custom.h"
#include "thread_private.h"
#include <time.h>

/* This method is auto-generated into server_autogen.c
 * and included at the end of this file. */
static void
server_fill_command_handler_table (server_t *server);

static inline void
sleep_nanoseconds (int num_nanoseconds)
{
    struct timespec spec;
    spec.tv_sec = 0;
    spec.tv_nsec = num_nanoseconds;
    nanosleep (&spec, NULL);
}

void
server_start_work_loop (server_t *server)
{
    while (true) {
        size_t data_left_to_read;
        command_t *read_command = (command_t *) buffer_read_address (server->buffer,
                                                                     &data_left_to_read);
        /* The buffer is empty, so wait until there's something to read. */
         int times_slept = 0;
         while (! read_command && times_slept < 20) {
             sleep_nanoseconds (500);
             read_command = (command_t *) buffer_read_address (server->buffer,
                                                           &data_left_to_read);
             times_slept++;
         }

        /* We ran out of hot cycles, try a more lackadaisical approach. */
        if (! read_command) {
            pthread_mutex_lock (server->signal_mutex);
            read_command = (command_t *) buffer_read_address (server->buffer,
                                                               &data_left_to_read);
            while (! read_command) {
                pthread_cond_wait (server->signal, server->signal_mutex);
                read_command = (command_t *) buffer_read_address (server->buffer,
                                                                   &data_left_to_read);
            }
            pthread_mutex_unlock (server->signal_mutex);
        }

        if (read_command->type == COMMAND_SHUTDOWN)
            break;

        server->handler_table[read_command->type](server, read_command);
        buffer_read_advance (server->buffer, read_command->size);

        if (read_command->token)
            server->buffer->last_token = read_command->token;
    }
}

server_t *
server_new (buffer_t *buffer)
{
    server_t *server = malloc (sizeof (server_t));
    server_init (server, buffer);
    return server;
}

static void
server_handle_no_op (server_t *server,
                     command_t *command)
{
    return;
}

void
server_init (server_t *server,
             buffer_t *buffer)
{
    server->buffer = buffer;
    server->dispatch = *dispatch_table_get_base();
    server->command_post_hook = NULL;

    server->handler_table[COMMAND_NO_OP] = server_handle_no_op;
    server->buffer_names_cache = NewHashTable();
    server->framebuffer_names_cache = NewHashTable();
    server->texture_names_cache = NewHashTable();

    server_fill_command_handler_table (server);
    server_add_custom_command_handlers (server);
}

bool
server_destroy (server_t *server)
{
    server->buffer = NULL;

    HashWalk (server->buffer_names_cache, FreeDataCallback, NULL);
    DeleteHashTable (server->buffer_names_cache);

    HashWalk (server->framebuffer_names_cache, FreeDataCallback, NULL);
    DeleteHashTable (server->framebuffer_names_cache);

    HashWalk (server->texture_names_cache, FreeDataCallback, NULL);
    DeleteHashTable (server->texture_names_cache);

    free (server);
    return true;
}

#include "server_autogen.c"
