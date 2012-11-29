#include "config.h"
#include "server.h"

#include "ring_buffer.h"
#include "dispatch_table.h"
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
         /*int times_slept = 0;
         while (! read_command && times_slept < 20) {
             sleep_nanoseconds (500);
             read_command = (command_t *) buffer_read_address (server->buffer,
                                                           &data_left_to_read);
             times_slept++;
         }*/

        /* We ran out of hot cycles, try a more lackadaisical approach. */
        if (! read_command) {
            pthread_mutex_lock (server->server_signal_mutex);
            read_command = (command_t *) buffer_read_address (server->buffer,
                                                               &data_left_to_read);
            while (! read_command) {
                pthread_cond_wait (server->server_signal, server->server_signal_mutex);
                read_command = (command_t *) buffer_read_address (server->buffer,
                                                                   &data_left_to_read);
            }
            pthread_mutex_unlock (server->server_signal_mutex);
        }

        if (read_command->type == COMMAND_SHUTDOWN)
            break;

        server->handler_table[read_command->type](server, read_command);
        buffer_read_advance (server->buffer, read_command->size);

        /*if (read_command->token) {
            pthread_mutex_lock (server->client_signal_mutex);
            pthread_cond_signal (server->client_signal);
            pthread_mutex_unlock (server->client_signal_mutex);
        }*/
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

mutex_static_init (name_mapping_mutex);
static HashTable *name_mapping = NULL;

static void
server_handle_glgenbuffers (server_t *server,
                                   command_t *abstract_command)
{
    INSTRUMENT();

    command_glgenbuffers_t *command =
        (command_glgenbuffers_t *)abstract_command;

    GLuint *server_buffers = (GLuint *)malloc (command->n * sizeof (GLuint));
    server->dispatch.glGenBuffers (server, command->n, server_buffers);

    int i;
    mutex_lock (name_mapping_mutex);
    for (i = 0; i < command->n; i++) {
        GLuint *data = (GLuint *)malloc (sizeof (GLuint));
        *data = server_buffers[i];
        HashInsert (name_mapping, command->buffers[i], data);
    }
    mutex_unlock (name_mapping_mutex);

    free (server_buffers);
    command_glgenbuffers_destroy_arguments (command);
}

static void
server_handle_gldeletebuffers (server_t *server, command_t *abstract_command)
{
    INSTRUMENT();

    command_gldeletebuffers_t *command =
        (command_gldeletebuffers_t *)abstract_command;

    int i;
    mutex_lock (name_mapping_mutex);
    for (i = 0; i < command->n; i++) {
        GLuint *entry = HashTake (name_mapping, command->buffers[i]);
        if (entry) {
            command->buffers[i] = *entry;
            free (entry);
        }
    }
    mutex_unlock (name_mapping_mutex);

    server->dispatch.glDeleteBuffers (server, command->n, command->buffers);

    command_gldeletebuffers_destroy_arguments (command);
}

static void
server_handle_glgenframebuffers (server_t *server, command_t *abstract_command)
{
    INSTRUMENT();

    command_glgenframebuffers_t *command =
        (command_glgenframebuffers_t *)abstract_command;

    GLuint *server_framebuffers = (GLuint *)malloc (command->n * sizeof (GLuint));
    server->dispatch.glGenFramebuffers (server, command->n, server_framebuffers);

    int i;
    mutex_lock (name_mapping_mutex);
    for (i = 0; i < command->n; i++) {
        GLuint *data = (GLuint *)malloc (sizeof (GLuint));
        *data = server_framebuffers[i];
        HashInsert (name_mapping, command->framebuffers[i], data);
    }
    mutex_unlock (name_mapping_mutex);

    free (server_framebuffers);
    command_glgenframebuffers_destroy_arguments (command);
}

static void
server_handle_gldeleteframebuffers (server_t *server, command_t *abstract_command)
{
    INSTRUMENT();

    command_gldeleteframebuffers_t *command =
        (command_gldeleteframebuffers_t *)abstract_command;

    int i;
    mutex_lock (name_mapping_mutex);
    for (i = 0; i < command->n; i++) {
        GLuint *entry = HashTake (name_mapping, command->framebuffers[i]);
        if (entry) {
            command->framebuffers[i] = *entry;
            free (entry);
        }
    }
    mutex_unlock (name_mapping_mutex);

    server->dispatch.glDeleteFramebuffers (server, command->n, command->framebuffers);

    command_gldeleteframebuffers_destroy_arguments (command);
}

static void
server_handle_glgentextures (server_t *server, command_t *abstract_command)
{
    INSTRUMENT();

    command_glgentextures_t *command =
        (command_glgentextures_t *)abstract_command;

    GLuint *server_textures = (GLuint *)malloc (command->n * sizeof (GLuint));
    server->dispatch.glGenTextures (server, command->n, server_textures);

    int i;
    mutex_lock (name_mapping_mutex);
    for (i = 0; i < command->n; i++) {
        GLuint *data = (GLuint *)malloc (sizeof (GLuint));
        *data = server_textures[i];
        HashInsert (name_mapping, command->textures[i], data);
    }
    mutex_unlock (name_mapping_mutex);

    free (server_textures);
    command_glgentextures_destroy_arguments (command);
}

static void
server_handle_gldeletetextures (server_t *server, command_t *abstract_command)
{
    INSTRUMENT();

    command_gldeletetextures_t *command =
        (command_gldeletetextures_t *)abstract_command;

    int i;
    mutex_lock (name_mapping_mutex);
    for (i = 0; i < command->n; i++) {
        GLuint *entry = HashTake (name_mapping, command->textures[i]);
        if (entry) {
            command->textures[i] = *entry;
            free (entry);
        }
    }
    mutex_unlock (name_mapping_mutex);

    server->dispatch.glDeleteTextures (server, command->n, command->textures);

    command_gldeletetextures_destroy_arguments (command);
}

static void
server_handle_glgenrenderbuffers (server_t *server,
                                         command_t *abstract_command)
{
    INSTRUMENT();

    command_glgenrenderbuffers_t *command =
        (command_glgenrenderbuffers_t *)abstract_command;

    GLuint *server_renderbuffers = (GLuint *)malloc (command->n * sizeof (GLuint));
    server->dispatch.glGenRenderbuffers (server, command->n, server_renderbuffers);

    int i;
    mutex_lock (name_mapping_mutex);
    for (i = 0; i < command->n; i++) {
        GLuint *data = (GLuint *)malloc (sizeof (GLuint));
        *data = server_renderbuffers[i];
        HashInsert (name_mapping, command->renderbuffers[i], data);
    }
    mutex_unlock (name_mapping_mutex);

    free (server_renderbuffers);
    command_glgenrenderbuffers_destroy_arguments (command);
}

static void
server_handle_gldeleterenderbuffers (server_t *server, command_t *abstract_command)
{
    INSTRUMENT();

    command_gldeleterenderbuffers_t *command =
        (command_gldeleterenderbuffers_t *)abstract_command;

    int i;
    mutex_lock (name_mapping_mutex);
    for (i = 0; i < command->n; i++) {
        GLuint *entry = HashTake (name_mapping, command->renderbuffers[i]);
        if (entry) {
            command->renderbuffers[i] = *entry;
            free (entry);
        }
    }
    mutex_unlock (name_mapping_mutex);

    server->dispatch.glDeleteBuffers (server, command->n, command->renderbuffers);
    command_gldeleterenderbuffers_destroy_arguments (command);
}

void
server_init (server_t *server,
             buffer_t *buffer)
{
    server->buffer = buffer;
    server->dispatch = *dispatch_table_get_base();
    server->command_post_hook = NULL;

    server->handler_table[COMMAND_NO_OP] = server_handle_no_op;
    server_fill_command_handler_table (server);

    server->handler_table[COMMAND_GLGENBUFFERS] =
        server_handle_glgenbuffers;
    server->handler_table[COMMAND_GLDELETEBUFFERS] =
        server_handle_gldeletebuffers;
    server->handler_table[COMMAND_GLDELETEFRAMEBUFFERS] =
        server_handle_gldeleteframebuffers;
    server->handler_table[COMMAND_GLGENFRAMEBUFFERS] =
        server_handle_glgenframebuffers;
    server->handler_table[COMMAND_GLGENTEXTURES] =
        server_handle_glgentextures;
    server->handler_table[COMMAND_GLDELETETEXTURES] =
        server_handle_gldeletetextures;
    server->handler_table[COMMAND_GLGENRENDERBUFFERS] =
        server_handle_glgenrenderbuffers;
    server->handler_table[COMMAND_GLDELETERENDERBUFFERS] =
        server_handle_gldeleterenderbuffers;

    mutex_lock (name_mapping_mutex);
    if (name_mapping)
        return;
    name_mapping = NewHashTable(free);
    mutex_unlock (name_mapping_mutex);
}

bool
server_destroy (server_t *server)
{
    free (server);
    return true;
}

#include "server_autogen.c"
