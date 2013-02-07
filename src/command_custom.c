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

    uint32_t dest_size;
    uint32_t unpadded_row_size;
    uint32_t padded_row_size;
    uint32_t unpack_alignment = client_get_unpack_alignment ();
    uint32_t unpack_row_length = client_get_unpack_row_length ();
    uint32_t unpack_skip_pixels = client_get_unpack_skip_pixels ();
    uint32_t unpack_skip_rows = client_get_unpack_skip_rows ();

    if (! pixels)
        return;

    if (!compute_image_data_sizes (width, height, format, type,
                                   unpack_alignment, unpack_row_length, unpack_skip_rows, &dest_size,
                                   &unpadded_row_size, &padded_row_size)) {
        /* TODO: Set an error on the client-side.
         SetGLError(GL_INVALID_VALUE, "glTexImage2D", "dimension < 0"); */
        return;
    }

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

    command->pixels = malloc (dest_size);
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
    command_glshadersource_t *command =
        (command_glshadersource_t *) abstract_command;
    command->shader = (GLuint) shader;
    command->count = (GLsizei) count;
    command->string = ( char**) string;
    command->length = ( GLint*) length;
}

void
command_glshadersource_destroy_arguments (command_glshadersource_t *command)
{
    if (command->count <= 0)
        return;

    unsigned i = 0;
    for (i = 0; i < command->count; i++)
        free (command->string[i]);
    free (command->string);
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

void
command_glvertexattribpointer_destroy_arguments (command_glvertexattribpointer_t *command)

{
    /* This command is asynchronous, but we don't want to free the pointer
     * until after glDraw(Elements/Arrays). */
}

void
command_registersurface_init (command_t *command,
                              EGLDisplay display, EGLSurface surface,
                              thread_t client_id)
{
    command_registersurface_t *c = (command_registersurface_t *)command;
    c->dpy = display;
    c->surface = surface;
    c->client_id = client_id;
}
 
void
command_registercontext_init (command_t *command,
                              EGLDisplay display, EGLContext context,
                              thread_t client_id)
{
    command_registercontext_t *c = (command_registercontext_t *)command;
    c->dpy = display;
    c->context = context;
    c->client_id = client_id;
}

void
command_registerimage_init (command_t *command,
                              EGLDisplay display, EGLImageKHR image,
                              thread_t client_id)
{
    command_registerimage_t *c = (command_registerimage_t *)command;
    c->dpy = display;
    c->image = image;
    c->client_id = client_id;
}
