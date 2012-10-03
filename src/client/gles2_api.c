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
    return !client_get_thread_local();
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

    /* We need to flush the command buffer because some previous
     * glVertexAttribPointer() might have been executed yet.*/
    int token = client_insert_token ();
    client_wait_for_token (token);

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

    /* We need to flush the command buffer because some previous
     * glVertexAttribPointer() might have been executed yet.*/
    int token = client_insert_token ();
    client_wait_for_token (token);

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

void glPixelStorei (GLenum pname, GLint param)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
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

void
glTexParameteriv (GLenum target,
                  GLenum pname,
                  const GLint *params)
{
}

void
glTexParameterfv (GLenum target,
                  GLenum pname,
                  const GLfloat *params)
{
}
