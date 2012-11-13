
#include "server_custom.h"

static void
server_custom_handle_glbindbuffer (server_t *server,
                                   command_t *abstract_command)
{
    command_glbindbuffer_t *command =
        (command_glbindbuffer_t *)abstract_command;

    GLuint *server_buffer = (GLuint *)HashLookup (server->name_mapping,
                                                command->buffer);

    server->dispatch.glBindBuffer (server, command->target, *server_buffer);

    command_glbindbuffer_destroy_arguments (command);
}

static void
server_custom_handle_glgenbuffers (server_t *server,
                                   command_t *abstract_command)
{
    command_glgenbuffers_t *command =
        (command_glgenbuffers_t *)abstract_command;

    GLuint *server_buffers = (GLuint *)malloc (command->n * sizeof (GLuint));

    server->dispatch.glGenBuffers (server, command->n, server_buffers);

    int i;
    for (i = 0; i < command->n; i++) {
        GLuint *data = (GLuint *)malloc (sizeof (GLuint));
        *data = server_buffers[i];
        HashInsert (server->name_mapping, command->buffers[i], data);
    }

    free (server_buffers);

    command_glgenbuffers_destroy_arguments (command);
}

static void
server_custom_handle_gldeletebuffers (server_t *server, command_t *abstract_command)
{
    command_gldeletebuffers_t *command =
            (command_gldeletebuffers_t *)abstract_command;

    int i;
    for (i = 0; i < command->n; i++) {
        unsigned client_name = command->buffers[i];
        command->buffers[i] = *(GLuint *)HashLookup (server->name_mapping,
                                                     client_name);
        HashRemove (server->name_mapping, client_name);
    }
    server->dispatch.glDeleteBuffers (server, command->n, command->buffers);

    command_gldeletebuffers_destroy_arguments (command);
}

static void
server_custom_handle_glgenframebuffers (server_t *server, command_t *abstract_command)
{
    command_glgenframebuffers_t *command =
            (command_glgenframebuffers_t *)abstract_command;

    GLuint *server_framebuffers = (GLuint *)malloc (command->n * sizeof (GLuint));

    server->dispatch.glGenFramebuffers (server, command->n, server_framebuffers);

    int i;
    for (i = 0; i < command->n; i++) {
        GLuint *data = (GLuint *)malloc (sizeof (GLuint));
        *data = server_framebuffers[i];
        HashInsert (server->name_mapping, command->framebuffers[i], data);
    }

    free (server_framebuffers);

    command_glgenframebuffers_destroy_arguments (command);
}

static void
server_custom_handle_glbindframebuffer (server_t *server, command_t *abstract_command)
{
    command_glbindframebuffer_t *command =
            (command_glbindframebuffer_t *)abstract_command;
    GLuint *server_buffer = (GLuint *)HashLookup (server->name_mapping,
                                                  command->framebuffer);
    server->dispatch.glBindFramebuffer (server, command->target, *server_buffer);
    command_glbindframebuffer_destroy_arguments (command);
}

static void
server_custom_handle_gldeleteframebuffers (server_t *server, command_t *abstract_command)
{
    command_gldeleteframebuffers_t *command =
            (command_gldeleteframebuffers_t *)abstract_command;

    int i;
    for (i = 0; i < command->n; i++) {
        unsigned client_name = command->framebuffers[i];
        command->framebuffers[i] = *(GLuint *)HashLookup (server->name_mapping,
                                                          client_name);
        HashRemove (server->name_mapping, command->framebuffers[i]);
    }
    server->dispatch.glDeleteFramebuffers (server, command->n, command->framebuffers);

    command_gldeleteframebuffers_destroy_arguments (command);
}

static void
server_custom_handle_glgentextures (server_t *server, command_t *abstract_command)
{
    command_glgentextures_t *command =
            (command_glgentextures_t *)abstract_command;

    GLuint *server_textures = (GLuint *)malloc (command->n * sizeof (GLuint));
    server->dispatch.glGenTextures (server, command->n, server_textures);

    int i;
    for (i = 0; i < command->n; i++) {
        GLuint *data = (GLuint *)malloc (sizeof (GLuint));
        *data = server_textures[i];
        HashInsert (server->name_mapping, command->textures[i], data);
    }
    free (server_textures);

    command_glgentextures_destroy_arguments (command);
}

static void
server_custom_handle_glbindtexture (server_t *server, command_t *abstract_command)
{
    command_glbindtexture_t *command =
            (command_glbindtexture_t *)abstract_command;
    GLuint *texture = (GLuint *)HashLookup (server->name_mapping,
                                            command->texture);
    server->dispatch.glBindTexture (server, command->target, *texture);
    command_glbindtexture_destroy_arguments (command);
}

static void
server_custom_handle_gldeletetextures (server_t *server, command_t *abstract_command)
{
    command_gldeletetextures_t *command =
            (command_gldeletetextures_t *)abstract_command;

    int i;
    for (i = 0; i < command->n; i++) {
        unsigned client_name = command->textures[i];
        command->textures[i] = *(GLuint *)HashLookup (server->name_mapping,
                                                      client_name);
        HashRemove (server->name_mapping, command->textures[i]);
    }

    server->dispatch.glDeleteTextures (server, command->n, command->textures);

    command_gldeletetextures_destroy_arguments (command);
}

static void
server_custom_handle_glgenrenderbuffers (server_t *server,
                                          command_t *abstract_command)
{
    command_glgenrenderbuffers_t *command =
        (command_glgenrenderbuffers_t *)abstract_command;

    GLuint *server_renderbuffers = (GLuint *)malloc (command->n * sizeof (GLuint));
    server->dispatch.glGenRenderbuffers (server, command->n, server_renderbuffers);

    int i;
    for (i = 0; i < command->n; i++) {
        GLuint *data = (GLuint *)malloc (sizeof (GLuint));
        *data = server_renderbuffers[i];
        HashInsert (server->name_mapping, command->renderbuffers[i], data);
    }

    free (server_renderbuffers);
    command_glgenrenderbuffers_destroy_arguments (command);
}

static void
server_custom_handle_gldeleterenderbuffers (server_t *server, command_t *abstract_command)
{
    command_gldeleterenderbuffers_t *command =
            (command_gldeleterenderbuffers_t *)abstract_command;
    int i;
    for (i=0; i < command->n; i++) {
        unsigned client_name = command->renderbuffers[i];
        command->renderbuffers[i] = *(GLuint *)HashLookup (server->name_mapping,
                                                           client_name);
        HashRemove (server->name_mapping, command->renderbuffers[i]);
    }

    server->dispatch.glDeleteBuffers (server, command->n, command->renderbuffers);
    command_gldeleterenderbuffers_destroy_arguments (command);
}

static void
server_custom_handle_glbindrenderbuffer (server_t *server, command_t *abstract_command)
{
    command_glbindrenderbuffer_t *command =
            (command_glbindrenderbuffer_t *)abstract_command;
    GLuint *renderbuffer = (GLuint *)HashLookup (server->name_mapping,
                                                 command->renderbuffer);
    server->dispatch.glBindRenderbuffer (server, command->target, *renderbuffer);
    command_glbindrenderbuffer_destroy_arguments (command);
}

void
server_add_custom_command_handlers (server_t *server) {
    server->handler_table[COMMAND_GLBINDBUFFER] =
        server_custom_handle_glbindbuffer;
    server->handler_table[COMMAND_GLBINDFRAMEBUFFER] =
        server_custom_handle_glbindframebuffer;
    server->handler_table[COMMAND_GLBINDTEXTURE] =
        server_custom_handle_glbindtexture;
    server->handler_table[COMMAND_GLDELETEBUFFERS] =
        server_custom_handle_gldeletebuffers;
    server->handler_table[COMMAND_GLDELETEFRAMEBUFFERS] =
        server_custom_handle_gldeleteframebuffers;
    server->handler_table[COMMAND_GLDELETETEXTURES] =
        server_custom_handle_gldeletetextures;
    server->handler_table[COMMAND_GLGENBUFFERS] =
        server_custom_handle_glgenbuffers;
    server->handler_table[COMMAND_GLGENFRAMEBUFFERS] =
        server_custom_handle_glgenframebuffers;
    server->handler_table[COMMAND_GLGENTEXTURES] =
        server_custom_handle_glgentextures;

    server->handler_table[COMMAND_GLBINDRENDERBUFFER] =
        server_custom_handle_glbindrenderbuffer;
    server->handler_table[COMMAND_GLGENRENDERBUFFERS] =
        server_custom_handle_glgenrenderbuffers;
    server->handler_table[COMMAND_GLDELETERENDERBUFFERS] =
        server_custom_handle_gldeleterenderbuffers;
}

