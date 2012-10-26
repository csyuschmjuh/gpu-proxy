
#include "server_custom.h"

static void
server_custom_handle_glbindbuffer (server_t *server,
                                   command_t *abstract_command)
{
    GLuint *server_buffer;
    command_glbindbuffer_t *command =
        (command_glbindbuffer_t *)abstract_command;

    INSTRUMENT ();

    server_buffer = (GLuint *)HashLookup (server->names_cache,
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
        HashInsert (server->names_cache, command->buffers[i], data);
    }

    free (server_buffers);
}

void
server_add_custom_command_handlers (server_t *server) {
    server->handler_table[COMMAND_GLBINDBUFFER] =
        server_custom_handle_glbindbuffer;
    server->handler_table[COMMAND_GLGENBUFFERS] =
        server_custom_handle_glgenbuffers;
}

