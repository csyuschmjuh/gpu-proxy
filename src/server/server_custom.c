
#include "server_custom.h"

static void
server_custom_handle_glbindbuffer (server_t *server,
                                   command_t *abstract_command)
{
    GLuint *server_buffer;
    command_glbindbuffer_t *command =
        (command_glbindbuffer_t *)abstract_command;

    INSTRUMENT ();

    server_buffer = (GLuint *)HashLookup (server->buffer_names_cache,
                                          command->buffer);

    server->dispatch.glBindBuffer (server, command->target, *server_buffer);

    command_glbindbuffer_destroy_arguments (command);
}

static void
server_custom_handle_glgenbuffers (server_t *server,
                                   command_t *abstract_command)
{
    GLuint *server_buffers;
    int i;
    command_glgenbuffers_t *command =
        (command_glgenbuffers_t *)abstract_command;

    INSTRUMENT ();

    server_buffers = (GLuint *)malloc (command->n * sizeof (GLuint));

    server->dispatch.glGenBuffers (server, command->n, server_buffers);

    for (i=0; i<command->n; i++) {
        GLuint *data = (GLuint *)malloc (sizeof (GLuint));
        *data = server_buffers[i];
        HashInsert (server->buffer_names_cache, command->buffers[i], data);
    }

    free (command->buffers);
    free (server_buffers);
}

static void
server_custom_handle_glgenframebuffers (server_t *server, command_t *abstract_command)
{
    GLuint *server_framebuffers;
    int i;

    command_glgenframebuffers_t *command =
            (command_glgenframebuffers_t *)abstract_command;

    INSTRUMENT ();

    server_framebuffers = (GLuint *)malloc (command->n * sizeof (GLuint));

    server->dispatch.glGenFramebuffers (server, command->n, command->framebuffers);

    for (i=0; i<command->n; i++) {
        GLuint *data = (GLuint *)malloc (sizeof (GLuint));
        *data = server_framebuffers[i];
        HashInsert (server->framebuffer_names_cache, command->framebuffers[i], data);
    }

    free (command->framebuffers);
    free (server_framebuffers);
}

static void
server_custom_handle_glbindframebuffer (server_t *server, command_t *abstract_command)
{
    GLuint *server_buffer;
    command_glbindframebuffer_t *command =
            (command_glbindframebuffer_t *)abstract_command;

    INSTRUMENT ();

    server_buffer = (GLuint *)HashLookup (server->framebuffer_names_cache,
                                          command->framebuffer);

    server->dispatch.glBindFramebuffer (server, command->target, *server_buffer);

    command_glbindframebuffer_destroy_arguments (command);
}

void
server_add_custom_command_handlers (server_t *server) {
    server->handler_table[COMMAND_GLBINDBUFFER] =
        server_custom_handle_glbindbuffer;
    server->handler_table[COMMAND_GLBINDFRAMEBUFFER] =
        server_custom_handle_glbindframebuffer;
    server->handler_table[COMMAND_GLGENBUFFERS] =
        server_custom_handle_glgenbuffers;
    server->handler_table[COMMAND_GLGENFRAMEBUFFERS] =
        server_custom_handle_glgenframebuffers;
}

