#include "command.h"
#include <string.h>
#include "gles2_utils.h"

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

    static int unpack_alignment = 4;
    uint32_t dest_size;
    uint32_t unpadded_row_size;
    uint32_t padded_row_size;
    if (!compute_image_data_sizes (width, height, format, type,
                                   unpack_alignment, &dest_size,
                                   &unpadded_row_size, &padded_row_size)) {
        /* TODO: Set an error on the client-side.
         SetGLError(GL_INVALID_VALUE, "glTexImage2D", "dimension < 0"); */
        return;
    }

    command->pixels = malloc (dest_size);
    copy_rect_to_buffer (pixels, command->pixels, height, unpadded_row_size,
                         padded_row_size, false /* flip y */, padded_row_size);
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

    static int unpack_alignment = 4;
    uint32_t dest_size;
    uint32_t unpadded_row_size;
    uint32_t padded_row_size;
    if (! compute_image_data_sizes (width, height, format,
                                    type, unpack_alignment,
                                    &dest_size, &unpadded_row_size,
                                    &padded_row_size)) {
        /* XXX: Set a GL error on the client side here. */
        /* SetGLError(GL_INVALID_VALUE, "glTexSubImage2D", "size to large"); */
        return;
    }

    command->pixels = malloc (dest_size);
    copy_rect_to_buffer (pixels, command->pixels, height, unpadded_row_size,
                         padded_row_size, false /* flip y */, padded_row_size);
}

void
command_glshadersource_init (command_t *abstract_command,
                             GLuint shader,
                             GLsizei count,
                             const GLchar **string,
                             const GLint *length)
{
    command_glshadersource_t *command =
        (command_glshadersource_t *) abstract_command;
    command->shader = shader;
    command->count = count;
    command->string = NULL;
    command->length = NULL;

    if (count <= 0 || ! string)
        return;

    if (length != NULL) {
        size_t lengths_size = sizeof (GLint *) * count;
        command->length = malloc (lengths_size);
        memcpy (command->length, length, lengths_size);
    }

    command->string = malloc (sizeof (GLchar *) * count);

    unsigned i = 0;
    for (i = 0; i < count; i++) {
        size_t string_length = length ? length[i] : strlen (string[i]);
        command->string[i] = malloc (string_length + 1);
        memcpy (command->string[i], string[i], string_length);
        command->string[i][string_length] = 0;
    }
}

void
command_gltexparameteriv_init (command_t *abstract_command,
                               GLenum target,
                               GLenum pname,
                               const GLint *params)
{
    command_gltexparameteriv_t *command =
        (command_gltexparameteriv_t *) abstract_command;
    command->target = target;
    command->pname = pname;

    size_t params_size = sizeof (GLint);
    command->params = malloc (params_size);
    memcpy (command->params, params, params_size);
}

void
command_gltexparameterfv_init (command_t *abstract_command,
                               GLenum target,
                               GLenum pname,
                               const GLfloat *params)
{
    command_gltexparameterfv_t *command =
        (command_gltexparameterfv_t *) abstract_command;
    command->target = target;
    command->pname = pname;

    size_t params_size = sizeof (GLfloat);
    command->params = malloc (params_size);
    memcpy (command->params, params, params_size);
}
