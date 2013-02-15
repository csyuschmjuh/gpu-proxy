#include "client.h"
#include "command.h"
#include "gles2_utils.h"
#include <string.h>

void
command_glteximage2d_init (command_t *abstract_command,
                           GLenum target,
                           GLint level,
                           GLint internalformat,
                           GLsizei width,
                           GLsizei height,
                           GLint border,
                           GLenum format,
                           GLenum type,
                           const void* pixels)
{
    uint32_t dest_size;
    uint32_t unpadded_row_size;
    uint32_t padded_row_size;
    uint32_t unpack_alignment = client_get_unpack_alignment ();
    uint32_t unpack_row_length = client_get_unpack_row_length ();
    uint32_t unpack_skip_pixels = client_get_unpack_skip_pixels ();
    uint32_t unpack_skip_rows = client_get_unpack_skip_rows ();

    if (!compute_image_data_sizes (width, height, format, type,
                                   unpack_alignment, unpack_row_length, unpack_skip_rows, &dest_size,
                                   &unpadded_row_size, &padded_row_size)) {
        /* TODO: Set an error on the client-side.
         SetGLError(GL_INVALID_VALUE, "glTexImage2D", "dimension < 0"); */
        return;
    }


    size_t command_size = 0;

    if (pixels) {
        client_t *client = client_get_thread_local ();

        command_size = command_get_size (COMMAND_GLTEXIMAGE2D);

        abstract_command = client_get_space_for_size (client, command_size + dest_size);
        abstract_command->type = COMMAND_GLTEXIMAGE2D;
        abstract_command->size = command_size + dest_size;
        abstract_command->token = 0;

    }

    command_glteximage2d_t *command =
        (command_glteximage2d_t *) abstract_command;
    command->target = (GLenum) target;
    command->level = (GLint) level;
    command->internalformat = (GLint) internalformat;
    command->width = (GLsizei) width;
    command->height = (GLsizei) height;
    command->border = (GLint) border;
    command->format = (GLenum) format;
    command->type = (GLenum) type;
    command->pixels = NULL;

    if (! pixels)
        return;

    command->pixels = malloc (dest_size);
    copy_rect_to_buffer (pixels, command->pixels, format, type, height,
                         unpack_skip_pixels, unpack_skip_rows,
                         unpadded_row_size, padded_row_size, padded_row_size);
}

void
command_gltexsubimage2d_init (command_t *abstract_command,
                              GLenum target,
                              GLint level,
                              GLint xoffset,
                              GLint yoffset,
                              GLsizei width,
                              GLsizei height,
                              GLenum format,
                              GLenum type,
                              const void* pixels)
{
    uint32_t dest_size;
    uint32_t unpadded_row_size;
    uint32_t padded_row_size;
    uint32_t unpack_alignment = client_get_unpack_alignment ();
    uint32_t unpack_row_length = client_get_unpack_row_length ();
    uint32_t unpack_skip_pixels = client_get_unpack_skip_pixels ();
    uint32_t unpack_skip_rows = client_get_unpack_skip_rows ();

    if (! compute_image_data_sizes (width, height, format,
                                    type, unpack_alignment, unpack_skip_rows, unpack_row_length,
                                    &dest_size, &unpadded_row_size,
                                    &padded_row_size)) {
        /* XXX: Set a GL error on the client side here. */
        /* SetGLError(GL_INVALID_VALUE, "glTexSubImage2D", "size to large"); */
        return;
    }


    size_t command_size = 0;

    if (pixels) {
        client_t *client = client_get_thread_local ();

        command_size = command_get_size (COMMAND_GLTEXSUBIMAGE2D);

        abstract_command = client_get_space_for_size (client, command_size + dest_size);
        abstract_command->type = COMMAND_GLTEXSUBIMAGE2D;
        abstract_command->size = command_size + dest_size;
        abstract_command->token = 0;

    }

    command_gltexsubimage2d_t *command =
        (command_gltexsubimage2d_t *) abstract_command;
    command->target = (GLenum) target;
    command->level = (GLint) level;
    command->xoffset = (GLint) xoffset;
    command->yoffset = (GLint) yoffset;
    command->width = (GLsizei) width;
    command->height = (GLsizei) height;
    command->format = (GLenum) format;
    command->type = (GLenum) type;

    if (!pixels)
        return;

    command->pixels = pixels ? (char*)command + command_size : NULL;

    copy_rect_to_buffer (pixels, command->pixels, format, type, height,
                         unpack_skip_pixels, unpack_skip_rows,
                         unpadded_row_size, padded_row_size, padded_row_size);
}

/* XXX: command_glshadersource_init: could be auto generated, however it will break the logic */
/* in the python code */
void
command_glshadersource_init (command_t *abstract_command,
                             GLuint shader,
                             GLsizei count,
                             const GLchar **string,
                             const GLint *length)
{
    /* FIXME: Add the situation where count is more than 1. */
    size_t command_size = 0;
    size_t string_size = 0;

    if (string) {
        size_t parameters_size = 0;
        size_t length_size = 0;
        client_t *client = client_get_thread_local ();

        command_size = command_get_size (COMMAND_GLSHADERSOURCE);

        if (length)
            length_size = sizeof (GLint);

        if (*string) {
            if (length)
                string_size = sizeof (GLchar *) + *length * sizeof (GLchar);
            else
                string_size = sizeof (GLchar *) + strlen (*string) + 1;
        } else
            string_size = sizeof (GLchar *);

        parameters_size =  string_size + length_size;

        abstract_command = client_get_space_for_size (client, command_size + parameters_size);
        abstract_command->type = COMMAND_GLSHADERSOURCE;
        abstract_command->size = command_size + parameters_size;
        abstract_command->token = 0;
    }

    command_glshadersource_t *command =
        (command_glshadersource_t *) abstract_command;
    command->shader = (GLuint) shader;
    command->count = (GLsizei) count;

    if (string) {
        command->string = (GLchar**)((char *)command + command_size);
        if (*string) {
            *command->string = (char *)command->string + sizeof (GLchar *);
            memcpy (*command->string, *string, string_size - sizeof (GLchar *));
            *((char*)(*command->string) + string_size) = 0;
        } else
            command->string = NULL;
    } else
        command->string = NULL;

    if (length) {
        command->length = (GLint*)((char *)command + command_size + string_size);
        memcpy (command->length, length, sizeof(GLint));
    } else
        command->length = NULL;
}

void
command_gltexparameteriv_init (command_t *abstract_command,
                               GLenum target,
                               GLenum pname,
                               const GLint *params)
{
    size_t params_size = sizeof (GLint);
    size_t command_size = 0;

    if (params) {
        client_t *client = client_get_thread_local ();

        command_size = command_get_size (COMMAND_GLTEXPARAMETERIV);

        abstract_command = client_get_space_for_size (client, command_size + params_size);
        abstract_command->type = COMMAND_GLTEXPARAMETERIV;
        abstract_command->size = command_size + params_size;
        abstract_command->token = 0;
    }

    command_gltexparameteriv_t *command =
        (command_gltexparameteriv_t *) abstract_command;
    command->target = target;
    command->pname = pname;

    if (!params)
        return;

    command->params = (GLint *)((char *)command + command_size);
    memcpy (command->params, params, params_size);
}

void
command_gltexparameterfv_init (command_t *abstract_command,
                               GLenum target,
                               GLenum pname,
                               const GLfloat *params)
{
    size_t params_size = sizeof (GLfloat);
    size_t command_size = 0;

    if (params) {
        client_t *client = client_get_thread_local ();

        command_size = command_get_size (COMMAND_GLTEXPARAMETERFV);

        abstract_command = client_get_space_for_size (client, command_size + params_size);
        abstract_command->type = COMMAND_GLTEXPARAMETERFV;
        abstract_command->size = command_size + params_size;
        abstract_command->token = 0;
    }

    command_gltexparameterfv_t *command =
        (command_gltexparameterfv_t *) abstract_command;
    command->target = target;
    command->pname = pname;

    if (!params)
        return;

    command->params = (GLfloat *)((char *)command + command_size);
    memcpy (command->params, params, params_size);
}
