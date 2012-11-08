
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

    free (server_buffers);

    command_glgenbuffers_destroy_arguments (command);
}

static void
server_custom_handle_gldeletebuffers (server_t *server, command_t *abstract_command)
{
    GLuint *server_buffers;
    int i;

    INSTRUMENT ();

    command_gldeletebuffers_t *command =
            (command_gldeletebuffers_t *)abstract_command;

    server_buffers = (GLuint *)malloc (command->n * sizeof (GLuint));

    for (i=0; i<command->n; i++) {
        server_buffers[i] = *(GLuint *)HashLookup (server->buffer_names_cache,
                                                   command->buffers[i]);
        HashRemove (server->buffer_names_cache, command->buffers[i]);
    }

    server->dispatch.glDeleteBuffers (server, command->n, server_buffers);

    free (server_buffers);
    command_gldeletebuffers_destroy_arguments (command);
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

    server->dispatch.glGenFramebuffers (server, command->n, server_framebuffers);

    for (i=0; i<command->n; i++) {
        GLuint *data = (GLuint *)malloc (sizeof (GLuint));
        *data = server_framebuffers[i];
        HashInsert (server->framebuffer_names_cache, command->framebuffers[i], data);
    }

    free (server_framebuffers);

    command_glgenframebuffers_destroy_arguments (command);
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

static void
server_custom_handle_gldeleteframebuffers (server_t *server, command_t *abstract_command)
{
    GLuint *server_framebuffers;
    int i;

    INSTRUMENT ();

    command_gldeleteframebuffers_t *command =
            (command_gldeleteframebuffers_t *)abstract_command;

    server_framebuffers = (GLuint *)malloc (command->n * sizeof (GLuint));

    for (i=0; i<command->n; i++) {
        server_framebuffers[i] = *(GLuint *)HashLookup (server->framebuffer_names_cache,
                                                        command->framebuffers[i]);
        HashRemove (server->framebuffer_names_cache, command->framebuffers[i]);
    }

    server->dispatch.glDeleteFramebuffers (server, command->n, server_framebuffers);

    free (server_framebuffers);
    command_gldeleteframebuffers_destroy_arguments (command);
}

static void
server_custom_handle_glgentextures (server_t *server, command_t *abstract_command)
{
    GLuint *server_textures;
    int i;

    INSTRUMENT ();

    command_glgentextures_t *command =
            (command_glgentextures_t *)abstract_command;

    server_textures = (GLuint *)malloc (command->n * sizeof (GLuint));

    server->dispatch.glGenTextures (server, command->n, server_textures);

    for (i=0; i<command->n; i++) {
        GLuint *data = (GLuint *)malloc (sizeof (GLuint));
        *data = server_textures[i];
        HashInsert (server->texture_names_cache, command->textures[i], data);
    }

    free (server_textures);

    command_glgentextures_destroy_arguments (command);
}

static void
server_custom_handle_glbindtexture (server_t *server, command_t *abstract_command)
{
    GLuint *texture;
    INSTRUMENT ();

    command_glbindtexture_t *command =
            (command_glbindtexture_t *)abstract_command;

    texture = (GLuint *)HashLookup (server->texture_names_cache,
                                    command->texture);

    server->dispatch.glBindTexture (server, command->target, *texture);

    command_glbindtexture_destroy_arguments (command);
}

static void
server_custom_handle_gldeletetextures (server_t *server, command_t *abstract_command)
{
    GLuint *server_textures;
    int i;

    INSTRUMENT ();

    command_gldeletetextures_t *command =
            (command_gldeletetextures_t *)abstract_command;

    server_textures = (GLuint *)malloc (command->n * sizeof (GLuint));

    for (i=0; i<command->n; i++) {
        server_textures[i] = *(GLuint *)HashLookup (server->texture_names_cache,
                                                    command->textures[i]);
        HashRemove (server->texture_names_cache, command->textures[i]);
    }

    server->dispatch.glDeleteTextures (server, command->n, server_textures);

    free (server_textures);

    command_gldeletetextures_destroy_arguments (command);
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

}

