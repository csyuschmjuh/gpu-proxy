#include "config.h"
#include "server.h"

#include "caching_server_private.h"
#include "ring_buffer.h"
#include "server_dispatch_table.h"
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
    /* FIXME: add exit condition of the loop. */
    while (true) {
        size_t data_left_to_read;
        command_t *read_command = (command_t *) buffer_read_address (server->buffer,
                                                                     &data_left_to_read);

        /* The buffer is empty, so wait until there's something to read. */
        while (! read_command) {
            sleep_nanoseconds (500);
            read_command = (command_t *) buffer_read_address (server->buffer,
                                                              &data_left_to_read);
        }

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
    server->dispatch = *server_dispatch_table_get_base();
    server->command_post_hook = NULL;

    server->handler_table[COMMAND_NO_OP] = server_handle_no_op;
    server_fill_command_handler_table (server);
}

bool
server_destroy (server_t *server)
{
    server->buffer = NULL;

    free (server);
    return true;
}

#include "server_autogen.c"
