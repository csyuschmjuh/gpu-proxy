/* Parts of this file:
 * Copyright (c) 2012 The Chromium Authors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *    * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* This file implements gles2 functions. */
#include "gles2_api_private.h"

bool
_is_error_state (void)
{
    egl_state_t *egl_state;

    if (! client_active_egl_state_available ())
        return true;

    egl_state = client_get_active_egl_state ();

    if (! egl_state ||
        ! (egl_state->display == EGL_NO_DISPLAY ||
           egl_state->context == EGL_NO_CONTEXT) ||
           egl_state->active == false) {
        return true;
    }

    return false;
}

void glBufferData (GLenum target, GLsizeiptr size,
                   const GLvoid *data, GLenum usage)
{
    GLvoid *data_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in data */
    if (data && size > 0) {
        data_copy = (GLvoid *) malloc (sizeof (char) * size);
        memcpy (data_copy, data, sizeof (char) * size);
    }
}

void glBufferSubData (GLenum target, GLintptr offset,
                      GLsizeiptr size, const GLvoid *data)
{
    GLvoid *data_copy = NULL;
    
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in data */
    if (data && size > 0) {
        data_copy = (GLvoid *) malloc (sizeof (char) * size);
        memcpy (data_copy, data, sizeof (char) * size);
    }
}

GLenum glCheckFramebufferStatus (GLenum target)
{
    GLenum result = GL_INVALID_ENUM;
    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

/* total parameters 8 * sizeof (GLint) */
void glCompressedTexImage2D (GLenum target, GLint level,
                             GLenum internalformat,
                             GLsizei width, GLsizei height,
                             GLint border, GLsizei imageSize,
                             const GLvoid *data)
{
    GLvoid *data_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in data */
    if (data && imageSize > 0) {
        data_copy = (GLvoid *) malloc (sizeof (char) * imageSize);
        memcpy (data_copy, data, sizeof (char) * imageSize);
    }
}

/* total parameters 9 * sizeof (GLint) */
void glCompressedTexSubImage2D (GLenum target, GLint level,
                                GLint xoffset, GLint yoffset,
                                GLsizei width, GLsizei height,
                                GLenum format, GLsizei imageSize,
                                const GLvoid *data)
{
    GLvoid *data_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in data */
    if (data && imageSize > 0) {
        data_copy = (GLvoid *) malloc (sizeof (char) * imageSize);
        memcpy (data_copy, data, sizeof (char) * imageSize);
    }
}

/* This is a sync call */
GLuint glCreateProgram (void)
{
    GLuint result = 0;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

/* sync call */
GLuint glCreateShader (GLenum shaderType)
{
    GLuint result = 0;

    if (_is_error_state ())
        return result;

    command_t *command = client_get_space_for_command (COMMAND_GLCREATESHADER);
    command_glcreateshader_init (command,
                                 shaderType);
    command_buffer_write_command (command);

    unsigned int token = client_insert_token();
    command_buffer_wait_for_token (token);

    return ((command_glcreateshader_t *)command)->result;
}


void glDeleteBuffers (GLsizei n, const GLuint *buffers)
{
    GLuint *buffers_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in buffers */
    if (buffers && n > 0) {
        buffers_copy = (GLuint *) malloc (sizeof (GLuint) * n);
        memcpy ((void *)buffers, (const void *)buffers, sizeof (GLuint) * n);
    }
}

void glDeleteFramebuffers (GLsizei n, const GLuint *framebuffers)
{
    GLuint *framebuffers_copy = NULL;
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in framebuffers */
    if (framebuffers && n > 0) {
        framebuffers_copy = (GLuint *) malloc (sizeof (GLuint) * n);
        memcpy ((void *)framebuffers_copy, (const void *)framebuffers, 
                sizeof (GLuint) * n);
    }
}

void glDeleteRenderbuffers (GLsizei n, const GLuint *renderbuffers)
{
    GLuint *renderbuffers_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in renderbuffers */
    if (renderbuffers && n > 0) {
        renderbuffers_copy = (GLuint *) malloc (sizeof (GLuint) * n);
        memcpy ((void *)renderbuffers_copy, (const void *)renderbuffers, 
                sizeof (GLuint) * n);
    }
}

void glDeleteTextures (GLsizei n, const GLuint *textures)
{
    GLuint *textures_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in textures */
    if (textures && n > 0) {
        textures_copy = (GLuint *) malloc (sizeof (GLuint) * n);
        memcpy ((void *)textures_copy, (const void *)textures, 
                sizeof (GLuint) * n);
    }
}

static char *
_create_data_array (vertex_attrib_t *attrib, int count)
{
    int i;
    char *data = NULL;
    int size = 0;

    if (attrib->type == GL_BYTE || attrib->type == GL_UNSIGNED_BYTE)
        size = sizeof (char);
    else if (attrib->type == GL_SHORT || attrib->type == GL_UNSIGNED_SHORT)
        size = sizeof (short);
    else if (attrib->type == GL_FLOAT)
        size = sizeof (float);
    else if (attrib->type == GL_FIXED)
        size = sizeof (int);

    if (size == 0)
        return NULL;

    data = (char *)malloc (size * count * attrib->size);

    for (i = 0; i < count; i++)
        memcpy (data + i * attrib->size, attrib->pointer + attrib->stride * i, attrib->size * size);

    return data;
}

void glDrawArrays (GLenum mode, GLint first, GLsizei count)
{
    egl_state_t *egl_state;
    gles2_state_t *gles_state;
    char *data;
    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int i;

    if (_is_error_state ())
        return;

    egl_state = client_get_active_egl_state();
    gles_state = &egl_state->state;
    attrib_list = &gles_state->vertex_attribs;
    attribs = attrib_list->attribs;

    /* if vertex array has binding buffer */
    if (gles_state->vertex_array_binding || count <= 0) {
        /* post command buffer */
    }

    for (i = 0; i < attrib_list->count; i++) {
        if (! attribs[i].array_enabled) 
            continue;
        else if (! attribs[i].array_buffer_binding) {
            data = _create_data_array (&attribs[i], count);
            if (! data)
                continue;
            attribs[i].data = data;
        }
    }

    /* post command and no wait */
}

void glDrawElements (GLenum mode, GLsizei count, GLenum type,
                     const GLvoid *indices)
{
    GLvoid *indices_copy = NULL;
    egl_state_t *egl_state;
    gles2_state_t *gles_state;
    char *data;
    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int i;

    if (_is_error_state ())
        return;

    egl_state = client_get_active_egl_state();
    gles_state = &egl_state->state;
    attrib_list = &gles_state->vertex_attribs;
    attribs = attrib_list->attribs;

    /* if vertex array has binding buffer */
    if (gles_state->vertex_array_binding || count <= 0) {
        /* post command buffer */
    }

    for (i = 0; i < attrib_list->count; i++) {
        if (! attribs[i].array_enabled) 
            continue;
        else if (! attribs[i].array_buffer_binding) {
            data = _create_data_array (&attribs[i], count);
            if (! data)
                continue;
            attribs[i].data = data;
        }
    }

    /* XXX: copy indices.  According to spec, if type is neither 
     * GL_UNSIGNED_BYTE nor GL_UNSIGNED_SHORT, GL_INVALID_ENUM error
     * can be generated, we only support these two types, other types
     * will generate error even the underlying driver supports other
     * types.
     */
    if (egl_state->state.element_array_buffer_binding == 0) {
        if (indices && count > 0 ) {
            if (type == GL_UNSIGNED_BYTE) {
                indices_copy = (GLvoid *) malloc (sizeof (char) * count);
                memcpy ((void *)indices_copy, (const void *)indices, 
                        sizeof (char) * count);
            }
            else if (type == GL_UNSIGNED_SHORT) {
                indices_copy = (GLvoid *) malloc (sizeof (short) * count);
                memcpy ((void *)indices_copy, (const void *)indices, 
                        sizeof (short) * count);
            }
        }
    }

    /* post command and no wait */
}

void glGenBuffers (GLsizei n, GLuint *buffers)
{
    name_handler_t * name_handler;

    if (_is_error_state ())
        return;

    name_handler = client_get_name_handler();
    buffers = name_handler_alloc_names (name_handler, RESOURCE_GEN_BUFFERS, n);

    /* FIXME: post command with generated client buffers. */
}

void glGenFramebuffers (GLsizei n, GLuint *framebuffers)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

void glGenRenderbuffers (GLsizei n, GLuint *renderbuffers)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

void glGenTextures (GLsizei n, GLuint *textures)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

void glGetBooleanv (GLenum pname, GLboolean *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

void glGetFloatv (GLenum pname, GLfloat *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

void glGetIntegerv (GLenum pname, GLint *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

/* total parameters 7 * sizeof (GLint) */
void glGetActiveAttrib (GLuint program, GLuint index,
                        GLsizei bufsize, GLsizei *length,
                        GLint *size, GLenum *type, GLchar *name)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

/* total parameters 7 * sizeof (GLint) */
void glGetActiveUniform (GLuint program, GLuint index, GLsizei bufsize,
                         GLsizei *length, GLint *size, GLenum *type,
                         GLchar *name)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

void glGetAttachedShaders (GLuint program, GLsizei maxCount,
                           GLsizei *count, GLuint *shaders)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

GLint glGetAttribLocation (GLuint program, const GLchar *name)
{
    GLint result = -1;
    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

void glGetBufferParameteriv (GLenum target, GLenum value, GLint *data)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

GLenum glGetError (void)
{
    GLenum error = GL_INVALID_OPERATION;
    
    if (_is_error_state ())
        return error;

    /* XXX: post command and wait */
    return error;
}

void glGetFramebufferAttachmentParameteriv (GLenum target,
                                            GLenum attachment,
                                            GLenum pname,
                                            GLint *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glGetProgramInfoLog (GLuint program, GLsizei maxLength,
                          GLsizei *length, GLchar *infoLog)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glGetProgramiv (GLuint program, GLenum pname, GLint *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glGetRenderbufferParameteriv (GLenum target,
                                   GLenum pname,
                                   GLint *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glGetShaderInfoLog (GLuint program, GLsizei maxLength,
                         GLsizei *length, GLchar *infoLog)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glGetShaderPrecisionFormat (GLenum shaderType, GLenum precisionType,
                                 GLint *range, GLint *precision)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glGetShaderSource (GLuint shader, GLsizei bufSize, GLsizei *length,
                        GLchar *source)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glGetShaderiv (GLuint shader, GLenum pname, GLint *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

const GLubyte *glGetString (GLenum name)
{
    GLubyte *result = NULL;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

void glGetTexParameteriv (GLenum target, GLenum pname, GLint *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glGetTexParameterfv (GLenum target, GLenum pname, GLfloat *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glGetUniformiv (GLuint program, GLint location, GLint *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glGetUniformfv (GLuint program, GLint location, GLfloat *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GLint glGetUniformLocation (GLuint program, const GLchar *name)
{
    GLint result = -1;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

void glGetVertexAttribfv (GLuint index, GLenum pname, GLfloat *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glGetVertexAttribiv (GLuint index, GLenum pname, GLint *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glGetVertexAttribPointerv (GLuint index, GLenum pname, GLvoid **pointer)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GLboolean glIsBuffer (GLuint buffer)
{
    GLboolean result = GL_FALSE;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

GLboolean glIsEnabled (GLenum cap)
{
    GLboolean result = GL_FALSE;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

GLboolean glIsFramebuffer (GLuint framebuffer)
{
    GLboolean result = GL_FALSE;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

GLboolean glIsProgram (GLuint program)
{
    GLboolean result = GL_FALSE;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

GLboolean glIsRenderbuffer (GLuint renderbuffer)
{
    GLboolean result = GL_FALSE;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

GLboolean glIsShader (GLuint shader)
{
    GLboolean result = GL_FALSE;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

GLboolean glIsTexture (GLuint texture)
{
    GLboolean result = GL_FALSE;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

void glPixelStorei (GLenum pname, GLint param)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

/* total parameter 7 * sizeof (GLint) */
void glReadPixels (GLint x, GLint y,
                   GLsizei width, GLsizei height,
                   GLenum format, GLenum type, GLvoid *data)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glShaderBinary (GLsizei n, const GLuint *shaders,
                     GLenum binaryformat, const void *binary,
                     GLsizei length)
{
    void *binary_copy = NULL;
    GLuint *shaders_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: we must copy data in shaders, binary */
    if (n > 0 && shaders) {
        shaders_copy = (GLuint *)malloc (sizeof (GLuint) * n);
        memcpy ((void *)shaders_copy, (const void *)shaders, 
                sizeof (GLuint) * n);
    }
    if (length > 0 && binary) {
        binary_copy = (void *) malloc (sizeof (char) * length);
        memcpy (binary_copy, binary, sizeof (char) * length);
    }

    return;
}

void glShaderSource (GLuint shader, GLsizei count,
                     const GLchar **string, const GLint *length)
{
    GLchar **string_copy = NULL;
    GLint *length_copy = NULL;
    int i;
    int str_len;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in string and length */
    if (count > 0 && string) {
        string_copy = (GLchar **) malloc (sizeof (GLchar *) * count );
        if (length != NULL) {
            length_copy = (GLint *)malloc (sizeof (GLint) * count);
        
            memcpy ((void *)length_copy, (const void *)length,
                    sizeof (GLint) * count);
        }

        for (i = 0; i < count; i++) {
            if (length) {
                if (length[i] > 0) {
                    string_copy[i] = (GLchar *)malloc (sizeof (GLchar) * length[i]);
                    memcpy ((void *)string_copy[i], 
                            (const void *)string[i],
                            sizeof (GLchar) * length[i]);
                }
            } else {
                str_len = strlen (string[i]);
                string_copy[i] = (GLchar *)malloc (sizeof (GLchar) * (str_len + 1));
                memcpy ((void *)string_copy[i],
                        (const void *)string[i],
                        sizeof (GLchar) * str_len);
                string_copy[i][str_len] = 0;
            }
        }
    }

    return;
}

static int
_gl_get_data_width (GLsizei width,
                    GLenum format,
                    GLenum type)
{
    int padding = 0;
    int mod = 0;
    int total_width = 0;
    int unpack_alignment = 4;

    if (type == GL_UNSIGNED_BYTE) {
        if (format == GL_RGB) {
            mod = width * 3 % unpack_alignment;
            padding = mod == 0 ? 0 : unpack_alignment - mod;
            total_width = width * 3 + padding;
        }
        else if (format == GL_RGBA) {
            mod = width * 4 % unpack_alignment;
            padding = mod == 0 ? 0 : unpack_alignment - mod;
            total_width = width * 4 + padding;
        }
        else if (format == GL_ALPHA || format == GL_LUMINANCE) {
            mod = width  % unpack_alignment;
            padding = mod == 0 ? 0 : unpack_alignment - mod;
            total_width = width + padding;
        }
        else if (format == GL_LUMINANCE_ALPHA) {
            mod = width * 2  % unpack_alignment;
            padding = mod == 0 ? 0 : unpack_alignment - mod;
            total_width = width * 2 + padding;
        }
    }
    else {
        mod = width * 2 % unpack_alignment;
        padding = mod == 0 ? 0 : unpack_alignment - mod;
        total_width = width * 2 + padding;
    }
    
    return total_width;
}

/* total parameters 9 * sizeof (GLint) */
void glTexImage2D (GLenum target,
                   GLint level,
                   GLint internalformat,
                   GLsizei width,
                   GLsizei height,
                   GLint border,
                   GLenum format,
                   GLenum type,
                   const GLvoid *source_data)
{
    if (_is_error_state ())
        return;

    if (level < 0 || height < 0 || width < 0) {
        /* TODO: Set an error on the client-side. 
           SetGLError(GL_INVALID_VALUE, "glTexImage2D", "dimension < 0"); */
        return;
    }

    command_t *command = client_get_space_for_command (COMMAND_GLTEXIMAGE2D);
    if (!source_data) {
        command_teximage2d_init (command, target, level, internalformat, width,
                                 height, border, format, type, NULL);
        client_write_command (command);
        return;
    }

    int unpack_alignment = 4;

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

    char* dest_data = malloc (dest_size);
    copy_rect_to_buffer (source_data, dest_data, height, unpadded_row_size,
                         padded_row_size, false /* flip y */, padded_row_size);

    command_teximage2d_init (command, target, level, internalformat, width,
                             height, border, format, type, dest_data);
    client_write_command (command);
}

void glTexSubImage2D (GLenum target,
                      GLint level,
                      GLint xoffset,
                      GLint yoffset,
                      GLsizei width,
                      GLsizei height,
                      GLenum format,
                      GLenum type,
                      const GLvoid *source_data)
{
    if (_is_error_state ())
        return;

    if (level < 0 || height < 0 || width < 0) {
        // XXX: Set a GL error on the client side here.
        // SetGLError(GL_INVALID_VALUE, "glTexSubImage2D", "dimension < 0");
        return;
    }

    if (height == 0 || width == 0)
        return;

    // TODO: This should be read from the current GL client-side state.
    static const int unpack_alignment = 4;

    uint32_t dest_size;
    uint32_t unpadded_row_size;
    uint32_t padded_row_size;
    if (! compute_image_data_sizes (width, height, format,
                                    type, unpack_alignment,
                                    &dest_size, &unpadded_row_size,
                                    &padded_row_size)) {
        // XXX: Set a GL error on the client side here.
        // SetGLError(GL_INVALID_VALUE, "glTexSubImage2D", "size to large");
        return;
    }

    char* dest_data = malloc (dest_size);
    copy_rect_to_buffer (source_data, dest_data, height, unpadded_row_size,
                         padded_row_size, false /* flip y */, padded_row_size);


    command_t *command = client_get_space_for_command (COMMAND_GLTEXSUBIMAGE2D);
    command_texsubimage2d_init (command, target, level, xoffset, yoffset,
                                width, height, format, type, dest_data);

    client_write_command ( command);

}

void glUniform1fv (GLint location, GLsizei count, const GLfloat *value)
{
    GLfloat *value_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in value */
    if (count > 0 && value) {
        value_copy = (GLfloat *) malloc (sizeof (GLfloat) * count);
        memcpy ((void *)value_copy, (const void *)value, sizeof (GLfloat) * count);
    }
    return;
}

void glUniform2fv (GLint location, GLsizei count, const GLfloat *value)
{
    GLfloat *value_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in value */
    if (count > 0 && value) {
        value_copy = (GLfloat *) malloc (sizeof (GLfloat) * count);
        memcpy ((void *)value_copy, (const void *)value, sizeof (GLfloat) * count);
    }
    return;
}

void glUniform3fv (GLint location, GLsizei count, const GLfloat *value)
{
    GLfloat *value_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in value */
    if (count > 0 && value) {
        value_copy = (GLfloat *) malloc (sizeof (GLfloat) * count);
        memcpy ((void *)value_copy, (const void *)value, sizeof (GLfloat) * count);
    }
    return;
}

void glUniform4fv (GLint location, GLsizei count, const GLfloat *value)
{
    GLfloat *value_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in value */
    if (count > 0 && value) {
        value_copy = (GLfloat *) malloc (sizeof (GLfloat) * count);
        memcpy ((void *)value_copy, (const void *)value, sizeof (GLfloat) * count);
    }
    return;
}

void glUniform1iv (GLint location, GLsizei count, const GLint *value)
{
    GLint *value_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in value */
    if (count > 0 && value) {
        value_copy = (GLint *) malloc (sizeof (GLint) * count);
        memcpy ((void *)value_copy, (const void *)value, sizeof (GLint) * count);
    }
    return;
}

void glUniform2iv (GLint location, GLsizei count, const GLint *value)
{
    GLint *value_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in value */
    if (count > 0 && value) {
        value_copy = (GLint *) malloc (sizeof (GLint) * count);
        memcpy ((void *)value_copy, (const void *)value, sizeof (GLint) * count);
    }
    return;
}

void glUniform3iv (GLint location, GLsizei count, const GLint *value)
{
    GLint *value_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in value */
    if (count > 0 && value) {
        value_copy = (GLint *) malloc (sizeof (GLint) * count);
        memcpy ((void *)value_copy, (const void *)value, sizeof (GLint) * count);
    }
    return;
}

void glUniform4iv (GLint location, GLsizei count, const GLint *value)
{
    GLint *value_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in value */
    if (count > 0 && value) {
        value_copy = (GLint *) malloc (sizeof (GLint) * count);
        memcpy ((void *)value_copy, (const void *)value, sizeof (GLint) * count);
    }
    return;
}

void glVertexAttrib1fv (GLuint index, const GLfloat *v)
{
    GLfloat *v_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in v */
    if (v) {
        v_copy = (GLfloat *) malloc (sizeof (GLfloat));
        v_copy[0] = v[0];
    }
    return;
}

void glVertexAttrib2fv (GLuint index, const GLfloat *v)
{
    GLfloat *v_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in v */
    if (v) {
        v_copy = (GLfloat *) malloc (sizeof (GLfloat) * 2);
        memcpy ((void *)v_copy, (const void *)v, sizeof (GLfloat) * 2);
    }
    return;
}

void glVertexAttrib3fv (GLuint index, const GLfloat *v)
{
    GLfloat *v_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in v */
    if (v) {
        v_copy = (GLfloat *) malloc (sizeof (GLfloat) * 3);
        memcpy ((void *)v_copy, (const void *)v, sizeof (GLfloat) * 3);
    }
    return;
}

void glVertexAttrib4fv (GLuint index, const GLfloat *v)
{
    GLfloat *v_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in v */
    if (v) {
        v_copy = (GLfloat *) malloc (sizeof (GLfloat) * 4);
        memcpy ((void *)v_copy, (const void *)v, sizeof (GLfloat) * 3);
    }
    return;
}

/* total parameters 6 * sizeof (GLint) */
void glVertexAttribPointer (GLuint index, GLint size, GLenum type,
                            GLboolean normalized, GLsizei stride,
                            const GLvoid *pointer)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: no need to copy data in pointer */
    return;
}

/* end of GLES2 core profile */

#ifdef GL_OES_EGL_image
GL_APICALL void GL_APIENTRY
glEGLImageTargetTexture2DOES (GLenum target, GLeglImageOES image)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

GL_APICALL void GL_APIENTRY
glEGLImageTargetRenderbufferStorageOES (GLenum target, GLeglImageOES image)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}
#endif

#ifdef GL_OES_get_program_binary
/* total parameters size 5 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glGetProgramBinaryOES (GLuint program, GLsizei bufSize, GLsizei *length,
                       GLenum *binaryFormat, GLvoid *binary)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glProgramBinaryOES (GLuint program, GLenum binaryFormat,
                    const GLvoid *binary, GLint length)
{
    GLvoid *binary_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in binary */
    if (length > 0 && binary) {
        binary_copy = (void *) malloc (sizeof (char) * length);
        memcpy (binary_copy, binary, sizeof (char) * length);
    }
    return;
}
#endif

#ifdef GL_OES_mapbuffer
GL_APICALL void* GL_APIENTRY glMapBufferOES (GLenum target, GLenum access)
{
    void *result = NULL;
    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

GL_APICALL GLboolean GL_APIENTRY
glUnmapBufferOES (GLenum target)
{
    GLboolean result = GL_FALSE;
    
    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

GL_APICALL void GL_APIENTRY
glGetBufferPointervOES (GLenum target, GLenum pname, GLvoid **params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}
#endif

#ifdef GL_OES_texture_3D
/* total parameters 10 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glTexImage3DOES (GLenum target, GLint level, GLenum internalformat,
                 GLsizei width, GLsizei height, GLsizei depth,
                 GLint border, GLenum format, GLenum type,
                 const GLvoid *pixels)
{
    GLvoid *pixels_copy = NULL;
    int pixels_width = 0;
    int image_size;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: Is it worth make copy data and make it async call?,
     * I am not clear whether we need padding for depth. 
     * no mentioning in the spec: 
     * http://www.khronos.org/registry/gles/extensions/OES/OES_texture_3D.txt
     */
    if (pixels && width > 0 && height > 0 && depth > 0 ) {
        pixels_width = _gl_get_data_width (width, format, type);
        image_size = pixels_width * height * depth;

        if (image_size > 0) {
            pixels_copy = (GLvoid *)malloc (sizeof (char) * image_size);
            memcpy ((void *)pixels_copy, (const void *)pixels, sizeof (char) * image_size);
        }
    } 
            
    return;
}

/* total parameters 11 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glTexSubImage3DOES (GLenum target, GLint level,
                    GLint xoffset, GLint yoffset, GLint zoffset,
                    GLsizei width, GLsizei height, GLsizei depth,
                    GLenum format, GLenum type, const GLvoid *data)
{
    GLvoid *data_copy = NULL;
    int data_width = 0;
    int image_size;
    
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: Is it worth make copy data and make it async call?
     * I am not clear whether we need padding for depth. 
     * no mentioning in the spec: 
     * http://www.khronos.org/registry/gles/extensions/OES/OES_texture_3D.txt
     */
    if (data && width > 0 && height > 0 && depth) {
        data_width = _gl_get_data_width (width, format, type);
        image_size = data_width * height * depth;

        if (image_size > 0) {
            data_copy = (GLvoid *)malloc (sizeof (char) * image_size);
            memcpy ((void *)data_copy, (const void *)data, sizeof (char) * image_size);
        }
    } 
    return;
}

/* total parameters 9 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glCopyTexSubImage3DOES (GLenum target, GLint level,
                        GLint xoffset, GLint yoffset, GLint zoffset,
                        GLint x, GLint y,
                        GLint width, GLint height)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

GL_APICALL void GL_APIENTRY
glCompressedTexImage3DOES (GLenum target, GLint level,
                           GLenum internalformat,
                           GLsizei width, GLsizei height, GLsizei depth,
                           GLint border, GLsizei imageSize,
                           const GLvoid *data)
{
    GLvoid *data_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in data */
    if (data && imageSize > 0) {
        data_copy = (GLvoid *) malloc (sizeof (char) * imageSize);
        memcpy (data_copy, data, sizeof (char) * imageSize);
    }
}

/* total parameters 11 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glCompressedTexSubImage3DOES (GLenum target, GLint level,
                              GLint xoffset, GLint yoffset, GLint zoffset,
                              GLsizei width, GLsizei height, GLsizei depth,
                              GLenum format, GLsizei imageSize,
                              const GLvoid *data)
{
    GLvoid *data_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in data */
    if (data && imageSize > 0) {
        data_copy = (GLvoid *) malloc (sizeof (char) * imageSize);
        memcpy (data_copy, data, sizeof (char) * imageSize);
    }
        
    return;
}

/* total parameters 6 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glFramebufferTexture3DOES (GLenum target, GLenum attachment,
                           GLenum textarget, GLuint texture,
                           GLint level, GLint zoffset)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}
#endif

#ifdef GL_OES_vertex_array_object
GL_APICALL void GL_APIENTRY
glBindVertexArrayOES (GLuint array)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

GL_APICALL void GL_APIENTRY
glDeleteVertexArraysOES (GLsizei n, const GLuint *arrays)
{
    GLuint *arrays_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in arrays */
    if (n > 0 && arrays) {
        arrays_copy = (GLuint *) malloc (sizeof (GLuint) * n);
        memcpy ((void *)arrays_copy, (const void *)arrays,
                sizeof (GLuint) * n);
    }
    return;
}

GL_APICALL void GL_APIENTRY
glGenVertexArraysOES (GLsizei n, GLuint *arrays)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL GLboolean GL_APIENTRY
glIsVertexArrayOES (GLuint array)
{
    GLboolean result = GL_FALSE;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}
#endif

#ifdef GL_AMD_performance_monitor
GL_APICALL void GL_APIENTRY
glGetPerfMonitorGroupsAMD (GLint *numGroups, GLsizei groupSize, 
                           GLuint *groups)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

/* total parameters 5 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glGetPerfMonitorCountersAMD (GLuint group, GLint *numCounters, 
                             GLint *maxActiveCounters, GLsizei counterSize,
                             GLuint *counters)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glGetPerfMonitorGroupStringAMD (GLuint group, GLsizei bufSize, 
                                GLsizei *length, GLchar *groupString)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

/* total parameters 5 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glGetPerfMonitorCounterStringAMD (GLuint group, GLuint counter, 
                                  GLsizei bufSize, 
                                  GLsizei *length, GLchar *counterString)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glGetPerfMonitorCounterInfoAMD (GLuint group, GLuint counter, 
                                GLenum pname, GLvoid *data)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glGenPerfMonitorsAMD (GLsizei n, GLuint *monitors) 
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glDeletePerfMonitorsAMD (GLsizei n, GLuint *monitors) 
{
    GLuint *monitors_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in monitors */
    if (n > 0 && monitors) {
        monitors_copy = (GLuint *) malloc (sizeof (GLuint) * n);
        memcpy ((void *)monitors_copy, (const void *)monitors,
                sizeof (GLuint) * n);
    }
    return;
}

/* total parameter 5 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glSelectPerfMonitorCountersAMD (GLuint monitor, GLboolean enable,
                                GLuint group, GLint numCounters,
                                GLuint *countersList) 
{
    GLuint *counters_list_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in counterisList */
    if (numCounters > 0 && countersList) {
        counters_list_copy = (GLuint *) malloc (sizeof (GLuint) * numCounters);
        memcpy ((void *)counters_list_copy, (const void *)countersList,
                sizeof (GLuint) * numCounters);
    }
    return;
}

GL_APICALL void GL_APIENTRY
glBeginPerfMonitorAMD (GLuint monitor)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

GL_APICALL void GL_APIENTRY
glEndPerfMonitorAMD (GLuint monitor)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

/* total parameters 5 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glGetPerfMonitorCounterDataAMD (GLuint monitor, GLenum pname,
                                GLsizei dataSize, GLuint *data,
                                GLint *bytesWritten)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}
#endif

#ifdef GL_ANGLE_framebuffer_blit
/* total parameters 10 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glBlitFramebufferANGLE (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                        GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                        GLbitfield mask, GLenum filter)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}
#endif

#ifdef GL_ANGLE_framebuffer_multisample
/* total parameters 5 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glRenderbufferStorageMultisampleANGLE (GLenum target, GLsizei samples, 
                                       GLenum internalformat, 
                                       GLsizei width, GLsizei height)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}
#endif

#ifdef GL_APPLE_framebuffer_multisample
/* total parameters 5 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glRenderbufferStorageMultisampleAPPLE (GLenum target, GLsizei samples, 
                                       GLenum internalformat, 
                                       GLsizei width, GLsizei height)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

GL_APICALL void GL_APIENTRY
glResolveMultisampleFramebufferAPPLE (void) 
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}
#endif

#ifdef GL_EXT_discard_framebuffer
GL_APICALL void GL_APIENTRY
glDiscardFramebufferEXT (GLenum target, GLsizei numAttachments,
                         const GLenum *attachments)
{
    GLenum *attachments_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in attachments */
    if (numAttachments > 0 && attachments) {
        attachments_copy = (GLenum *)malloc (sizeof (GLenum) * numAttachments);
        memcpy ((void *)attachments_copy, (const void *)attachments,
                sizeof (GLenum) * numAttachments);
    }
}
#endif

#ifdef GL_EXT_multi_draw_arrays
GL_APICALL void GL_APIENTRY
glMultiDrawArraysEXT (GLenum mode, const GLint *first, 
                      const GLsizei *count, GLsizei primcount)
{
    /* not implemented */
}

GL_APICALL void GL_APIENTRY
glMultiDrawElementsEXT (GLenum mode, const GLsizei *count, GLenum type,
                        const GLvoid **indices, GLsizei primcount)
{
    /* not implemented */
}
#endif

#ifdef GL_EXT_multisampled_render_to_texture
/* total parameters 5 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glRenderbufferStorageMultisampleEXT (GLenum target, GLsizei samples,
                                     GLenum internalformat,
                                     GLsizei width, GLsizei height)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

/* total parameters 6 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glFramebufferTexture2DMultisampleEXT (GLenum target, GLenum attachment,
                                      GLenum textarget, GLuint texture,
                                      GLint level, GLsizei samples)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}
#endif

#ifdef GL_IMG_multisampled_render_to_texture
/* total parameters 5 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glRenderbufferStorageMultisampleIMG (GLenum target, GLsizei samples,
                                     GLenum internalformat,
                                     GLsizei width, GLsizei height)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

/* total parameter 6 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glFramebufferTexture2DMultisampleIMG (GLenum target, GLenum attachment,
                                      GLenum textarget, GLuint texture,
                                      GLint level, GLsizei samples)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}
#endif

#ifdef GL_NV_fence
GL_APICALL void GL_APIENTRY
glDeleteFencesNV (GLsizei n, const GLuint *fences)
{
    GLuint *fences_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in fences */
    if (fences && n > 0) {
        fences_copy = (GLuint *)malloc (sizeof (GLuint) * n);
        memcpy ((void *)fences_copy, (const void *)fences, 
                sizeof (GLuint) * n);
    }
}

GL_APICALL void GL_APIENTRY
glGenFencesNV (GLsizei n, GLuint *fences)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

GL_APICALL GLboolean GL_APIENTRY
glIsFenceNV (GLuint fence)
{
    GLboolean result = GL_FALSE;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

GL_APICALL GLboolean GL_APIENTRY
glTestFenceNV (GLuint fence)
{
    GLboolean result = GL_FALSE;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

GL_APICALL void GL_APIENTRY
glGetFenceivNV (GLuint fence, GLenum pname, int *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glFinishFenceivNV (GLuint fence)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

GL_APICALL void GL_APIENTRY
glSetFenceNV (GLuint fence, GLenum condition)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}
#endif

#ifdef GL_NV_coverage_sample
GL_APICALL void GL_APIENTRY
glCoverageMaskNV (GLboolean mask)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

GL_APICALL void GL_APIENTRY
glCoverageOperationNV (GLenum operation)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}
#endif

#ifdef GL_QCOM_driver_control
GL_APICALL void GL_APIENTRY
glGetDriverControlsQCOM (GLint *num, GLsizei size, GLuint *driverControls)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glGetDriverControlStringQCOM (GLuint driverControl, GLsizei bufSize,
                              GLsizei *length, GLchar *driverControlString)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glEnableDriverControlQCOM (GLuint driverControl)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

GL_APICALL void GL_APIENTRY
glDisableDriverControlQCOM (GLuint driverControl)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}
#endif

#ifdef GL_QCOM_extended_get
GL_APICALL void GL_APIENTRY
glExtGetTexturesQCOM (GLuint *textures, GLint maxTextures, 
                      GLint *numTextures)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glExtGetBuffersQCOM (GLuint *buffers, GLint maxBuffers, GLint *numBuffers) 
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glExtGetRenderbuffersQCOM (GLuint *renderbuffers, GLint maxRenderbuffers,
                           GLint *numRenderbuffers)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glExtGetFramebuffersQCOM (GLuint *framebuffers, GLint maxFramebuffers,
                          GLint *numFramebuffers)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glExtGetTexLevelParameterivQCOM (GLuint texture, GLenum face, GLint level,
                                 GLenum pname, GLint *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glExtTexObjectStateOverrideiQCOM (GLenum target, GLenum pname, GLint param)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

/* total parameter 11 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glExtGetTexSubImageQCOM (GLenum target, GLint level,
                         GLint xoffset, GLint yoffset, GLint zoffset,
                         GLsizei width, GLsizei height, GLsizei depth,
                         GLenum format, GLenum type, GLvoid *texels)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glExtGetBufferPointervQCOM (GLenum target, GLvoid **params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}
#endif

#ifdef GL_QCOM_extended_get2
GL_APICALL void GL_APIENTRY
glExtGetShadersQCOM (GLuint *shaders, GLint maxShaders, GLint *numShaders)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glExtGetProgramsQCOM (GLuint *programs, GLint maxPrograms,
                      GLint *numPrograms)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL GLboolean GL_APIENTRY
glExtIsProgramBinaryQCOM (GLuint program)
{
    GLboolean result = false;
    
    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

GL_APICALL void GL_APIENTRY
glExtGetProgramBinarySourceQCOM (GLuint program, GLenum shadertype,
                                 GLchar *source, GLint *length)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}
#endif

#ifdef GL_QCOM_tiled_rendering
/* total parameter 5 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glStartTilingQCOM (GLuint x, GLuint y, GLuint width, GLuint height,
                   GLbitfield preserveMask)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

GL_APICALL void GL_APIENTRY
glEndTilingQCOM (GLbitfield preserveMask)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}
#endif
