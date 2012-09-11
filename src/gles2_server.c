/* This file implements the server thread side of GLES2 functions.  Thses
 * functions are called by the command buffer.
 *
 * It references to the server thread side of active_state.
 * if active_state is NULL or there is no symbol for the corresponding
 * gl functions, the cached error state is set to GL_INVALID_OPERATION
 * if the cached error has not been set to one of the errors.
 */
#include "config.h"

#ifndef HAS_GLES2
#include <GLES/gl.h>
#include <GLES/glext.h>
#else
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif
#include <string.h>
#include <stdlib.h>

/* XXX: we should move it to the srv */
#include "dispatch_private.h"
#include "types_private.h"
#include "egl_states.h"

/* server local variable, referenced from egl_server.c */
extern __thread link_list_t        *active_state;

/* global variable for all server threads, referenced from 
 * egl_server_helper.c */
extern dispatch_t         dispatch;

static inline bool
_gl_is_valid_func (void *func)
{
    egl_state_t *egl_state;

    if (func)
        return true;

    if (active_state) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (egl_state->active == true &&
            egl_state->state.error == GL_NO_ERROR) {
            egl_state->state.error = GL_INVALID_OPERATION;
            return false;
        }
    }
    return false;
}

static inline bool
_gl_is_valid_context (void)
{
    egl_state_t *egl_state;

    bool is_valid = false;

    if (active_state) {
        egl_state = (egl_state_t *)active_state->data;
        if (egl_state->active)
            return true;
    }
    return is_valid;
}

static inline void
_gl_set_error (GLenum error)
{
    egl_state_t *egl_state;

    if (active_state) {
        egl_state = (egl_state_t *) active_state->data;
 
        if (egl_state->active && egl_state->state.error == GL_NO_ERROR)
            egl_state->state.error = error;
    }
}

/* GLES2 core profile API */
static void _gl_active_texture (GLenum texture)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.ActiveTexture) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (egl_state->state.active_texture == texture)
            return;
        else if (texture > GL_TEXTURE31 || texture < GL_TEXTURE0) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }
        else {
            dispatch.ActiveTexture (texture);
            /* FIXME: this maybe not right because this texture may be 
             * invalid object, we save here to save time in glGetError() 
             */
            egl_state->state.active_texture = texture;
        }
    }
}

static void _gl_attach_shader (GLuint program, GLuint shader)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.AttachShader) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.AttachShader (program, shader);

        egl_state->state.need_get_error = true;
    }
}

static void _gl_bind_attrib_location (GLuint program, GLuint index, const GLchar *name)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.BindAttribLocation) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.BindAttribLocation (program, index, name);
        egl_state->state.need_get_error = true;
    }
    if (name)
        free ((char *)name);
}

static void _gl_bind_buffer (GLenum target, GLuint buffer)
{
    egl_state_t *egl_state;
    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int count;
    int i;
    
    if (_gl_is_valid_func (dispatch.BindBuffer) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        attrib_list = &egl_state->state.vertex_attribs;
        vertex_attrib_t *attribs = attrib_list->attribs;
        count = attrib_list->count;

        if (target == GL_ARRAY_BUFFER) {
            if (egl_state->state.array_buffer_binding == buffer)
                return;
            else {
                dispatch.BindBuffer (target, buffer);
                egl_state->state.need_get_error = true;

               /* FIXME: we don't know whether it succeeds or not */
               egl_state->state.array_buffer_binding = buffer;
            }
        }
        else if (target == GL_ELEMENT_ARRAY_BUFFER) {
            if (egl_state->state.element_array_buffer_binding == buffer)
                return;
            else {
                dispatch.BindBuffer (target, buffer);
                egl_state->state.need_get_error = true;

               /* FIXME: we don't know whether it succeeds or not */
               egl_state->state.element_array_buffer_binding = buffer;
            }
        }
        else {
            _gl_set_error (GL_INVALID_ENUM);
        }
    }
}

static void _gl_bind_framebuffer (GLenum target, GLuint framebuffer)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.BindFramebuffer) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (target == GL_FRAMEBUFFER &&
            egl_state->state.framebuffer_binding == framebuffer)
                return;

        if (target != GL_FRAMEBUFFER
#ifdef GL_ANGLE_framebuffer_blit
            &&
            target != GL_READ_FRAMEBUFFER_ANGLE &&
            target != GL_DRAW_FRAMEBUFFER_ANGLE
#endif
#ifdef GL_APPLE_framebuffer_multisample
            &&
            target != GL_READ_FRAMEBUFFER_APPLE &&
            target != GL_DRAW_FRAMEBUFFER_APPLE
#endif
        ) {
            _gl_set_error (GL_INVALID_ENUM);
        }

        dispatch.BindFramebuffer (target, framebuffer);
        /* FIXME: should we save it, it will be invalid if the
         * framebuffer is invalid 
         */
        egl_state->state.framebuffer_binding = framebuffer;

        /* egl_state->state.need_get_error = true; */
    }
}

static void _gl_bind_renderbuffer (GLenum target, GLuint renderbuffer)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.BindRenderbuffer) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (target != GL_RENDERBUFFER) {
            _gl_set_error (GL_INVALID_ENUM);
        }

        dispatch.BindRenderbuffer (target, renderbuffer);
        /* egl_state->state.need_get_error = true; */
    }
}

static void _gl_bind_texture (GLenum target, GLuint texture)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.BindTexture) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (target == GL_TEXTURE_2D &&
            egl_state->state.texture_binding[0] == texture)
            return;
        else if (target == GL_TEXTURE_CUBE_MAP &&
                 egl_state->state.texture_binding[1] == texture)
            return;
#ifdef GL_OES_texture_3D
        else if (target == GL_TEXTURE_3D_OES &&
                 egl_state->state.texture_binding_3d == texture)
            return;
#endif

        if (! (target == GL_TEXTURE_2D || target == GL_TEXTURE_CUBE_MAP
#ifdef GL_OES_texture_3D
           || target == GL_TEXTURE_3D_OES
#endif
          )) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        dispatch.BindTexture (target, texture);
        egl_state->state.need_get_error = true;

        /* FIXME: do we need to save them ? */
        if (target == GL_TEXTURE_2D)
            egl_state->state.texture_binding[0] = texture;
        else if (target == GL_TEXTURE_CUBE_MAP)
            egl_state->state.texture_binding[1] = texture;
#ifdef GL_OES_texture_3D
        else
            egl_state->state.texture_binding_3d = texture;
#endif

        /* egl_state->state.need_get_error = true; */
    }
}

static void _gl_blend_color (GLclampf red, GLclampf green,
                             GLclampf blue, GLclampf alpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (dispatch.BlendColor) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        state = &egl_state->state;

        if (state->blend_color[0] == red &&
            state->blend_color[1] == green &&
            state->blend_color[2] == blue &&
            state->blend_color[3] == alpha)
            return;

        state->blend_color[0] = red;
        state->blend_color[1] = green;
        state->blend_color[2] = blue;
        state->blend_color[3] = alpha;

        dispatch.BlendColor (red, green, blue, alpha);
    }
}

static void _gl_blend_equation (GLenum mode)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (dispatch.BlendEquation) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        state = &egl_state->state;
    
        if (state->blend_equation[0] == mode &&
            state->blend_equation[1] == mode)
            return;

        if (! (mode == GL_FUNC_ADD ||
               mode == GL_FUNC_SUBTRACT ||
               mode == GL_FUNC_REVERSE_SUBTRACT)) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        state->blend_equation[0] = mode;
        state->blend_equation[1] = mode;

        dispatch.BlendEquation (mode);
    }
}

static void _gl_blend_equation_separate (GLenum modeRGB, GLenum modeAlpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (dispatch.BlendEquationSeparate) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        state = &egl_state->state;
    
        if (state->blend_equation[0] == modeRGB &&
            state->blend_equation[1] == modeAlpha)
            return;

        if (! (modeRGB == GL_FUNC_ADD ||
               modeRGB == GL_FUNC_SUBTRACT ||
               modeRGB == GL_FUNC_REVERSE_SUBTRACT) || 
            ! (modeAlpha == GL_FUNC_ADD ||
               modeAlpha == GL_FUNC_SUBTRACT ||
               modeAlpha == GL_FUNC_REVERSE_SUBTRACT)) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        state->blend_equation[0] = modeRGB;
        state->blend_equation[1] = modeAlpha;

        dispatch.BlendEquationSeparate (modeRGB, modeAlpha);
    }
}

static void _gl_blend_func (GLenum sfactor, GLenum dfactor)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (dispatch.BlendFunc) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        state = &egl_state->state;
    
        if (state->blend_src[0] == sfactor &&
            state->blend_src[1] == sfactor &&
            state->blend_dst[0] == dfactor &&
            state->blend_dst[1] == dfactor)
            return;
    
        if (! (sfactor == GL_ZERO ||
               sfactor == GL_ONE ||
               sfactor == GL_SRC_COLOR ||
               sfactor == GL_ONE_MINUS_SRC_COLOR ||
               sfactor == GL_DST_COLOR ||
               sfactor == GL_ONE_MINUS_DST_COLOR ||
               sfactor == GL_SRC_ALPHA ||
               sfactor == GL_ONE_MINUS_SRC_ALPHA ||
               sfactor == GL_DST_ALPHA ||
               sfactor == GL_ONE_MINUS_DST_ALPHA ||
               sfactor == GL_CONSTANT_COLOR ||
               sfactor == GL_ONE_MINUS_CONSTANT_COLOR ||
               sfactor == GL_CONSTANT_ALPHA ||
               sfactor == GL_ONE_MINUS_CONSTANT_ALPHA ||
               sfactor == GL_SRC_ALPHA_SATURATE) ||
            ! (dfactor == GL_ZERO ||
               dfactor == GL_ONE ||
               dfactor == GL_SRC_COLOR ||
               dfactor == GL_ONE_MINUS_SRC_COLOR ||
               dfactor == GL_DST_COLOR ||
               dfactor == GL_ONE_MINUS_DST_COLOR ||
               dfactor == GL_SRC_ALPHA ||
               dfactor == GL_ONE_MINUS_SRC_ALPHA ||
               dfactor == GL_DST_ALPHA ||
               dfactor == GL_ONE_MINUS_DST_ALPHA ||
               dfactor == GL_CONSTANT_COLOR ||
               dfactor == GL_ONE_MINUS_CONSTANT_COLOR ||
               dfactor == GL_CONSTANT_ALPHA ||
               dfactor == GL_ONE_MINUS_CONSTANT_ALPHA ||
               dfactor == GL_SRC_ALPHA_SATURATE)) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        state->blend_src[0] = state->blend_src[1] = sfactor;
        state->blend_dst[0] = state->blend_dst[1] = dfactor;

        dispatch.BlendFunc (sfactor, dfactor);
    }
}

static void _gl_blend_func_separate (GLenum srcRGB, GLenum dstRGB,
                                     GLenum srcAlpha, GLenum dstAlpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (dispatch.BlendFuncSeparate) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        state = &egl_state->state;
    
        if (state->blend_src[0] == srcRGB &&
            state->blend_src[1] == srcAlpha &&
            state->blend_dst[0] == dstRGB &&
            state->blend_dst[1] == dstAlpha)
            return;

        if (! (srcRGB == GL_ZERO ||
               srcRGB == GL_ONE ||
               srcRGB == GL_SRC_COLOR ||
               srcRGB == GL_ONE_MINUS_SRC_COLOR ||
               srcRGB == GL_DST_COLOR ||
               srcRGB == GL_ONE_MINUS_DST_COLOR ||
               srcRGB == GL_SRC_ALPHA ||
               srcRGB == GL_ONE_MINUS_SRC_ALPHA ||
               srcRGB == GL_DST_ALPHA ||
               srcRGB == GL_ONE_MINUS_DST_ALPHA ||
               srcRGB == GL_CONSTANT_COLOR ||
               srcRGB == GL_ONE_MINUS_CONSTANT_COLOR ||
               srcRGB == GL_CONSTANT_ALPHA ||
               srcRGB == GL_ONE_MINUS_CONSTANT_ALPHA ||
               srcRGB == GL_SRC_ALPHA_SATURATE) ||
            ! (dstRGB == GL_ZERO ||
               dstRGB == GL_ONE ||
               dstRGB == GL_SRC_COLOR ||
               dstRGB == GL_ONE_MINUS_SRC_COLOR ||
               dstRGB == GL_DST_COLOR ||
               dstRGB == GL_ONE_MINUS_DST_COLOR ||
               dstRGB == GL_SRC_ALPHA ||
               dstRGB == GL_ONE_MINUS_SRC_ALPHA ||
               dstRGB == GL_DST_ALPHA ||
               dstRGB == GL_ONE_MINUS_DST_ALPHA ||
               dstRGB == GL_CONSTANT_COLOR ||
               dstRGB == GL_ONE_MINUS_CONSTANT_COLOR ||
               dstRGB == GL_CONSTANT_ALPHA ||
               dstRGB == GL_ONE_MINUS_CONSTANT_ALPHA ||
               dstRGB == GL_SRC_ALPHA_SATURATE) ||
            ! (srcAlpha == GL_ZERO ||
               srcAlpha == GL_ONE ||
               srcAlpha == GL_SRC_COLOR ||
               srcAlpha == GL_ONE_MINUS_SRC_COLOR ||
               srcAlpha == GL_DST_COLOR ||
               srcAlpha == GL_ONE_MINUS_DST_COLOR ||
               srcAlpha == GL_SRC_ALPHA ||
               srcAlpha == GL_ONE_MINUS_SRC_ALPHA ||
               srcAlpha == GL_DST_ALPHA ||
               srcAlpha == GL_ONE_MINUS_DST_ALPHA ||
               srcAlpha == GL_CONSTANT_COLOR ||
               srcAlpha == GL_ONE_MINUS_CONSTANT_COLOR ||
               srcAlpha == GL_CONSTANT_ALPHA ||
               srcAlpha == GL_ONE_MINUS_CONSTANT_ALPHA ||
               srcAlpha == GL_SRC_ALPHA_SATURATE) ||
            ! (dstAlpha == GL_ZERO ||
               dstAlpha == GL_ONE ||
               dstAlpha == GL_SRC_COLOR ||
               dstAlpha == GL_ONE_MINUS_SRC_COLOR ||
               dstAlpha == GL_DST_COLOR ||
               dstAlpha == GL_ONE_MINUS_DST_COLOR ||
               dstAlpha == GL_SRC_ALPHA ||
               dstAlpha == GL_ONE_MINUS_SRC_ALPHA ||
               dstAlpha == GL_DST_ALPHA ||
               dstAlpha == GL_ONE_MINUS_DST_ALPHA ||
               dstAlpha == GL_CONSTANT_COLOR ||
               dstAlpha == GL_ONE_MINUS_CONSTANT_COLOR ||
               dstAlpha == GL_CONSTANT_ALPHA ||
               dstAlpha == GL_ONE_MINUS_CONSTANT_ALPHA ||
               dstAlpha == GL_SRC_ALPHA_SATURATE)) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        state->blend_src[0] = srcRGB;
        state->blend_src[1] = srcAlpha;
        state->blend_dst[0] = dstRGB;
        state->blend_dst[0] = dstAlpha;

        dispatch.BlendFuncSeparate (srcRGB, dstRGB, srcAlpha, dstAlpha);
    }
}

static void _gl_buffer_data (GLenum target, GLsizeiptr size,
                             const GLvoid *data, GLenum usage)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.BufferData) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        /* XXX: we skip rest of check, because driver
         * can generate GL_OUT_OF_MEMORY, which cannot check
         */
        dispatch.BufferData (target, size, data, usage);
        egl_state = (egl_state_t *) active_state->data;
        egl_state->state.need_get_error = true;
    }

    if (data)
        free ((void *)data);
}

static void _gl_buffer_sub_data (GLenum target, GLintptr offset,
                                 GLsizeiptr size, const GLvoid *data)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.BufferSubData) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        /* XXX: we skip rest of check, because driver
         * can generate GL_INVALID_VALUE, when offset + data can be out of
         * bound
         */
        dispatch.BufferSubData (target, offset, size, data);
        egl_state->state.need_get_error = true;
    }

    if (data)
        free ((void *)data);
}

static GLenum _gl_check_framebuffer_status (GLenum target)
{
    GLenum result = GL_INVALID_ENUM;
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.CheckFramebufferStatus) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (target != GL_FRAMEBUFFER
#ifdef GL_ANGLE_framebuffer_blit
            &&
            target != GL_READ_FRAMEBUFFER_ANGLE &&
            target != GL_DRAW_FRAMEBUFFER_ANGLE
#endif
#ifdef GL_APPLE_framebuffer_multisample
            &&
            target != GL_READ_FRAMEBUFFER_APPLE &&
            target != GL_DRAW_FRAMEBUFFER_APPLE
#endif
        ) {
            _gl_set_error (GL_INVALID_ENUM);
            return result;
        }

        return dispatch.CheckFramebufferStatus (target);
    }

    return result;
}

static void _gl_clear (GLbitfield mask)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.Clear) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (! (mask & GL_COLOR_BUFFER_BIT ||
               mask & GL_DEPTH_BUFFER_BIT ||
               mask & GL_STENCIL_BUFFER_BIT)) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        dispatch.Clear (mask);
    }
}

static void _gl_clear_color (GLclampf red, GLclampf green,
                             GLclampf blue, GLclampf alpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (dispatch.ClearColor) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        state = &egl_state->state;

        if (state->color_clear_value[0] == red &&
            state->color_clear_value[1] == green &&
            state->color_clear_value[2] == blue &&
            state->color_clear_value[3] == alpha)
            return;

        state->color_clear_value[0] = red;
        state->color_clear_value[1] = green;
        state->color_clear_value[2] = blue;
        state->color_clear_value[3] = alpha;

        dispatch.ClearColor (red, green, blue, alpha);
    }
}

static void _gl_clear_depthf (GLclampf depth)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (dispatch.ClearDepthf) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        state = &egl_state->state;

        if (state->depth_clear_value == depth)
            return;

        state->depth_clear_value = depth;

        dispatch.ClearDepthf (depth);
    }
}

static void _gl_clear_stencil (GLint s)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (dispatch.ClearStencil) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        state = &egl_state->state;

        if (state->stencil_clear_value == s)
            return;

        state->stencil_clear_value = s;

        dispatch.ClearStencil (s);
    }
}

static void _gl_color_mask (GLboolean red, GLboolean green,
                            GLboolean blue, GLboolean alpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (dispatch.ColorMask) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        state = &egl_state->state;

        if (state->color_writemask[0] == red &&
            state->color_writemask[1] == green &&
            state->color_writemask[2] == blue &&
            state->color_writemask[3] == alpha)
            return;

        state->color_writemask[0] = red;
        state->color_writemask[1] = green;
        state->color_writemask[2] = blue;
        state->color_writemask[3] = alpha;

        dispatch.ColorMask (red, green, blue, alpha);
    }
}

static void _gl_compressed_tex_image_2d (GLenum target, GLint level,
                                         GLenum internalformat,
                                         GLsizei width, GLsizei height,
                                         GLint border, GLsizei imageSize,
                                         const GLvoid *data)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.CompressedTexImage2D) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.CompressedTexImage2D (target, level, internalformat,
                                       width, height, border, imageSize,
                                       data);

        egl_state->state.need_get_error = true;
    }

    if (data)
        free ((void *)data);
}

static void _gl_compressed_tex_sub_image_2d (GLenum target, GLint level,
                                             GLint xoffset, GLint yoffset,
                                             GLsizei width, GLsizei height,
                                             GLenum format, 
                                             GLsizei imageSize,
                                             const GLvoid *data)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.CompressedTexSubImage2D) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.CompressedTexSubImage2D (target, level, xoffset, yoffset,
                                          width, height, format, imageSize,
                                          data);

        egl_state->state.need_get_error = true;
    }

    if (data)
        free ((void *)data);
}

static void _gl_copy_tex_image_2d (GLenum target, GLint level,
                                   GLenum internalformat,
                                   GLint x, GLint y,
                                   GLsizei width, GLsizei height,
                                   GLint border)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.CopyTexImage2D) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.CopyTexImage2D (target, level, internalformat,
                                 x, y, width, height, border);

        egl_state->state.need_get_error = true;
    }
}

static void _gl_copy_tex_sub_image_2d (GLenum target, GLint level,
                                       GLint xoffset, GLint yoffset,
                                       GLint x, GLint y,
                                       GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.CopyTexSubImage2D) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.CopyTexSubImage2D (target, level, xoffset, yoffset,
                                    x, y, width, height);

        egl_state->state.need_get_error = true;
    }
}

/* This is a sync call */
static GLuint _gl_create_program  (void)
{
    GLuint result = 0;
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.CreateProgram) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        result = dispatch.CreateProgram ();
    }

    return result;
}

/* sync call */
static GLuint _gl_create_shader (GLenum shaderType)
{
    GLuint result = 0;
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.CreateShader) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (! (shaderType == GL_VERTEX_SHADER ||
               shaderType == GL_FRAGMENT_SHADER)) {
            _gl_set_error (GL_INVALID_ENUM);
            return result;
        }

        result = dispatch.CreateShader (shaderType);
    }

    return result;
}

static void _gl_cull_face (GLenum mode)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.CullFace) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (egl_state->state.cull_face_mode == mode)
            return;

        if (! (mode == GL_FRONT ||
               mode == GL_BACK ||
               mode == GL_FRONT_AND_BACK)) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        egl_state->state.cull_face_mode = mode;

        dispatch.CullFace (mode);
    }
}

static void _gl_delete_buffers (GLsizei n, const GLuint *buffers)
{
    egl_state_t *egl_state;
    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int count;
    int i, j;
    
    if (_gl_is_valid_func (dispatch.DeleteBuffers) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        attrib_list = &egl_state->state.vertex_attribs;
        vertex_attrib_t *attribs = attrib_list->attribs;
        count = attrib_list->count;

        if (n < 0) {
            _gl_set_error (GL_INVALID_VALUE);
            goto FINISH;
        }

        dispatch.DeleteBuffers (n, buffers);

        /* check array_buffer_binding and element_array_buffer_binding */
        for (i = 0; i < n; i++) {
            if (buffers[i] == egl_state->state.array_buffer_binding)
                egl_state->state.array_buffer_binding = 0;
            else if (buffers[i] == egl_state->state.element_array_buffer_binding)
                egl_state->state.element_array_buffer_binding = 0;
        }
        
        /* update client state */
        if (count == 0)
            goto FINISH;

        for (i = 0; i < n; i++) {
            if (attribs[0].array_buffer_binding == buffers[i]) {
                for (j = 0; j < count; j++) {
                    attribs[j].array_buffer_binding = 0;
                }
                break;
            }
        }
    }

FINISH:
    if (buffers)
        free ((void *)buffers);
}

static void _gl_delete_framebuffers (GLsizei n, const GLuint *framebuffers)
{
    egl_state_t *egl_state;
    int i;
    
    if (_gl_is_valid_func (dispatch.DeleteFramebuffers) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (n < 0) {
            _gl_set_error (GL_INVALID_VALUE);
            goto FINISH;
        }

        dispatch.DeleteFramebuffers (n, framebuffers);

        for (i = 0; i < n; i++) {
            if (egl_state->state.framebuffer_binding == framebuffers[i]) {
                egl_state->state.framebuffer_binding = 0;
                break;
            }
        }
    }

FINISH:
    if (framebuffers)
        free ((void *)framebuffers);
}

static void _gl_delete_program (GLuint program)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.DeleteProgram) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.DeleteProgram (program);

        egl_state->state.need_get_error = true;
    }
}

static void _gl_delete_renderbuffers (GLsizei n, const GLuint *renderbuffers)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.DeleteRenderbuffers) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (n < 0) {
            _gl_set_error (GL_INVALID_VALUE);
            goto FINISH;
        }

        dispatch.DeleteRenderbuffers (n, renderbuffers);
    }

FINISH:
    if (renderbuffers)
        free ((void *)renderbuffers);
}

static void _gl_delete_shader (GLuint shader)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.DeleteShader) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.DeleteShader (shader);

        egl_state->state.need_get_error = true;
    }
}

static void _gl_delete_textures (GLsizei n, const GLuint *textures)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.DeleteTextures) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (n < 0) {
            _gl_set_error (GL_INVALID_VALUE);
            goto FINISH;
        }

        dispatch.DeleteTextures (n, textures);
    }

FINISH:
    if (textures)
        free ((void *)textures);
}

static void _gl_depth_func (GLenum func)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.DepthFunc) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (egl_state->state.depth_func == func)
            return;

        if (! (func == GL_NEVER ||
               func == GL_LESS ||
               func == GL_EQUAL ||
               func == GL_LEQUAL ||
               func == GL_GREATER ||
               func == GL_NOTEQUAL ||
               func == GL_GEQUAL ||
               func == GL_ALWAYS)) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        egl_state->state.depth_func = func;

        dispatch.DepthFunc (func);
    }
}

static void _gl_depth_mask (GLboolean flag)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.DepthMask) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (egl_state->state.depth_writemask == flag)
            return;

        egl_state->state.depth_writemask = flag;

        dispatch.DepthMask (flag);
    }
}

static void _gl_depth_rangef (GLclampf nearVal, GLclampf farVal)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.DepthRangef) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (egl_state->state.depth_range[0] == nearVal &&
            egl_state->state.depth_range[1] == farVal)
            return;

        egl_state->state.depth_range[0] = nearVal;
        egl_state->state.depth_range[1] = farVal;

        dispatch.DepthRangef (nearVal, farVal);
    }
}

static void _gl_detach_shader (GLuint program, GLuint shader)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.DetachShader) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        /* XXX: command buffer, error code generated */
        dispatch.DetachShader (program, shader);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_set_cap (GLenum cap, GLboolean enable)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    bool needs_call = false;

    if (_gl_is_valid_func (dispatch.Disable) &&
        _gl_is_valid_func (dispatch.Enable) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        state = &egl_state->state;

        switch (cap) {
        case GL_BLEND:
            if (state->blend != enable) {
                state->blend = enable;
                needs_call = true;
            }
            break;
        case GL_CULL_FACE:
            if (state->cull_face != enable) {
                state->cull_face = enable;
                needs_call = true;
            }
            break;
        case GL_DEPTH_TEST:
            if (state->depth_test != enable) {
                state->depth_test = enable;
                needs_call = true;
            }
            break;
        case GL_DITHER:
            if (state->dither != enable) {
                state->dither = enable;
                needs_call = true;
            }
            break;
        case GL_POLYGON_OFFSET_FILL:
            if (state->polygon_offset_fill != enable) {
                state->polygon_offset_fill = enable;
                needs_call = true;
            }
            break;
        case GL_SAMPLE_ALPHA_TO_COVERAGE:
            if (state->sample_alpha_to_coverage != enable) {
                state->sample_alpha_to_coverage = enable;
                needs_call = true;
            }
            break;
        case GL_SAMPLE_COVERAGE:
            if (state->sample_coverage != enable) {
                state->sample_coverage = enable;
                needs_call = true;
            }
            break;
        case GL_SCISSOR_TEST:
            if (state->scissor_test != enable) {
                state->scissor_test = enable;
                needs_call = true;
            }
            break;
        case GL_STENCIL_TEST:
            if (state->stencil_test != enable) {
                state->stencil_test = enable;
                needs_call = true;
            }
            break;
        default:
            needs_call = true;
            state->need_get_error = true;
            break;
        }

        if (needs_call) {
            if (enable)
                dispatch.Enable (cap);
            else
                dispatch.Disable (cap);
        }
    }
}

static void _gl_disable (GLenum cap)
{
    _gl_set_cap (cap, GL_FALSE);
}

static void _gl_enable (GLenum cap)
{
    _gl_set_cap (cap, GL_TRUE);
}

static bool
_gl_index_is_too_large (gles2_state_t *state, GLuint index)
{
    if (index >= state->max_vertex_attribs) {
        if (! state->max_vertex_attribs_queried) {
            /* XXX: command buffer */
            dispatch.GetIntegerv (GL_MAX_VERTEX_ATTRIBS,
                                  &(state->max_vertex_attribs));
        }
        if (index >= state->max_vertex_attribs) {
            if (state->error == GL_NO_ERROR)
                state->error = GL_INVALID_VALUE;
            return true;
        }
    }

    return false;
}

static void 
_gl_set_vertex_attrib_array (GLuint index, gles2_state_t *state, 
                             GLboolean enable)
{
    vertex_attrib_list_t *attrib_list = &state->vertex_attribs;
    vertex_attrib_t *attribs = attrib_list->attribs;
    int count = attrib_list->count;
    int i, found_index = -1;

    GLint bound_buffer = 0;
    
    /* look into client state */
    for (i = 0; i < count; i++) {
        if (attribs[i].index == index) {
            if (attribs[i].array_enabled == enable)
                return;
            else {
                found_index = i;
                attribs[i].array_enabled = GL_FALSE;
                break;
            }
        }
    }
        
    /* gles2 spec says at least 8 */
    if (_gl_index_is_too_large (state, index)) {
        return;
    }

    if (enable == GL_FALSE)
        dispatch.DisableVertexAttribArray (index);
    else
        dispatch.EnableVertexAttribArray (index);

    bound_buffer = state->array_buffer_binding;

    /* update client state */
    if (found_index != -1)
        return;

    if (i < NUM_EMBEDDED) {
        attribs[i].array_enabled = enable;
        attribs[i].index = index;
        attribs[i].size = 0;
        attribs[i].stride = 0;
        attribs[i].type = GL_FLOAT;
        attribs[i].array_normalized = GL_FALSE;
        attribs[i].pointer = NULL;
        attribs[i].data = NULL;
        memset ((void *)attribs[i].current_attrib, 0, sizeof (GLfloat) * 4);
        attribs[i].array_buffer_binding = bound_buffer;
        attrib_list->count ++;
    }
    else {
        vertex_attrib_t *new_attribs = 
            (vertex_attrib_t *)malloc (sizeof (vertex_attrib_t) * (count + 1));

        memcpy (new_attribs, attribs, (count+1) * sizeof (vertex_attrib_t));
        if (attribs != attrib_list->embedded_attribs)
            free (attribs);

        new_attribs[count].array_enabled = enable;
        new_attribs[count].index = index;
        new_attribs[count].size = 0;
        new_attribs[count].stride = 0;
        new_attribs[count].type = GL_FLOAT;
        new_attribs[count].array_normalized = GL_FALSE;
        new_attribs[count].pointer = NULL;
        new_attribs[count].data = NULL;
        new_attribs[count].array_buffer_binding = bound_buffer;
        memset (new_attribs[count].current_attrib, 0, sizeof (GLfloat) * 4);
        attrib_list->attribs = new_attribs;
        attrib_list->count ++;
    }
}   

static void _gl_disable_vertex_attrib_array (GLuint index)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (dispatch.DisableVertexAttribArray) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        state = &egl_state->state;

        _gl_set_vertex_attrib_array (index, state, GL_FALSE);
    }
}

static void _gl_enable_vertex_attrib_array (GLuint index)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (dispatch.EnableVertexAttribArray) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        state = &egl_state->state;

        _gl_set_vertex_attrib_array (index, state, GL_TRUE);
    }
}

/* FIXME: we should use pre-allocated buffer if possible */
static char *
_gl_create_data_array (vertex_attrib_t *attrib, int count)
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

static void _gl_draw_arrays (GLenum mode, GLint first, GLsizei count)
{
    gles2_state_t *state;
    egl_state_t *egl_state;
    char *data;
    link_list_t *array_data = NULL;
    link_list_t *array, *new_array_data;
 
    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int i, found_index = -1;
    int n = 0;

    if (_gl_is_valid_func (dispatch.DrawArrays) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        state = &egl_state->state;
        attrib_list = &state->vertex_attribs;
        attribs = attrib_list->attribs;

        if (! (mode == GL_POINTS         || 
               mode == GL_LINE_STRIP     ||
               mode == GL_LINE_LOOP      || 
               mode == GL_LINES          ||
               mode == GL_TRIANGLE_STRIP || 
               mode == GL_TRIANGLE_FAN   ||
               mode == GL_TRIANGLES)) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }
        else if (count < 0) {
            _gl_set_error (GL_INVALID_VALUE);
            return;
        }

#ifdef GL_OES_vertex_array_object
        /* if vertex array binding is not 0 */
        if (state->vertex_array_binding) {
            dispatch.DrawArrays (mode, first, count);
            state->need_get_error = true;
            return;
        } else
#endif
  
        for (i = 0; i < attrib_list->count; i++) {
            if (! attribs[i].array_enabled)
                continue;
            /* we need to create a separate buffer for it */
            else if (! attribs[i].array_buffer_binding) {
                data = _gl_create_data_array (&attribs[i], count);
                if (! data)
                    continue;
                dispatch.VertexAttribPointer (attribs[i].index,
                                              attribs[i].size,
                                              attribs[i].type,
                                              attribs[i].array_normalized,
                                              0,
                                              data);
                /* create a data list to host our newly created data */
                if (array_data == NULL) {
                    array_data = (link_list_t *)malloc (sizeof (link_list_t));
                    array_data->prev = NULL;
                    array_data->next = NULL;
                    array_data->data = data;
                }
                else {
                    array = array_data;
                    while (array->next)
                         array = array->next;

                    new_array_data = (link_list_t *)malloc (sizeof (link_list_t));
                    new_array_data->prev = array;
                    new_array_data->next = NULL;
                    array_data->next = new_array_data;
                    new_array_data->data = data;
                }
            }
        }

        /* we need call DrawArrays */
        dispatch.DrawArrays (mode, first, count);

        /* remove data */
        array = array_data;
        while (array != NULL) {
            new_array_data = array;
            array = array->next;
            free (new_array_data->data);
            free (new_array_data);
        }
        /* should we need this?  The only error we could not catch is
         * GL_INVALID_FRAMEBUFFER_OPERATION
         */
        //egl_state->state.need_get_error = true;
    }
}

/* FIXME: we should use pre-allocated buffer if possible */
/*
static char *
_gl_create_indices_array (GLenum mode, GLenum type, int count,
                          char *indices)
{
    char *data = NULL;
    int length;
    int size = 0;

    if (type == GL_UNSIGNED_BYTE)
        size = sizeof (char);
    else if (type == GL_UNSIGNED_SHORT)
        size = sizeof (unsigned short);

    if (size == 0)
         return NULL;

    if(mode == GL_POINTS) {
        length = size * count;
    }
    else if (mode == GL_LINE_STRIP) {
        length = size * (count + 1);
    }
    else if (mode == GL_LINE_LOOP) {
        length = size * count;
    }
    else if (mode == GL_LINES) {
        length = size * count * 2;
    }
    else if (mode == GL_TRIANGLE_STRIP || mode == GL_TRIANGLE_FAN) {
        length = size * (count + 2);
    }
    else if (mode == GL_TRIANGLES) {
        length = sizeof (char) * (count * 3);
    }

    data = (char *)malloc (length);
    memcpy (data, indices, length);

    return data;
}*/

static void _gl_draw_elements (GLenum mode, GLsizei count, GLenum type,
                               const GLvoid *indices)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    char *data;
    link_list_t *array_data = NULL;
    link_list_t *array, *new_array_data;

    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int i, found_index = -1;
    int n = 0;
    bool element_binding = false;

    if (_gl_is_valid_func (dispatch.DrawElements) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        state = &egl_state->state;
        attrib_list = &state->vertex_attribs;
        attribs = attrib_list->attribs;
        element_binding = state->element_array_buffer_binding == 0 ? false : true;
        
        if (! (mode == GL_POINTS         || 
               mode == GL_LINE_STRIP     ||
               mode == GL_LINE_LOOP      || 
               mode == GL_LINES          ||
               mode == GL_TRIANGLE_STRIP || 
               mode == GL_TRIANGLE_FAN   ||
               mode == GL_TRIANGLES)) {
            _gl_set_error (GL_INVALID_ENUM);
            goto FINISH;
        }
        else if (! (type == GL_UNSIGNED_BYTE  || 
                    type == GL_UNSIGNED_SHORT)) {
            _gl_set_error (GL_INVALID_ENUM);
            goto FINISH;
        }
        else if (count < 0) {
            _gl_set_error (GL_INVALID_VALUE);
            goto FINISH;
        }

#ifdef GL_OES_vertex_array_object
        if (state->vertex_array_binding) {
            dispatch.DrawElements (mode, count, type, indices);
            state->need_get_error = true;
            goto FINISH;
        } else
#endif

        for (i = 0; i < attrib_list->count; i++) {
            if (! attribs[i].array_enabled)
                continue;
            /* we need to create a separate buffer for it */
            else if (! attribs[i].array_buffer_binding) {
                data = _gl_create_data_array (&attribs[i], count);
                if (! data)
                    continue;
                dispatch.VertexAttribPointer (attribs[i].index,
                                              attribs[i].size,
                                              attribs[i].type,
                                              attribs[i].array_normalized,
                                              0,
                                              data);
                /* save data */
                if (array_data == NULL) {
                    array_data = (link_list_t *)malloc (sizeof (link_list_t));
                    array_data->prev = NULL;
                    array_data->next = NULL;
                    array_data->data = data;
                }
                else {
                    array = array_data;
                    while (array->next)
                        array = array->next;

                    new_array_data = (link_list_t *)malloc (sizeof (link_list_t));
                    new_array_data->prev = array;
                    new_array_data->next = NULL;
                    array_data->next = new_array_data;
                    new_array_data->data = data;
                }
            }
        }

        dispatch.DrawElements (mode, type, count, indices);

        array = array_data;
        while (array != NULL) {
            new_array_data = array;
            array = array->next;
            free (new_array_data->data);
            free (new_array_data);
        }

        /* should we need this?  The only error we could not catch is
         * GL_INVALID_FRAMEBUFFER_OPERATION
         */
        //egl_state->state.need_get_error = true;
    }
FINISH:
    if (element_binding == false) {
        if (indices)
            free ((void *)indices);
    }
}

static void _gl_finish (void)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.Finish) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.Finish ();
    }
}

static void _gl_flush (void)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.Flush) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.Flush ();
    }
}

static void _gl_framebuffer_renderbuffer (GLenum target, GLenum attachment,
                                          GLenum renderbuffertarget,
                                          GLenum renderbuffer)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.FramebufferRenderbuffer) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (target != GL_FRAMEBUFFER) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }
        else if (renderbuffertarget != GL_RENDERBUFFER &&
                 renderbuffer != 0) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        dispatch.FramebufferRenderbuffer (target, attachment,
                                          renderbuffertarget, 
                                          renderbuffer);
        egl_state->state.need_get_error = true;
    }
}

static void _gl_framebuffer_texture_2d (GLenum target, GLenum attachment,
                                        GLenum textarget, GLuint texture, 
                                        GLint level)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.FramebufferTexture2D) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (target != GL_FRAMEBUFFER) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        dispatch.FramebufferTexture2D (target, attachment, textarget,
                                       texture, level);
        egl_state->state.need_get_error = true;
    }
}

static void _gl_front_face (GLenum mode)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.FramebufferTexture2D) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (egl_state->state.front_face == mode)
            return;

        if (! (mode == GL_CCW || mode == GL_CW)) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        egl_state->state.front_face = mode;
        dispatch.FrontFace (mode);
    }
}

static void _gl_gen_buffers (GLsizei n, GLuint *buffers)
{
    if (_gl_is_valid_func (dispatch.GenBuffers) &&
        _gl_is_valid_context ()) {

        if (n < 0) {
            _gl_set_error (GL_INVALID_VALUE);
            return;
        }
    
        dispatch.GenBuffers (n, buffers);
    }
}

static void _gl_gen_framebuffers (GLsizei n, GLuint *framebuffers)
{
    if (_gl_is_valid_func (dispatch.GenFramebuffers) &&
        _gl_is_valid_context ()) {

        if (n < 0) {
            _gl_set_error (GL_INVALID_VALUE);
            return;
        }
        
        dispatch.GenFramebuffers (n, framebuffers);
    }
}

static void _gl_gen_renderbuffers (GLsizei n, GLuint *renderbuffers)
{
    if (_gl_is_valid_func (dispatch.GenRenderbuffers) &&
        _gl_is_valid_context ()) {

        if (n < 0) {
            _gl_set_error (GL_INVALID_VALUE);
            return;
        }
        
        dispatch.GenRenderbuffers (n, renderbuffers);
    }
}

static void _gl_gen_textures (GLsizei n, GLuint *textures)
{
    if (_gl_is_valid_func (dispatch.GenTextures) &&
        _gl_is_valid_context ()) {

        if (n < 0) {
            _gl_set_error (GL_INVALID_VALUE);
            return;
        }
        
        dispatch.GenTextures (n, textures);
    }
}

static void _gl_generate_mipmap (GLenum target)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.GenerateMipmap) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (! (target == GL_TEXTURE_2D       || 
               target == GL_TEXTURE_CUBE_MAP
#ifdef GL_OES_texture_3D
                                             || 
               target == GL_TEXTURE_3D_OES
#endif
                                          )) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        dispatch.GenerateMipmap (target);
        egl_state->state.need_get_error = true;
    }
}

static void _gl_get_booleanv (GLenum pname, GLboolean *params)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.GetBooleanv) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        switch (pname) {
        case GL_BLEND:
            *params = egl_state->state.blend;
            break;
        case GL_COLOR_WRITEMASK:
            memcpy (params, egl_state->state.color_writemask, sizeof (GLboolean) * 4);
            break;
        case GL_CULL_FACE:
            *params = egl_state->state.cull_face;
            break;
        case GL_DEPTH_TEST:
            *params = egl_state->state.depth_test;
            break;
        case GL_DEPTH_WRITEMASK:
            *params = egl_state->state.depth_writemask;
            break;
        case GL_DITHER:
            *params = egl_state->state.dither;
            break;
        case GL_POLYGON_OFFSET_FILL:
            *params = egl_state->state.polygon_offset_fill;
            break;
        case GL_SAMPLE_ALPHA_TO_COVERAGE:
            *params = egl_state->state.sample_alpha_to_coverage;
            break;
        case GL_SAMPLE_COVERAGE:
            *params = egl_state->state.sample_coverage;
            break;
        case GL_SCISSOR_TEST:
            *params = egl_state->state.scissor_test;
            break;
        case GL_SHADER_COMPILER:
            *params = egl_state->state.shader_compiler;
            break;
        case GL_STENCIL_TEST:
            *params = egl_state->state.stencil_test;
            break;
        default:
            dispatch.GetBooleanv (pname, params);
            break;
        }
    }
}

static void _gl_get_floatv (GLenum pname, GLfloat *params)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.GetFloatv) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        switch (pname) {
        case GL_BLEND_COLOR:
            memcpy (params, egl_state->state.blend_color, sizeof (GLfloat) * 4);
            break;
        case GL_BLEND_DST_ALPHA:
            *params = egl_state->state.blend_dst[1];
            break;
        case GL_BLEND_DST_RGB:
            *params = egl_state->state.blend_dst[0];
            break;
        case GL_BLEND_EQUATION_ALPHA:
            *params = egl_state->state.blend_equation[1];
            break;
        case GL_BLEND_EQUATION_RGB:
            *params = egl_state->state.blend_equation[0];
            break;
        case GL_BLEND_SRC_ALPHA:
            *params = egl_state->state.blend_src[1];
            break;
        case GL_BLEND_SRC_RGB:
            *params = egl_state->state.blend_src[0];
            break;
        case GL_COLOR_CLEAR_VALUE:
            memcpy (params, egl_state->state.color_clear_value, sizeof (GLfloat) * 4);
            break;
        case GL_DEPTH_CLEAR_VALUE:
            *params = egl_state->state.depth_clear_value;
            break;
        case GL_DEPTH_RANGE:
            memcpy (params, egl_state->state.depth_range, sizeof (GLfloat) * 2);
            break;
        case GL_LINE_WIDTH:
            *params = egl_state->state.line_width;
            break;
        case GL_POLYGON_OFFSET_FACTOR:
            *params = egl_state->state.polygon_offset_factor;
            break; 
        default:
            dispatch.GetFloatv (pname, params);
            break;
        }
    }
}

static void _gl_get_integerv (GLenum pname, GLint *params)
{
    egl_state_t *egl_state;
    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int count;
    int i;
    
    if (_gl_is_valid_func (dispatch.GetIntegerv) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        attrib_list = &egl_state->state.vertex_attribs;
        attribs = attrib_list->attribs;
        count = attrib_list->count;

        switch (pname) {
        case GL_ACTIVE_TEXTURE:
            *params = egl_state->state.active_texture;
            break;
        case GL_CURRENT_PROGRAM:
            *params = egl_state->state.current_program;
            break;
        case GL_DEPTH_CLEAR_VALUE:
            *params = egl_state->state.depth_clear_value;
            break;
        case GL_DEPTH_FUNC:
            *params = egl_state->state.depth_func;
            break;
        case GL_DEPTH_RANGE:
            params[0] = egl_state->state.depth_range[0];
            params[1] = egl_state->state.depth_range[1];
            break;
        case GL_GENERATE_MIPMAP_HINT:
            *params = egl_state->state.generate_mipmap_hint;
            break;
        case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
            if (! egl_state->state.max_combined_texture_image_units_queried) {
                dispatch.GetIntegerv (pname, params);
                egl_state->state.max_combined_texture_image_units_queried = true;
                egl_state->state.max_combined_texture_image_units = *params;
            } else
                *params = egl_state->state.max_combined_texture_image_units;
            break;
        case GL_MAX_CUBE_MAP_TEXTURE_SIZE:
            if (! egl_state->state.max_cube_map_texture_size_queried) {
                dispatch.GetIntegerv (pname, params);
                egl_state->state.max_cube_map_texture_size = *params;
                egl_state->state.max_cube_map_texture_size_queried = true;
            } else
                *params = egl_state->state.max_cube_map_texture_size;
        break;
        case GL_MAX_FRAGMENT_UNIFORM_VECTORS:
            if (! egl_state->state.max_fragment_uniform_vectors_queried) {
                dispatch.GetIntegerv (pname, params);
                egl_state->state.max_fragment_uniform_vectors = *params;
                egl_state->state.max_fragment_uniform_vectors_queried = true;
            } else
                *params = egl_state->state.max_fragment_uniform_vectors;
            break;
        case GL_MAX_RENDERBUFFER_SIZE:
            if (! egl_state->state.max_renderbuffer_size_queried) {
                dispatch.GetIntegerv (pname, params);
                egl_state->state.max_renderbuffer_size = *params;
                egl_state->state.max_renderbuffer_size_queried = true;
            } else
                *params = egl_state->state.max_renderbuffer_size;
            break;
        case GL_MAX_TEXTURE_IMAGE_UNITS:
            if (! egl_state->state.max_texture_image_units_queried) {
                dispatch.GetIntegerv (pname, params);
                egl_state->state.max_texture_image_units = *params;
                egl_state->state.max_texture_image_units_queried = true;
            } else
                *params = egl_state->state.max_texture_image_units;
            break;
        case GL_MAX_VARYING_VECTORS:
            if (! egl_state->state.max_varying_vectors_queried) {
                dispatch.GetIntegerv (pname, params);
                egl_state->state.max_varying_vectors = *params;
                egl_state->state.max_varying_vectors_queried = true;
            } else
                *params = egl_state->state.max_varying_vectors;
            break;
        case GL_MAX_TEXTURE_SIZE:
            if (! egl_state->state.max_texture_size_queried) {
                dispatch.GetIntegerv (pname, params);
                egl_state->state.max_texture_size = *params;
                egl_state->state.max_texture_size_queried = true;
            } else
                *params = egl_state->state.max_texture_size;
            break;
        case GL_MAX_VERTEX_ATTRIBS:
            if (! egl_state->state.max_vertex_attribs_queried) {
                dispatch.GetIntegerv (pname, params);
                egl_state->state.max_vertex_attribs = *params;
                egl_state->state.max_vertex_attribs_queried = true;
            } else
                *params = egl_state->state.max_vertex_attribs;
            break;
        case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
            if (! egl_state->state.max_vertex_texture_image_units_queried) {
                dispatch.GetIntegerv (pname, params);
                egl_state->state.max_vertex_texture_image_units = *params;
                egl_state->state.max_vertex_texture_image_units_queried = true;
            } else
                *params = egl_state->state.max_vertex_texture_image_units;
            break;
        case GL_MAX_VERTEX_UNIFORM_VECTORS:
            if (! egl_state->state.max_vertex_uniform_vectors_queried) {
                dispatch.GetIntegerv (pname, params);
                egl_state->state.max_vertex_uniform_vectors = *params;
                egl_state->state.max_vertex_uniform_vectors_queried = true;
            } else
                *params = egl_state->state.max_vertex_uniform_vectors;
            break;
        case GL_POLYGON_OFFSET_UNITS:
            *params = egl_state->state.polygon_offset_units;
            break;
        case GL_SCISSOR_BOX:
            memcpy (params, egl_state->state.scissor_box, sizeof (GLint) * 4);
            break;
        case GL_STENCIL_BACK_FAIL:
            *params = egl_state->state.stencil_back_fail;
            break;
        case GL_STENCIL_BACK_FUNC:
            *params = egl_state->state.stencil_back_func;
            break;
        case GL_STENCIL_BACK_PASS_DEPTH_FAIL:
            *params = egl_state->state.stencil_back_pass_depth_fail;
            break;
        case GL_STENCIL_BACK_PASS_DEPTH_PASS:
            *params = egl_state->state.stencil_back_pass_depth_pass;
            break;
        case GL_STENCIL_BACK_REF:
            *params = egl_state->state.stencil_ref;
            break;
        case GL_STENCIL_BACK_VALUE_MASK:
            *params = egl_state->state.stencil_value_mask;
            break;
        case GL_STENCIL_CLEAR_VALUE:
            *params = egl_state->state.stencil_clear_value;
            break;
        case GL_STENCIL_FAIL:
            *params = egl_state->state.stencil_fail;
            break;
        case GL_STENCIL_FUNC:
            *params = egl_state->state.stencil_func;
            break;
        case GL_STENCIL_PASS_DEPTH_FAIL:
            *params = egl_state->state.stencil_pass_depth_fail;
            break;
        case GL_STENCIL_PASS_DEPTH_PASS:
            *params = egl_state->state.stencil_pass_depth_pass;
            break;
        case GL_STENCIL_REF:
            *params = egl_state->state.stencil_ref;
            break;
        case GL_STENCIL_VALUE_MASK:
            *params = egl_state->state.stencil_value_mask;
            break;
        case GL_STENCIL_WRITEMASK:
            *params = egl_state->state.stencil_writemask;
            break;
        case GL_STENCIL_BACK_WRITEMASK:
            *params = egl_state->state.stencil_back_writemask;
            break;
        case GL_VIEWPORT:
            memcpy (params, egl_state->state.viewport, sizeof (GLint) * 4);
            break;
        default:
            dispatch.GetIntegerv (pname, params);
            break;
        }

        if (pname == GL_ARRAY_BUFFER_BINDING) {
            egl_state->state.array_buffer_binding = *params;
            /* update client state */
            for (i = 0; i < count; i++) {
                attribs[i].array_buffer_binding = *params;
            }
        }
        else if (pname == GL_ELEMENT_ARRAY_BUFFER_BINDING)
            egl_state->state.array_buffer_binding = *params;
#ifdef GL_OES_vertex_array_object
        else if (pname == GL_VERTEX_ARRAY_BINDING_OES)
            egl_state->state.vertex_array_binding = *params;
#endif
    }
}

static void _gl_get_active_attrib (GLuint program, GLuint index,
                                   GLsizei bufsize, GLsizei *length,
                                   GLint *size, GLenum *type, GLchar *name)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.GetActiveAttrib) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.GetActiveAttrib (program, index, bufsize, length,
                                  size, type, name);
        egl_state->state.need_get_error = true;
    }
}

static void _gl_get_active_uniform (GLuint program, GLuint index, 
                                    GLsizei bufsize, GLsizei *length, 
                                    GLint *size, GLenum *type, 
                                    GLchar *name)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.GetActiveUniform) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.GetActiveUniform (program, index, bufsize, length,
                                   size, type, name);
        egl_state->state.need_get_error = true;
    }
}

static void _gl_get_attached_shaders (GLuint program, GLsizei maxCount,
                                      GLsizei *count, GLuint *shaders)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.GetAttachedShaders) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.GetAttachedShaders (program, maxCount, count, shaders);

        egl_state->state.need_get_error = true;
    }
}

static GLint _gl_get_attrib_location (GLuint program, const GLchar *name)
{
    egl_state_t *egl_state;
    GLint result = -1;
    
    if (_gl_is_valid_func (dispatch.GetAttribLocation) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        result = dispatch.GetAttribLocation (program, name);

        egl_state->state.need_get_error = true;
    }
    return result;
}

static void _gl_get_buffer_parameteriv (GLenum target, GLenum value, GLint *data)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.GetBufferParameteriv) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.GetBufferParameteriv (target, value, data);

        egl_state->state.need_get_error = true;
    }
}

static GLenum _gl_get_error (void)
{
    GLenum error = GL_INVALID_OPERATION;
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.GetError) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (! egl_state->state.need_get_error) {
            error = egl_state->state.error;
            egl_state->state.error = GL_NO_ERROR;
            return error;
        }

        error = dispatch.GetError ();

        egl_state->state.need_get_error = false;
        egl_state->state.error = GL_NO_ERROR;
    }

    return error;
}

static void _gl_get_framebuffer_attachment_parameteriv (GLenum target,
                                                        GLenum attachment,
                                                        GLenum pname,
                                                        GLint *params)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.GetFramebufferAttachmentParameteriv) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (target != GL_FRAMEBUFFER) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        dispatch.GetFramebufferAttachmentParameteriv (target, attachment,
                                                      pname, params);
        egl_state->state.need_get_error = true;
    }
}

static void _gl_get_program_info_log (GLuint program, GLsizei maxLength,
                                      GLsizei *length, GLchar *infoLog)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.GetProgramInfoLog) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.GetProgramInfoLog (program, maxLength, length, infoLog);

        egl_state->state.need_get_error = true;
    }
}

static void _gl_get_programiv (GLuint program, GLenum pname, GLint *params)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.GetProgramiv) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.GetProgramiv (program, pname, params);

        egl_state->state.need_get_error = true;
    }
}

static void _gl_get_renderbuffer_parameteriv (GLenum target,
                                              GLenum pname,
                                              GLint *params)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.GetRenderbufferParameteriv) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (target != GL_RENDERBUFFER) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        dispatch.GetRenderbufferParameteriv (target, pname, params);

        egl_state->state.need_get_error = true;
    }
}

static void _gl_get_shader_info_log (GLuint program, GLsizei maxLength,
                                     GLsizei *length, GLchar *infoLog)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.GetShaderInfoLog) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.GetShaderInfoLog (program, maxLength, length, infoLog);

        egl_state->state.need_get_error = true;
    }
}

static void _gl_get_shader_precision_format (GLenum shaderType, 
                                             GLenum precisionType,
                                             GLint *range, 
                                             GLint *precision)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.GetShaderPrecisionFormat) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.GetShaderPrecisionFormat (shaderType, precisionType,
                                           range, precision);

        egl_state->state.need_get_error = true;
    }
}

static void _gl_get_shader_source  (GLuint shader, GLsizei bufSize, 
                                    GLsizei *length, GLchar *source)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.GetShaderSource) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.GetShaderSource (shader, bufSize, length, source);

        egl_state->state.need_get_error = true;
    }
}

static void _gl_get_shaderiv (GLuint shader, GLenum pname, GLint *params)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.GetShaderiv) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.GetShaderiv (shader, pname, params);

        egl_state->state.need_get_error = true;
    }
}

static const GLubyte *_gl_get_string (GLenum name)
{
    GLubyte *result = NULL;
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.GetString) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (! (name == GL_VENDOR                   || 
               name == GL_RENDERER                 ||
               name == GL_SHADING_LANGUAGE_VERSION ||
               name == GL_EXTENSIONS)) {
            _gl_set_error (GL_INVALID_ENUM);
            return NULL;
        }

        result = (GLubyte *)dispatch.GetString (name);

        egl_state->state.need_get_error = true;
    }
    return (const GLubyte *)result;
}

static void _gl_get_tex_parameteriv (GLenum target, GLenum pname, 
                                     GLint *params)
{
    egl_state_t *egl_state;
    int active_texture_index;
    int target_index;
    
    if (_gl_is_valid_func (dispatch.GetTexParameteriv) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (! (target == GL_TEXTURE_2D       || 
               target == GL_TEXTURE_CUBE_MAP
#ifdef GL_OES_texture_3D
                                             || 
               target == GL_TEXTURE_3D_OES
#endif
                                          )) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        if (! (pname == GL_TEXTURE_MAG_FILTER ||
               pname == GL_TEXTURE_MIN_FILTER ||
               pname == GL_TEXTURE_WRAP_S     ||
               pname == GL_TEXTURE_WRAP_T
#ifdef GL_OES_texture_3D
                                              || 
               pname == GL_TEXTURE_WRAP_R_OES
#endif
                                             )) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        active_texture_index = egl_state->state.active_texture - GL_TEXTURE0;
        if (target == GL_TEXTURE_2D)
            target_index = 0;
        else if (target == GL_TEXTURE_CUBE_MAP)
            target_index = 1;
        else
            target_index = 2;

        if (pname == GL_TEXTURE_MAG_FILTER)
            *params = egl_state->state.texture_mag_filter[active_texture_index][target_index];
        else if (pname == GL_TEXTURE_MIN_FILTER)
            *params = egl_state->state.texture_min_filter[active_texture_index][target_index];
        else if (pname == GL_TEXTURE_WRAP_S)
            *params = egl_state->state.texture_wrap_s[active_texture_index][target_index];
        else if (pname == GL_TEXTURE_WRAP_T)
            *params = egl_state->state.texture_wrap_t[active_texture_index][target_index];
#ifdef GL_OES_texture_3D
        else if (pname == GL_TEXTURE_WRAP_R_OES)
            *params = egl_state->state.texture_3d_wrap_r[active_texture_index];
#endif
    }
}

static void _gl_get_tex_parameterfv (GLenum target, GLenum pname, GLfloat *params)
{
    GLint paramsi;

    _gl_get_tex_parameteriv (target, pname, &paramsi);
    *params = paramsi;
}

static void _gl_get_uniformiv (GLuint program, GLint location, 
                               GLint *params)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.GetUniformiv) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.GetUniformiv (program, location, params);

        egl_state->state.need_get_error = true;
    }
}

static void _gl_get_uniformfv (GLuint program, GLint location, 
                               GLfloat *params)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.GetUniformfv) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.GetUniformfv (program, location, params);

        egl_state->state.need_get_error = true;
    }
}

static GLint _gl_get_uniform_location (GLuint program, const GLchar *name)
{
    GLint result = -1;
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.GetUniformLocation) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        result = dispatch.GetUniformLocation (program, name);

        egl_state->state.need_get_error = true;
    }
    return result;
}

static void _gl_get_vertex_attribfv (GLuint index, GLenum pname, 
                                     GLfloat *params)
{
    egl_state_t *egl_state;

    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int count;
    int i, found_index = -1;

    if (_gl_is_valid_func (dispatch.GetVertexAttribfv) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        attrib_list = &egl_state->state.vertex_attribs;
        attribs = attrib_list->attribs;
        count = attrib_list->count;

        if (! (pname == GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING ||
               pname == GL_VERTEX_ATTRIB_ARRAY_ENABLED        ||
               pname == GL_VERTEX_ATTRIB_ARRAY_SIZE           ||
               pname == GL_VERTEX_ATTRIB_ARRAY_STRIDE         ||
               pname == GL_VERTEX_ATTRIB_ARRAY_TYPE           ||
               pname == GL_VERTEX_ATTRIB_ARRAY_NORMALIZED     ||
               pname == GL_CURRENT_VERTEX_ATTRIB)) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        /* check index is too large */
        if (_gl_index_is_too_large (&egl_state->state, index)) {
            return;
        }

#ifdef GL_OES_vertex_array_object
        /* we cannot use client state */
        if (egl_state->state.vertex_array_binding) {
            dispatch.GetVertexAttribfv (index, pname, params);
            return;
        }
#endif

        /* look into client state */
        for (i = 0; i < count; i++) {
            if (attribs[i].index == index) {
                switch (pname) {
                case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
                    *params = attribs[i].array_buffer_binding;
                    break;
                case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
                    *params = attribs[i].array_enabled;
                    break;
                case GL_VERTEX_ATTRIB_ARRAY_SIZE:
                    *params = attribs[i].size;
                    break;
                case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
                    *params = attribs[i].stride;
                    break;
                case GL_VERTEX_ATTRIB_ARRAY_TYPE:
                    *params = attribs[i].type;
                    break;
                case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
                    *params = attribs[i].array_normalized;
                    break;
                default:
                    memcpy (params, attribs[i].current_attrib, sizeof (GLfloat) * 4);
                    break;
                }
                return;
            }
        }

        /* we did not find anyting, return default */
        switch (pname) {
        case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
            *params = egl_state->state.array_buffer_binding;
            break;
        case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
            *params = false;
            break;
        case GL_VERTEX_ATTRIB_ARRAY_SIZE:
            *params = 4;
            break;
        case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
            *params = 0;
            break;
        case GL_VERTEX_ATTRIB_ARRAY_TYPE:
            *params = GL_FLOAT;
            break;
        case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
            *params = GL_FALSE;
            break;
        default:
            memset (params, 0, sizeof (GLfloat) * 4);
            break;
        }
    }
}

static void _gl_get_vertex_attribiv (GLuint index, GLenum pname, GLint *params)
{
    GLfloat paramsf[4];
    int i;

    glGetVertexAttribfv (index, pname, paramsf);

    if (pname == GL_CURRENT_VERTEX_ATTRIB) {
        for (i = 0; i < 4; i++)
            params[i] = paramsf[i];
    } else
        *params = paramsf[0];
}

static void _gl_get_vertex_attrib_pointerv (GLuint index, GLenum pname, 
                                            GLvoid **pointer)
{
    egl_state_t *egl_state;

    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int count;
    int i, found_index = -1;
    
    *pointer = 0;

    if (_gl_is_valid_func (dispatch.GetVertexAttribPointerv) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        attrib_list = &egl_state->state.vertex_attribs;
        attribs = attrib_list->attribs;
        count = attrib_list->count;

        if (pname != GL_VERTEX_ATTRIB_ARRAY_POINTER) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        /* XXX: check index validity */
        if (_gl_index_is_too_large (&egl_state->state, index)) {
            return;
        }

#ifdef GL_OES_vertex_array_object
        /* we cannot use client state */
        if (egl_state->state.vertex_array_binding) {
            dispatch.GetVertexAttribPointerv (index, pname, pointer);
            egl_state->state.need_get_error = true;
            return;
        }
#endif

        /* look into client state */
        for (i = 0; i < count; i++) {
            if (attribs[i].index == index) {
                *pointer = attribs[i].pointer;
                return;
            }
        }
    }
}

static void _gl_hint (GLenum target, GLenum mode)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.Hint) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (target == GL_GENERATE_MIPMAP_HINT &&
            egl_state->state.generate_mipmap_hint == mode)
            return;

        if ( !(mode == GL_FASTEST ||
               mode == GL_NICEST  ||
               mode == GL_DONT_CARE)) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        if (target == GL_GENERATE_MIPMAP_HINT)
            egl_state->state.generate_mipmap_hint = mode;

        dispatch.Hint (target, mode);

        if (target != GL_GENERATE_MIPMAP_HINT)
        egl_state->state.need_get_error = true;
    }
}

static GLboolean _gl_is_buffer (GLuint buffer)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (dispatch.IsBuffer) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        result = dispatch.IsBuffer (buffer);
    }
    return result;
}

static GLboolean _gl_is_enabled (GLenum cap)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (dispatch.IsEnabled) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        switch (cap) {
        case GL_BLEND:
            result = egl_state->state.blend;
            break;
        case GL_CULL_FACE:
            result = egl_state->state.cull_face;
            break;
        case GL_DEPTH_TEST:
            result = egl_state->state.depth_test;
            break;
        case GL_DITHER:
            result = egl_state->state.dither;
            break;
        case GL_POLYGON_OFFSET_FILL:
            result = egl_state->state.polygon_offset_fill;
            break;
        case GL_SAMPLE_ALPHA_TO_COVERAGE:
            result = egl_state->state.sample_alpha_to_coverage;
            break;
        case GL_SAMPLE_COVERAGE:
            result = egl_state->state.sample_coverage;
            break;
        case GL_SCISSOR_TEST:
            result = egl_state->state.scissor_test;
            break;
        case GL_STENCIL_TEST:
            result = egl_state->state.stencil_test;
            break;
        default:
            _gl_set_error (GL_INVALID_ENUM);
            break;
        }
    }
    return result;
}

static GLboolean _gl_is_framebuffer (GLuint framebuffer)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (dispatch.IsFramebuffer) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        result = dispatch.IsFramebuffer (framebuffer);
    }
    return result;
}

static GLboolean _gl_is_program (GLuint program)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (dispatch.IsProgram) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        result = dispatch.IsProgram (program);
    }
    return result;
}

static GLboolean _gl_is_renderbuffer (GLuint renderbuffer)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (dispatch.IsRenderbuffer) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        result = dispatch.IsRenderbuffer (renderbuffer);
    }
    return result;
}

static GLboolean _gl_is_shader (GLuint shader)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (dispatch.IsShader) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        result = dispatch.IsShader (shader);
    }
    return result;
}

static GLboolean _gl_is_texture (GLuint texture)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (dispatch.IsTexture) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        result = dispatch.IsTexture (texture);
    }
    return result;
}

static void _gl_line_width (GLfloat width)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.LineWidth) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (egl_state->state.line_width == width)
            return;

        if (width < 0) {
            _gl_set_error (GL_INVALID_VALUE);
            return;
        }

        egl_state->state.line_width = width;
        dispatch.LineWidth (width);
    }
}

static void _gl_link_program (GLuint program)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.LinkProgram) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.LinkProgram (program);
        egl_state->state.need_get_error = true;
    }
}

static void _gl_pixel_storei (GLenum pname, GLint param)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.PixelStorei) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if ((pname == GL_PACK_ALIGNMENT                &&
             egl_state->state.pack_alignment == param) ||
            (pname == GL_UNPACK_ALIGNMENT              &&
             egl_state->state.unpack_alignment == param))
            return;

        if (! (param == 1 ||
               param == 2 ||
               param == 4 ||
               param == 8)) {
            _gl_set_error (GL_INVALID_VALUE);
            return;
        }
        else if (! (pname == GL_PACK_ALIGNMENT ||
                    pname == GL_UNPACK_ALIGNMENT)) {
            _gl_set_error (GL_INVALID_VALUE);
            return;
        }

        if (pname == GL_PACK_ALIGNMENT)
           egl_state->state.pack_alignment = param;
        else
           egl_state->state.unpack_alignment = param;

        dispatch.PixelStorei (pname, param);
    }
}

static void _gl_polygon_offset (GLfloat factor, GLfloat units)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.PolygonOffset) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (egl_state->state.polygon_offset_factor == factor &&
            egl_state->state.polygon_offset_units == units)
            return;

        egl_state->state.polygon_offset_factor = factor;
        egl_state->state.polygon_offset_units = units;

        dispatch.PolygonOffset (factor, units);
    }
}

/* sync call */
static void _gl_read_pixels (GLint x, GLint y,
                             GLsizei width, GLsizei height,
                             GLenum format, GLenum type, GLvoid *data)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.ReadPixels) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.ReadPixels (x, y, width, height, format, type, data);
        egl_state->state.need_get_error = true;
    }
}

static void _gl_compile_shader (GLuint shader)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.CompileShader) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.CompileShader (shader);
        egl_state->state.need_get_error = true;
    }
}

static void _gl_release_shader_compiler (void)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.ReleaseShaderCompiler) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.ReleaseShaderCompiler ();
        egl_state->state.need_get_error = true;
    }
}

static void _gl_renderbuffer_storage (GLenum target, GLenum internalformat,
                                      GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.RenderbufferStorage) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.RenderbufferStorage (target, internalformat, width, height);
        egl_state->state.need_get_error = true;
    }
}

static void _gl_sample_coverage (GLclampf value, GLboolean invert)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.SampleCoverage) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (value == egl_state->state.sample_coverage_value &&
            invert == egl_state->state.sample_coverage_invert)
            return;

        egl_state->state.sample_coverage_invert = invert;
        egl_state->state.sample_coverage_value = value;

        dispatch.SampleCoverage (value, invert);
    }
}

static void _gl_scissor (GLint x, GLint y, GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.Scissor) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (x == egl_state->state.scissor_box[0]     &&
            y == egl_state->state.scissor_box[1]     &&
            width == egl_state->state.scissor_box[2] &&
            height == egl_state->state.scissor_box[3])
            return;

        if (width < 0 || height < 0) {
            _gl_set_error (GL_INVALID_VALUE);
            return;
        }

        egl_state->state.scissor_box[0] = x;
        egl_state->state.scissor_box[1] = y;
        egl_state->state.scissor_box[2] = width;
        egl_state->state.scissor_box[3] = height;

        dispatch.Scissor (x, y, width, height);
    }
}

static void _gl_shader_binary (GLsizei n, const GLuint *shaders,
                               GLenum binaryformat, const void *binary,
                               GLsizei length)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.ShaderBinary) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.ShaderBinary (n, shaders, binaryformat, binary, length);
        egl_state->state.need_get_error = true;
    }
    if (binary)
        free ((void *)binary);
}

static void _gl_shader_source (GLuint shader, GLsizei count,
                               const GLchar **string, const GLint *length)
{
    egl_state_t *egl_state;
    int i;
    
    if (_gl_is_valid_func (dispatch.ShaderSource) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.ShaderSource (shader, count, string, length);
        egl_state->state.need_get_error = true;
    }

    if (length)
        free ((void *)length);

    if (string) {
        for (i = 0; i < count; i++) {
            if (string[i]) {
                free ((char *)string[i]);
            }
        }
        free ((char **)string);
    }
}

static void _gl_stencil_func_separate (GLenum face, GLenum func,
                                       GLint ref, GLuint mask)
{
    egl_state_t *egl_state;
    bool needs_call = false;
    
    if (_gl_is_valid_func (dispatch.StencilFuncSeparate) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (! (face == GL_FRONT         ||
               face == GL_BACK          ||
               face == GL_FRONT_AND_BACK)) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }
        else if (! (func == GL_NEVER    ||
                    func == GL_LESS     ||
                    func == GL_LEQUAL   ||
                    func == GL_GREATER  ||
                    func == GL_GEQUAL   ||
                    func == GL_EQUAL    ||
                    func == GL_NOTEQUAL ||
                    func == GL_ALWAYS)) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }
        
        switch (face) {
        case GL_FRONT:
            if (func != egl_state->state.stencil_func ||
                ref != egl_state->state.stencil_ref   ||
                mask != egl_state->state.stencil_value_mask) {
                egl_state->state.stencil_func = func;
                egl_state->state.stencil_ref = ref;
                egl_state->state.stencil_value_mask = mask;
                needs_call = true;
            }
            break;
        case GL_BACK:
            if (func != egl_state->state.stencil_back_func ||
                ref != egl_state->state.stencil_back_ref   ||
                mask != egl_state->state.stencil_back_value_mask) {
                egl_state->state.stencil_back_func = func;
                egl_state->state.stencil_back_ref = ref;
                egl_state->state.stencil_back_value_mask = mask;
                needs_call = true;
            }
            break;
        default:
            if (func != egl_state->state.stencil_back_func       ||
                func != egl_state->state.stencil_func            ||
                ref != egl_state->state.stencil_back_ref         ||
                ref != egl_state->state.stencil_ref              ||
                mask != egl_state->state.stencil_back_value_mask ||
                mask != egl_state->state.stencil_value_mask) {
                egl_state->state.stencil_back_func = func;
                egl_state->state.stencil_func = func;
                egl_state->state.stencil_back_ref = ref;
                egl_state->state.stencil_ref = ref;
                egl_state->state.stencil_back_value_mask = mask;
                egl_state->state.stencil_value_mask = mask;
                needs_call = true;
            }
            break;
        }
    }

    if (needs_call)
        dispatch.StencilFuncSeparate (face, func, ref, mask);
}

static void _gl_stencil_func (GLenum func, GLint ref, GLuint mask)
{
    _gl_stencil_func_separate (GL_FRONT_AND_BACK, func, ref, mask);
}

static void _gl_stencil_mask_separate (GLenum face, GLuint mask)
{
    egl_state_t *egl_state;
    bool needs_call = false;
    
    if (_gl_is_valid_func (dispatch.StencilMaskSeparate) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (! (face == GL_FRONT         ||
               face == GL_BACK          ||
               face == GL_FRONT_AND_BACK)) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        switch (face) {
        case GL_FRONT:
            if (mask != egl_state->state.stencil_writemask) {
                egl_state->state.stencil_writemask = mask;
                needs_call = true;
            }
            break;
        case GL_BACK:
            if (mask != egl_state->state.stencil_back_writemask) {
                egl_state->state.stencil_back_writemask = mask;
                needs_call = true;
            }
            break;
        default:
            if (mask != egl_state->state.stencil_back_writemask ||
                mask != egl_state->state.stencil_writemask) {
                egl_state->state.stencil_back_writemask = mask;
                egl_state->state.stencil_writemask = mask;
                needs_call = true;
            }
            break;
        }
    }
    if (needs_call)
        dispatch.StencilMaskSeparate (face, mask);
}

static void _gl_stencil_mask (GLuint mask)
{
    _gl_stencil_mask_separate (GL_FRONT_AND_BACK, mask);
}

static void _gl_stencil_op_separate (GLenum face, GLenum sfail, 
                                     GLenum dpfail, GLenum dppass)
{
    egl_state_t *egl_state;
    bool needs_call = false;
    
    if (_gl_is_valid_func (dispatch.StencilOpSeparate) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (! (face == GL_FRONT         ||
               face == GL_BACK          ||
               face == GL_FRONT_AND_BACK)) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }
        else if (! (sfail == GL_KEEP       ||
                    sfail == GL_ZERO       ||
                    sfail == GL_REPLACE    ||
                    sfail == GL_INCR       ||
                    sfail == GL_INCR_WRAP  ||
                    sfail == GL_DECR       ||
                    sfail == GL_DECR_WRAP  ||
                    sfail == GL_INVERT)) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }
        else if (! (dpfail == GL_KEEP      ||
                    dpfail == GL_ZERO      ||
                    dpfail == GL_REPLACE   ||
                    dpfail == GL_INCR      ||
                    dpfail == GL_INCR_WRAP ||
                    dpfail == GL_DECR      ||
                    dpfail == GL_DECR_WRAP ||
                    dpfail == GL_INVERT)) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }
        else if (! (dppass == GL_KEEP      ||
                    dppass == GL_ZERO      ||
                    dppass == GL_REPLACE   ||
                    dppass == GL_INCR      ||
                    dppass == GL_INCR_WRAP ||
                    dppass == GL_DECR      ||
                    dppass == GL_DECR_WRAP ||
                    dppass == GL_INVERT)) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        switch (face) {
        case GL_FRONT:
            if (sfail != egl_state->state.stencil_fail             ||
                dpfail != egl_state->state.stencil_pass_depth_fail ||
                dppass != egl_state->state.stencil_pass_depth_pass) {
                egl_state->state.stencil_fail = sfail;
                egl_state->state.stencil_pass_depth_fail = dpfail;
                egl_state->state.stencil_pass_depth_pass = dppass;
                needs_call = true;
            }
            break;
        case GL_BACK:
            if (sfail != egl_state->state.stencil_back_fail             ||
                dpfail != egl_state->state.stencil_back_pass_depth_fail ||
                dppass != egl_state->state.stencil_back_pass_depth_pass) {
                egl_state->state.stencil_back_fail = sfail;
                egl_state->state.stencil_back_pass_depth_fail = dpfail;
                egl_state->state.stencil_back_pass_depth_pass = dppass;
                needs_call = true;
            }
            break;
        default:
            if (sfail != egl_state->state.stencil_fail                  ||
                dpfail != egl_state->state.stencil_pass_depth_fail      ||
                dppass != egl_state->state.stencil_pass_depth_pass      ||
                sfail != egl_state->state.stencil_back_fail             ||
                dpfail != egl_state->state.stencil_back_pass_depth_fail ||
                dppass != egl_state->state.stencil_back_pass_depth_pass) {
                egl_state->state.stencil_fail = sfail;
                egl_state->state.stencil_pass_depth_fail = dpfail;
                egl_state->state.stencil_pass_depth_pass = dppass;
                egl_state->state.stencil_back_fail = sfail;
                egl_state->state.stencil_back_pass_depth_fail = dpfail;
                egl_state->state.stencil_back_pass_depth_pass = dppass;
                needs_call = true;
            }
            break;
        }
    }

    if (needs_call)
        dispatch.StencilOpSeparate (face, sfail, dpfail, dppass);
}

static void _gl_stencil_op (GLenum sfail, GLenum dpfail, GLenum dppass)
{
    _gl_stencil_op_separate (GL_FRONT_AND_BACK, sfail, dpfail, dppass);
}

static void _gl_tex_image_2d (GLenum target, GLint level, 
                              GLint internalformat,
                              GLsizei width, GLsizei height, GLint border,
                              GLenum format, GLenum type, 
                              const GLvoid *data)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.TexImage2D) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.TexImage2D (target, level, internalformat, width, height,
                             border, format, type, data);
        egl_state->state.need_get_error = true;
    }
    if (data)
      free ((void *)data);
}

static void _gl_tex_parameteri (GLenum target, GLenum pname, GLint param)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    int active_texture_index;
    int target_index;

    if (_gl_is_valid_func (dispatch.TexParameteri) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        state = &egl_state->state;
        active_texture_index = state->active_texture - GL_TEXTURE0;

        if (! (target == GL_TEXTURE_2D       || 
               target == GL_TEXTURE_CUBE_MAP
#ifdef GL_OES_texture_3D
                                             || 
               target == GL_TEXTURE_3D_OES
#endif
                                          )) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        if (target == GL_TEXTURE_2D)
            target_index = 0;
        else if (target == GL_TEXTURE_CUBE_MAP)
            target_index = 1;
        else
            target_index = 2;

        if (pname == GL_TEXTURE_MAG_FILTER) {
            if (state->texture_mag_filter[active_texture_index][target_index] != param) {
                state->texture_mag_filter[active_texture_index][target_index] = param;
            }
            else
                return;
        }
        else if (pname == GL_TEXTURE_MIN_FILTER) {
            if (state->texture_min_filter[active_texture_index][target_index] != param) {
                state->texture_min_filter[active_texture_index][target_index] = param;
            }
            else
                return;
        }
        else if (pname == GL_TEXTURE_WRAP_S) {
            if (state->texture_wrap_s[active_texture_index][target_index] != param) {
                state->texture_wrap_s[active_texture_index][target_index] = param;
            }
            else
                return;
        }
        else if (pname == GL_TEXTURE_WRAP_T) {
            if (state->texture_wrap_t[active_texture_index][target_index] != param) {
                state->texture_wrap_t[active_texture_index][target_index] = param;
            }
            else
                return;
        }
#ifdef GL_OES_texture_3D
        else if (pname == GL_TEXTURE_WRAP_R_OES) {
            if (state->texture_3d_wrap_r[active_texture_index] != param) {
                state->texture_3d_wrap_r[active_texture_index] = param;
            }
            else
                return;
        }
#endif

        if (! (pname == GL_TEXTURE_MAG_FILTER ||
               pname == GL_TEXTURE_MIN_FILTER ||
               pname == GL_TEXTURE_WRAP_S     ||
               pname == GL_TEXTURE_WRAP_T
#ifdef GL_OES_texture_3D
                                              || 
               pname == GL_TEXTURE_WRAP_R_OES
#endif
                                             )) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        if (pname == GL_TEXTURE_MAG_FILTER &&
            ! (param == GL_NEAREST ||
               param == GL_LINEAR ||
               param == GL_NEAREST_MIPMAP_NEAREST ||
               param == GL_LINEAR_MIPMAP_NEAREST  ||
               param == GL_NEAREST_MIPMAP_LINEAR  ||
               param == GL_LINEAR_MIPMAP_LINEAR)) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }
        else if (pname == GL_TEXTURE_MIN_FILTER &&
                 ! (param == GL_NEAREST ||
                    param == GL_LINEAR)) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }
        else if ((pname == GL_TEXTURE_WRAP_S ||
                  pname == GL_TEXTURE_WRAP_T
#ifdef GL_OES_texture_3D
                                             || 
                  pname == GL_TEXTURE_WRAP_R_OES
#endif
                                                ) &&
                 ! (param == GL_CLAMP_TO_EDGE   ||
                    param == GL_MIRRORED_REPEAT ||
                    param == GL_REPEAT)) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        dispatch.TexParameteri (target, pname, param);
    }
}

static void _gl_tex_parameterf (GLenum target, GLenum pname, GLfloat param)
{
    GLint parami = param;
    _gl_tex_parameteri (target, pname, parami);
}

static void _gl_tex_sub_image_2d (GLenum target, GLint level,
                                  GLint xoffset, GLint yoffset,
                                  GLsizei width, GLsizei height,
                                  GLenum format, GLenum type, 
                                  const GLvoid *data)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.TexSubImage2D) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.TexSubImage2D (target, level, xoffset, yoffset,
                                width, height, format, type, data);
        egl_state->state.need_get_error = true;
    }
    if (data)
        free ((void *)data);
}

static void _gl_uniform1f (GLint location, GLfloat v0)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.Uniform1f) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.Uniform1f (location, v0);
        egl_state->state.need_get_error = true;
    }
}

static void _gl_uniform2f (GLint location, GLfloat v0, GLfloat v1)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.Uniform2f) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.Uniform2f (location, v0, v1);
        egl_state->state.need_get_error = true;
    }
}

static void _gl_uniform3f (GLint location, GLfloat v0, GLfloat v1, 
                           GLfloat v2)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.Uniform3f) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.Uniform3f (location, v0, v1, v2);
        egl_state->state.need_get_error = true;
    }
}

static void _gl_uniform4f (GLint location, GLfloat v0, GLfloat v1, 
                           GLfloat v2, GLfloat v3)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.Uniform4f) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.Uniform4f (location, v0, v1, v2, v3);
        egl_state->state.need_get_error = true;
    }
}

static void _gl_uniform1i (GLint location, GLint v0)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.Uniform1i) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.Uniform1i (location, v0);
        egl_state->state.need_get_error = true;
    }
}

static void _gl_uniform_2i (GLint location, GLint v0, GLint v1)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.Uniform2i) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.Uniform2i (location, v0, v1);
        egl_state->state.need_get_error = true;
    }
}

static void _gl_uniform3i (GLint location, GLint v0, GLint v1, GLint v2)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.Uniform3i) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.Uniform3i (location, v0, v1, v2);
        egl_state->state.need_get_error = true;
    }
}

static void _gl_uniform4i (GLint location, GLint v0, GLint v1, 
                           GLint v2, GLint v3)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.Uniform4i) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.Uniform4i (location, v0, v1, v2, v3);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_uniform_fv (int i, GLint location,
                GLsizei count, const GLfloat *value)
{
    egl_state_t *egl_state;

    if (! _is_valid_context ())
        goto FINISH;

    switch (i) {
    case 1:
        if(! _is_valid_func (dispatch.Uniform1fv))
            goto FINISH;
        dispatch.Uniform1fv (location, count, value);
        break;
    case 2:
        if(! _is_valid_func (dispatch.Uniform2fv))
            goto FINISH;
        dispatch.Uniform2fv (location, count, value);
        break;
    case 3:
        if(! _is_valid_func (dispatch.Uniform3fv))
            goto FINISH;
        dispatch.Uniform3fv (location, count, value);
        break;
    default:
        if(! _is_valid_func (dispatch.Uniform4fv))
            goto FINISH;
        dispatch.Uniform4fv (location, count, value);
        break;
    }

    egl_state = (egl_state_t *) active_state->data;
    egl_state->state.need_get_error = true;

FINISH:
    if (value)
        free ((GLfloat *)value);
}

static void _gl_uniform1fv (GLint location, GLsizei count, 
                            const GLfloat *value)
{
    _gl_uniform_fv (1, location, count, value);
}

static void _gl_uniform2fv (GLint location, GLsizei count, 
                            const GLfloat *value)
{
    _gl_uniform_fv (2, location, count, value);
}

static void _gl_uniform3fv (GLint location, GLsizei count, 
                            const GLfloat *value)
{
    _gl_uniform_fv (3, location, count, value);
}

static void _gl_uniform4fv (GLint location, GLsizei count, 
                            const GLfloat *value)
{
    _gl_uniform_fv (4, location, count, value);
}

static void
_gl_uniform_iv (int i, GLint location,
                GLsizei count, const GLint *value)
{
    egl_state_t *egl_state;

    if (! _is_valid_context ())
        goto FINISH;

    switch (i) {
    case 1:
        if(! _is_valid_func (dispatch.Uniform1iv))
            goto FINISH;
        dispatch.Uniform1iv (location, count, value);
        break;
    case 2:
        if(! _is_valid_func (dispatch.Uniform2iv))
            goto FINISH;
        dispatch.Uniform2iv (location, count, value);
        break;
    case 3:
        if(! _is_valid_func (dispatch.Uniform3iv))
            goto FINISH;
        dispatch.Uniform3iv (location, count, value);
        break;
    default:
        if(! _is_valid_func (dispatch.Uniform4iv))
            goto FINISH;
        dispatch.Uniform4iv (location, count, value);
        break;
    }

    egl_state = (egl_state_t *) active_state->data;
    egl_state->state.need_get_error = true;

FINISH:
    if (value)
        free ((GLint *)value);
}

static void _gl_uniform1iv (GLint location, GLsizei count, 
                            const GLint *value)
{
    _gl_uniform_iv (1, location, count, value);
}

static void _gl_uniform2iv (GLint location, GLsizei count, 
                            const GLint *value)
{
    _gl_uniform_iv (2, location, count, value);
}

static void _gl_uniform3iv (GLint location, GLsizei count, 
                            const GLint *value)
{
    _gl_uniform_iv (3, location, count, value);
}

static void _gl_uniform4iv (GLint location, GLsizei count, 
                            const GLint *value)
{
    _gl_uniform_iv (4, location, count, value);
}

static void _gl_use_program (GLuint program)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.UseProgram) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (egl_state->state.current_program == program)
            return;

        dispatch.UseProgram (program);
        /* FIXME: this maybe not right because this program may be invalid
         * object, we save here to save time in glGetError() */
        egl_state->state.current_program = program;
        /* FIXME: do we need to have this ? */
        // egl_state->state.need_get_error = true;
    }
}

static void _gl_validate_program (GLuint program)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.ValidateProgram) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.ValidateProgram (program);
        egl_state->state.need_get_error = true;
    }
}

static void _gl_vertex_attrib1f (GLuint index, GLfloat v0)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.VertexAttrib1f) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (_gl_index_is_too_large (&egl_state->state, index)) {
            return;
        }

        dispatch.VertexAttrib1f (index, v0);
    }
}

static void _gl_vertex_attrib2f (GLuint index, GLfloat v0, GLfloat v1)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.VertexAttrib2f) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (_gl_index_is_too_large (&egl_state->state, index)) {
            return;
        }

        dispatch.VertexAttrib2f (index, v0, v1);
    }
}

static void _gl_vertex_attrib3f (GLuint index, GLfloat v0, 
                                 GLfloat v1, GLfloat v2)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.VertexAttrib3f) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (_gl_index_is_too_large (&egl_state->state, index)) {
            return;
        }

        dispatch.VertexAttrib3f (index, v0, v1, v2);
    }
}

static void _gl_vertex_attrib4f (GLuint index, GLfloat v0, GLfloat v1, 
                                 GLfloat v2, GLfloat v3)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.VertexAttrib3f) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (_gl_index_is_too_large (&egl_state->state, index))
            return;

        dispatch.VertexAttrib4f (index, v0, v1, v2, v3);
    }
}

static void
_gl_vertex_attrib_fv (int i, GLuint index, const GLfloat *v)
{
    egl_state_t *egl_state;
    
    if (! _is_valid_context ())
        goto FINISH;
    
    egl_state = (egl_state_t *) active_state->data;

    if (_gl_index_is_too_large (&egl_state->state, index))
        goto FINISH;

    switch (i) {
        case 1:
            if(! _is_valid_func (dispatch.VertexAttrib1fv))
                goto FINISH;
            dispatch.VertexAttrib1fv (index, v);
            break;
        case 2:
            if(! _is_valid_func (dispatch.VertexAttrib2fv))
                goto FINISH;
            dispatch.VertexAttrib2fv (index, v);
            break;
        case 3:
            if(! _is_valid_func (dispatch.VertexAttrib3fv))
                goto FINISH;
            dispatch.VertexAttrib3fv (index, v);
            break;
        default:
            if(! _is_valid_func (dispatch.VertexAttrib4fv))
                goto FINISH;
            dispatch.VertexAttrib4fv (index, v);
            break;
    }

FINISH:
    if (v)
        free ((GLfloat *)v);
}

static void _gl_vertex_attrib1fv (GLuint index, const GLfloat *v)
{
    _gl_vertex_attrib_fv (1, index, v);
}

static void _gl_vertex_attrib2fv (GLuint index, const GLfloat *v)
{
    _gl_vertex_attrib_fv (2, index, v);
}

static void _gl_vertex_attrib3fv (GLuint index, const GLfloat *v)
{
    _gl_vertex_attrib_fv (3, index, v);
}

static void _gl_vertex_attrib4fv (GLuint index, const GLfloat *v)
{
    _gl_vertex_attrib_fv (4, index, v);
}

static void _gl_vertex_attrib_pointer (GLuint index, GLint size, 
                                       GLenum type, GLboolean normalized, 
                                       GLsizei stride, const GLvoid *pointer)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    char *data;
    link_list_t *array_data = NULL;
    link_list_t *array, *new_array_data;

    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int i, found_index = -1;
    int count;
    int n = 0;
    GLint bound_buffer = 0;

    if (_gl_is_valid_func (dispatch.VertexAttribPointer) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        state = &egl_state->state;
        attrib_list = &state->vertex_attribs;
        attribs = attrib_list->attribs;
        count = attrib_list->count;
        
        if (! (type == GL_BYTE                 ||
               type == GL_UNSIGNED_BYTE        ||
               type == GL_SHORT                ||
               type == GL_UNSIGNED_SHORT       ||
               type == GL_FLOAT)) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }
        else if (size > 4 || size < 1 || stride < 0) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        /* check max_vertex_attribs */
        if (_gl_index_is_too_large (&egl_state->state, index)) {
            return;
        }
        
#ifdef GL_OES_vertex_array_object
        if (egl_state->state.vertex_array_binding) {
            dispatch.VertexAttribPointer (index, size, type, normalized,
                                          stride, pointer);
            //egl_state->state.need_get_error = true;
            return;
        }
#endif
        bound_buffer = egl_state->state.array_buffer_binding;
        /* check existing client state */
        for (i = 0; i < count; i++) {
            if (attribs[i].index == index) {
                if (attribs[i].size == size &&
                    attribs[i].type == type &&
                    attribs[i].stride == stride) {
                    attribs[i].array_normalized = normalized;
                    attribs[i].pointer = (GLvoid *)pointer;
                    attribs[i].array_buffer_binding = bound_buffer;
                    return;
                }
                else {
                    found_index = i;
                    break;
                }
            }
        }

        /* use array_buffer? */
        if (bound_buffer) {
            dispatch.VertexAttribPointer (index, size, type, normalized,
                                          stride, pointer);
            //egl_state->state.need_get_error = true;
        }

        if (found_index != -1) {
            attribs[found_index].size = size;
            attribs[found_index].stride = stride;
            attribs[found_index].type = type;
            attribs[found_index].array_normalized = normalized;
            attribs[found_index].pointer = (GLvoid *)pointer;
            attribs[found_index].array_buffer_binding = bound_buffer;
            return;
        }

        /* we have not found index */
        if (i < NUM_EMBEDDED) {
            attribs[i].index = index;
            attribs[i].size = size;
            attribs[i].stride = stride;
            attribs[i].type = type;
            attribs[i].array_normalized = normalized;
            attribs[i].pointer = (GLvoid *)pointer;
            attribs[i].data = NULL;
            attribs[i].array_enabled = GL_FALSE;
            attribs[i].array_buffer_binding = bound_buffer;
            memset (attribs[i].current_attrib, 0, sizeof (GLfloat) * 4);
            attrib_list->count ++;
        }
        else {
            vertex_attrib_t *new_attribs = (vertex_attrib_t *)malloc (sizeof (vertex_attrib_t) * (count + 1));

            memcpy (new_attribs, attribs, (count+1) * sizeof (vertex_attrib_t));
            if (attribs != attrib_list->embedded_attribs)
                free (attribs);

            new_attribs[count].index = index;
            new_attribs[count].size = size;
            new_attribs[count].stride = stride;
            new_attribs[count].type = type;
            new_attribs[count].array_normalized = normalized;
            new_attribs[count].pointer = (GLvoid *)pointer;
            new_attribs[count].data = NULL;
            new_attribs[count].array_enabled = GL_FALSE;
            new_attribs[count].array_buffer_binding = bound_buffer;
            memset (new_attribs[count].current_attrib, 0, sizeof (GLfloat) * 4);

            attrib_list->attribs = new_attribs;
            attrib_list->count ++;
        }
    }
}

static void _gl_viewport (GLint x, GLint y, GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.Viewport) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (egl_state->state.viewport[0] == x     &&
            egl_state->state.viewport[1] == y     &&
            egl_state->state.viewport[2] == width &&
            egl_state->state.viewport[3] == height)
            return;

        if (x < 0 || y < 0) {
            _gl_set_error (GL_INVALID_VALUE);
            return;
        }

        egl_state->state.viewport[0] = x;
        egl_state->state.viewport[1] = y;
        egl_state->state.viewport[2] = width;
        egl_state->state.viewport[3] = height;

        dispatch.Viewport (x, y, width, height);
    }
}
/* end of GLES2 core profile */

#ifdef GL_OES_EGL_image
static void
_gl_egl_image_target_texture_2d_oes (GLenum target, GLeglImageOES image)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.EGLImageTargetTexture2DOES) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (target != GL_TEXTURE_2D) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        dispatch.EGLImageTargetTexture2DOES (target, image);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_egl_image_target_renderbuffer_storage_oes (GLenum target, 
                                               GLeglImageOES image)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.EGLImageTargetRenderbufferStorageOES) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
#ifndef HAS_GLES2
        if (target != GL_RENDERBUFFER_OES) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }
#endif

        dispatch.EGLImageTargetRenderbufferStorageOES (target, image);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_OES_get_program_binary
static void
_gl_get_program_binary_oes (GLuint program, GLsizei bufSize, 
                            GLsizei *length, GLenum *binaryFormat, 
                            GLvoid *binary)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.GetProgramBinaryOES) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.GetProgramBinaryOES (program, bufSize, length, 
                                      binaryFormat, binary);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_program_binary_oes (GLuint program, GLenum binaryFormat,
                        const GLvoid *binary, GLint length)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (dispatch.ProgramBinaryOES) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.ProgramBinaryOES (program, binaryFormat, binary, length);
        egl_state->state.need_get_error = true;
    }

    if (binary)
        free ((void *)binary);
}
#endif

#ifdef GL_OES_mapbuffer
static void* 
_gl_map_buffer_oes (GLenum target, GLenum access)
{
    egl_state_t *egl_state;
    void *result = NULL;
    
    if (_gl_is_valid_func (dispatch.MapBufferOES) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (access != GL_WRITE_ONLY_OES) {
            _gl_set_error (GL_INVALID_ENUM);
            return result;
        }
        else if (! (target == GL_ARRAY_BUFFER ||
                    target == GL_ELEMENT_ARRAY_BUFFER)) {
            _gl_set_error (GL_INVALID_ENUM);
            return result;
        }

        result = dispatch.MapBufferOES (target, access);
        egl_state->state.need_get_error = true;
    }
    return result;
}

static GLboolean
_gl_unmap_buffer_oes (GLenum target)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (dispatch.UnmapBufferOES) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (! (target == GL_ARRAY_BUFFER ||
               target == GL_ELEMENT_ARRAY_BUFFER)) {
            _gl_set_error (GL_INVALID_ENUM);
            return result;
        }

        result = dispatch.UnmapBufferOES (target);
        egl_state->state.need_get_error = true;
    }
    return result;
}

static void
_gl_get_buffer_pointerv_oes (GLenum target, GLenum pname, GLvoid **params)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.GetBufferPointervOES) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (! (target == GL_ARRAY_BUFFER ||
               target == GL_ELEMENT_ARRAY_BUFFER)) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        dispatch.GetBufferPointervOES (target, pname, params);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_OES_texture_3D
static void
_gl_tex_image_3d_oes (GLenum target, GLint level, GLenum internalformat,
                      GLsizei width, GLsizei height, GLsizei depth,
                      GLint border, GLenum format, GLenum type,
                      const GLvoid *pixels)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.TexImage3DOES) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.TexImage3DOES (target, level, internalformat, 
                                width, height, depth, 
                                border, format, type, pixels);
        egl_state->state.need_get_error = true;
    }
    if (pixels)
        free ((void *)pixels);
}

static void
_gl_tex_sub_image_3d_oes (GLenum target, GLint level,
                          GLint xoffset, GLint yoffset, GLint zoffset,
                          GLsizei width, GLsizei height, GLsizei depth,
                          GLenum format, GLenum type, const GLvoid *data)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.TexSubImage3DOES) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.TexSubImage3DOES (target, level, 
                                   xoffset, yoffset, zoffset,
                                   width, height, depth, 
                                   format, type, data);
        egl_state->state.need_get_error = true;
    }
    if (data)
        free ((void *)data);
}

static void
_gl_copy_tex_sub_image_3d_oes (GLenum target, GLint level,
                               GLint xoffset, GLint yoffset, GLint zoffset,
                               GLint x, GLint y,
                               GLint width, GLint height)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.CopyTexSubImage3DOES) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.CopyTexSubImage3DOES (target, level,
                                       xoffset, yoffset, zoffset,
                                       x, y, width, height);

        egl_state->state.need_get_error = true;
    }
}

static void
_gl_compressed_tex_image_3d_oes (GLenum target, GLint level,
                                 GLenum internalformat,
                                 GLsizei width, GLsizei height, 
                                 GLsizei depth,
                                 GLint border, GLsizei imageSize,
                                 const GLvoid *data)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.CompressedTexImage3DOES) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.CompressedTexImage3DOES (target, level, internalformat,
                                          width, height, depth,
                                          border, imageSize, data);

        egl_state->state.need_get_error = true;
    }
    if (data)
        free ((void *)data);
}

static void
_gl_compressed_tex_sub_image_3d_oes (GLenum target, GLint level,
                                     GLint xoffset, GLint yoffset, 
                                     GLint zoffset,
                                     GLsizei width, GLsizei height, 
                                     GLsizei depth,
                                     GLenum format, GLsizei imageSize,
                                     const GLvoid *data)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.CompressedTexSubImage3DOES) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        dispatch.CompressedTexSubImage3DOES (target, level,
                                             xoffset, yoffset, zoffset,
                                             width, height, depth,
                                             format, imageSize, data);

        egl_state->state.need_get_error = true;
    }

    if (data)
        free ((void *)data);
}

static void
_gl_framebuffer_texture_3d_oes (GLenum target, GLenum attachment,
                                GLenum textarget, GLuint texture,
                                GLint level, GLint zoffset)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.FramebufferTexture3DOES) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (target != GL_FRAMEBUFFER) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        dispatch.FramebufferTexture3DOES (target, attachment, textarget,
                                          texture, level, zoffset);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_OES_vertex_array_object
/* spec: http://www.hhronos.org/registry/gles/extensions/OES/OES_vertex_array_object.txt
 * spec says it generates GL_INVALID_OPERATION if 
 * (1) array is not generated by glGenVertexArrayOES()
 * (2) the array object has been deleted by glDeleteVertexArrayOES()
 * 
 * SPECIAL ATTENTION:
 * this call also affect glVertexAttribPointer() and glDrawXXXX(),
 * if there is a vertex_array_binding, instead of using client state, 
 * we pass them to server.
 */
static void
_gl_bind_vertex_array_oes (GLuint array)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.BindVertexArrayOES) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;

        if (egl_state->state.vertex_array_binding == array)
            return;

        dispatch.BindVertexArrayOES (array);
        egl_state->state.need_get_error = true;
        /* FIXME: should we save this ? */
        egl_state->state.vertex_array_binding = array;
    }
}

static void
_gl_delete_vertex_arrays_oes (GLsizei n, const GLuint *arrays)
{
    egl_state_t *egl_state;
    int i;

    if (_gl_is_valid_func (dispatch.DeleteVertexArraysOES) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        
        if (n <= 0) {
            _gl_set_error (GL_INVALID_VALUE);
            goto FINISH;
        }

        dispatch.DeleteVertexArraysOES (n, arrays);

        /* matching vertex_array_binding ? */
        for (i = 0; i < n; i++) {
            if (arrays[i] == egl_state->state.vertex_array_binding) {
                egl_state->state.vertex_array_binding = 0;
                break;
            }
        }
    }

FINISH:
    if (arrays)
        free ((void *)arrays);
}

static void
_gl_gen_vertex_arrays_oes (GLsizei n, GLuint *arrays)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.GenVertexArraysOES) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        
        if (n <= 0) {
            _gl_set_error (GL_INVALID_VALUE);
            return;
        }

        dispatch.GenVertexArraysOES (n, arrays);
    }
}

static GLboolean
_gl_is_vertex_array_oes (GLuint array)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (dispatch.IsVertexArrayOES) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        
        result = dispatch.IsVertexArrayOES (array);

        if (result == GL_FALSE && 
            egl_state->state.vertex_array_binding == array)
            egl_state->state.vertex_array_binding = 0;
    }
    return result;
}
#endif

#ifdef GL_AMD_performance_monitor
static void
_gl_get_perf_monitor_groups_amd (GLint *numGroups, GLsizei groupSize, 
                                 GLuint *groups)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.GetPerfMonitorGroupsAMD) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        
        dispatch.GetPerfMonitorGroupsAMD (numGroups, groupSize, groups);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_get_perf_monitor_counters_amd (GLuint group, GLint *numCounters, 
                                   GLint *maxActiveCounters, 
                                   GLsizei counterSize,
                                   GLuint *counters)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.GetPerfMonitorCountersAMD) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        
        dispatch.GetPerfMonitorCountersAMD (group, numCounters,
                                            maxActiveCounters,
                                            counterSize, counters);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_get_perf_monitor_group_string_amd (GLuint group, GLsizei bufSize, 
                                       GLsizei *length, 
                                       GLchar *groupString)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.GetPerfMonitorGroupStringAMD) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        
        dispatch.GetPerfMonitorGroupStringAMD (group, bufSize, length,
                                               groupString);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_get_perf_monitor_counter_string_amd (GLuint group, GLuint counter, 
                                         GLsizei bufSize, 
                                         GLsizei *length, 
                                         GLchar *counterString)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.GetPerfMonitorCounterStringAMD) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        
        dispatch.GetPerfMonitorCounterStringAMD (group, counter, bufSize, 
                                                 length, counterString);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_get_perf_monitor_counter_info_amd (GLuint group, GLuint counter, 
                                       GLenum pname, GLvoid *data)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.GetPerfMonitorCounterInfoAMD) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        
        dispatch.GetPerfMonitorCounterInfoAMD (group, counter, 
                                               pname, data); 
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_gen_perf_monitors_amd (GLsizei n, GLuint *monitors) 
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.GenPerfMonitorsAMD) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        
        dispatch.GenPerfMonitorsAMD (n, monitors); 
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_delete_perf_monitors_amd (GLsizei n, GLuint *monitors) 
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.DeletePerfMonitorsAMD) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        
        dispatch.DeletePerfMonitorsAMD (n, monitors); 
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_select_perf_monitor_counters_amd (GLuint monitor, GLboolean enable,
                                      GLuint group, GLint numCounters,
                                      GLuint *countersList) 
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.SelectPerfMonitorCountersAMD) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        
        dispatch.SelectPerfMonitorCountersAMD (monitor, enable, group,
                                               numCounters, countersList); 
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_begin_perf_monitor_amd (GLuint monitor)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.BeginPerfMonitorAMD) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        
        dispatch.BeginPerfMonitorAMD (monitor); 
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_end_perf_monitor_amd (GLuint monitor)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.EndPerfMonitorAMD) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        
        dispatch.EndPerfMonitorAMD (monitor); 
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_get_perf_monitor_counter_data_amd (GLuint monitor, GLenum pname,
                                       GLsizei dataSize, GLuint *data,
                                       GLint *bytesWritten)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.GetPerfMonitorCounterDataAMD) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        
        dispatch.GetPerfMonitorCounterDataAMD (monitor, pname, dataSize,
                                               data, bytesWritten); 
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_ANGLE_framebuffer_blit
static void
_gl_blit_framebuffer_angle (GLint srcX0, GLint srcY0, 
                            GLint srcX1, GLint srcY1,
                            GLint dstX0, GLint dstY0, 
                            GLint dstX1, GLint dstY1,
                            GLbitfield mask, GLenum filter)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.BlitFramebufferANGLE) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        
        dispatch.BlitFramebufferANGLE (srcX0, srcY0, srcX1, srcY1,
                                       dstX0, dstY0, dstX1, dstY1,
                                       mask, filter);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_ANGLE_framebuffer_multisample
static void
_gl_renderbuffer_storage_multisample_angle (GLenum target, GLsizei samples,
                                            GLenum internalformat,
                                            GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.RenderbufferStorageMultisampleANGLE) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        
        dispatch.RenderbufferStorageMultisampleANGLE (target, samples,
                                                      internalformat,
                                                      width, height);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_APPLE_framebuffer_multisample
static void
_gl_renderbuffer_storage_multisample_apple (GLenum target, GLsizei samples,
                                            GLenum internalformat, 
                                            GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.RenderbufferStorageMultisampleAPPLE) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        
        dispatch.RenderbufferStorageMultisampleAPPLE (target, samples,
                                                      internalformat,
                                                      width, height);
        egl_state->state.need_get_error = true;
    }
}

static void
gl_resolve_multisample_framebuffer_apple (void)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.ResolveMultisampleFramebufferAPPLE) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        
        dispatch.ResolveMultisampleFramebufferAPPLE ();
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_EXT_discard_framebuffer
static void
gl_discard_framebuffer_ext (GLenum target, GLsizei numAttachments,
                            const GLenum *attachments)
{
    egl_state_t *egl_state;
    int i;

    if (_gl_is_valid_func (dispatch.DiscardFramebufferEXT) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        
        if (target != GL_FRAMEBUFFER) {
            _gl_set_error (GL_INVALID_ENUM);
            goto FINISH;
        }

        for (i = 0; i < numAttachments; i++) {
            if (! (attachments[i] == GL_COLOR_ATTACHMENT0  ||
                   attachments[i] == GL_DEPTH_ATTACHMENT   ||
                   attachments[i] == GL_STENCIL_ATTACHMENT ||
                   attachments[i] == GL_COLOR_EXT          ||
                   attachments[i] == GL_DEPTH_EXT          ||
                   attachments[i] == GL_STENCIL_EXT)) {
                _gl_set_error (GL_INVALID_ENUM);
                goto FINISH;
            }
        }
        dispatch.DiscardFramebufferEXT (target, numAttachments, 
                                        attachments);
    }
FINISH:
    if (attachments)
        free ((void *)attachments);
}
#endif

#ifdef GL_EXT_multi_draw_arrays
static void
_gl_multi_draw_arrays_ext (GLenum mode, const GLint *first, 
                           const GLsizei *count, GLsizei primcount)
{
    /* not implemented */
}

void 
gl_multi_draw_elements_ext (GLenum mode, const GLsizei *count, GLenum type,
                            const GLvoid **indices, GLsizei primcount)
{
    /* not implemented */
}
#endif

#ifdef GL_EXT_multisampled_render_to_texture
static void
_gl_renderbuffer_storage_multisample_ext (GLenum target, GLsizei samples,
                                          GLenum internalformat,
                                          GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.RenderbufferStorageMultisampleEXT) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        
        dispatch.RenderbufferStorageMultisampleEXT (target, samples, 
                                                    internalformat, 
                                                    width, height);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_framebuffer_texture_2d_multisample_ext (GLenum target, 
                                            GLenum attachment,
                                            GLenum textarget, 
                                            GLuint texture,
                                            GLint level, GLsizei samples)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.FramebufferTexture2DMultisampleEXT) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        
        if (target != GL_FRAMEBUFFER) {
           _gl_set_error (GL_INVALID_ENUM);
           return;
        }

        dispatch.FramebufferTexture2DMultisampleEXT (target, attachment, 
                                                     textarget, texture, 
                                                     level, samples);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_IMG_multisampled_render_to_texture
static void
_gl_renderbuffer_storage_multisample_img (GLenum target, GLsizei samples,
                                          GLenum internalformat,
                                          GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.RenderbufferStorageMultisampleIMG) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        
        dispatch.RenderbufferStorageMultisampleIMG (target, samples, 
                                                    internalformat, 
                                                    width, height);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_framebuffer_texture_2d_multisample_img (GLenum target, 
                                            GLenum attachment,
                                            GLenum textarget, 
                                            GLuint texture,
                                            GLint level, GLsizei samples)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.FramebufferTexture2DMultisampleIMG) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
        
        if (target != GL_FRAMEBUFFER) {
           _gl_set_error (GL_INVALID_ENUM);
           return;
        }

        dispatch.FramebufferTexture2DMultisampleIMG (target, attachment,
                                                     textarget, texture,
                                                     level, samples);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_NV_fence
static void
_gl_delete_fences_nv (GLsizei n, const GLuint *fences)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.DeleteFencesNV) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (n <= 0) {
            _gl_set_error (GL_INVALID_VALUE);
            goto FINISH;
        }
    
        dispatch.DeleteFencesNV (n, fences); 
        egl_state->state.need_get_error = true;
    }
FINISH:
    if (fences)
        free ((void *)fences);
}

static void
_gl_gen_fences_nv (GLsizei n, GLuint *fences)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.GenFencesNV) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (n <= 0) {
            _gl_set_error (GL_INVALID_VALUE);
            return;
        }
    
        dispatch.GenFencesNV (n, fences); 
        egl_state->state.need_get_error = true;
    }
}

static GLboolean
_gl_is_fence_nv (GLuint fence)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (dispatch.IsFenceNV) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        result = dispatch.IsFenceNV (fence);
        egl_state->state.need_get_error = true;
    }
    return result;
}

static GLboolean
_gl_test_fence_nv (GLuint fence)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (dispatch.TestFenceNV) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        result = dispatch.TestFenceNV (fence);
        egl_state->state.need_get_error = true;
    }
    return result;
}

static void 
_gl_get_fenceiv_nv (GLuint fence, GLenum pname, int *params)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.GetFenceivNV) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.GetFenceivNV (fence, pname, params);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_finish_fence_nv (GLuint fence)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.FinishFenceNV) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.FinishFenceNV (fence);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_set_fence_nv (GLuint fence, GLenum condition)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.SetFenceNV) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.SetFenceNV (fence, condition);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_NV_coverage_sample
static void
_gl_coverage_mask_nv (GLboolean mask)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.CoverageMaskNV) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.CoverageMaskNV (mask);
    }
}

static void
_gl_coverage_operation_nv (GLenum operation)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.CoverageOperationNV) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (! (operation == GL_COVERAGE_ALL_FRAGMENTS_NV  ||
               operation == GL_COVERAGE_EDGE_FRAGMENTS_NV ||
               operation == GL_COVERAGE_AUTOMATIC_NV)) {
            _gl_set_error (GL_INVALID_ENUM);
            return;
        }

        dispatch.CoverageOperationNV (operation);
    }
}
#endif

#ifdef GL_QCOM_driver_control
static void
_gl_get_driver_controls_qcom (GLint *num, GLsizei size, 
                              GLuint *driverControls)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.GetDriverControlsQCOM) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.GetDriverControlsQCOM (num, size, driverControls);
    }
}

static void
_gl_get_driver_control_string_qcom (GLuint driverControl, GLsizei bufSize,
                                    GLsizei *length, 
                                    GLchar *driverControlString)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.GetDriverControlStringQCOM) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.GetDriverControlStringQCOM (driverControl, bufSize,
                                             length, driverControlString);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_enable_driver_control_qcom (GLuint driverControl)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.EnableDriverControlQCOM) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.EnableDriverControlQCOM (driverControl);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_disable_driver_control_qcom (GLuint driverControl)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.DisableDriverControlQCOM) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.DisableDriverControlQCOM (driverControl);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_QCOM_extended_get
static void
_gl_ext_get_textures_qcom (GLuint *textures, GLint maxTextures, 
                           GLint *numTextures)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.ExtGetTexturesQCOM) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.ExtGetTexturesQCOM (textures, maxTextures, numTextures);
    }
}

static void
_gl_ext_get_buffers_qcom (GLuint *buffers, GLint maxBuffers, 
                          GLint *numBuffers) 
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.ExtGetBuffersQCOM) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.ExtGetBuffersQCOM (buffers, maxBuffers, numBuffers);
    }
}

static void
_gl_ext_get_renderbuffers_qcom (GLuint *renderbuffers, 
                                GLint maxRenderbuffers,
                                GLint *numRenderbuffers)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.ExtGetRenderbuffersQCOM) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.ExtGetRenderbuffersQCOM (renderbuffers, maxRenderbuffers,
                                          numRenderbuffers);
    }
}

static void
_gl_ext_get_framebuffers_qcom (GLuint *framebuffers, GLint maxFramebuffers,
                               GLint *numFramebuffers)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.ExtGetFramebuffersQCOM) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.ExtGetFramebuffersQCOM (framebuffers, maxFramebuffers,
                                         numFramebuffers);
    }
}

static void
_gl_ext_get_tex_level_parameteriv_qcom (GLuint texture, GLenum face, 
                                        GLint level, GLenum pname, 
                                        GLint *params)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.ExtGetTexLevelParameterivQCOM) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.ExtGetTexLevelParameterivQCOM (texture, face, level,
                                                pname, params);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_ext_tex_object_state_overridei_qcom (GLenum target, GLenum pname, 
                                         GLint param)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.ExtTexObjectStateOverrideiQCOM) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.ExtTexObjectStateOverrideiQCOM (target, pname, param);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_ext_get_tex_sub_image_qcom (GLenum target, GLint level,
                                GLint xoffset, GLint yoffset, 
                                GLint zoffset,
                                GLsizei width, GLsizei height, 
                                GLsizei depth,
                                GLenum format, GLenum type, GLvoid *texels)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.ExtGetTexSubImageQCOM) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.ExtGetTexSubImageQCOM (target, level,
                                        xoffset, yoffset, zoffset,
                                        width, height, depth,
                                        format, type, texels);
    egl_state->state.need_get_error = true;
    }
}

static void
_gl_ext_get_buffer_pointerv_qcom (GLenum target, GLvoid **params)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.ExtGetBufferPointervQCOM) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.ExtGetBufferPointervQCOM (target, params);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_QCOM_extended_get2
static void
_gl_ext_get_shaders_qcom (GLuint *shaders, GLint maxShaders, 
                          GLint *numShaders)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.ExtGetShadersQCOM) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.ExtGetShadersQCOM (shaders, maxShaders, numShaders);
    }
}

static void
_gl_ext_get_programs_qcom (GLuint *programs, GLint maxPrograms,
                           GLint *numPrograms)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.ExtGetProgramsQCOM) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.ExtGetProgramsQCOM (programs, maxPrograms, numPrograms);
    }
}

static GLboolean
_gl_ext_is_program_binary_qcom (GLuint program)
{
    egl_state_t *egl_state;
    bool result = false;

    if (_gl_is_valid_func (dispatch.ExtIsProgramBinaryQCOM) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        result = dispatch.ExtIsProgramBinaryQCOM (program);
        egl_state->state.need_get_error = true;
    }
    return result;
}

static void
_gl_ext_get_program_binary_source_qcom (GLuint program, GLenum shadertype,
                                        GLchar *source, GLint *length)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.ExtGetProgramBinarySourceQCOM) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.ExtGetProgramBinarySourceQCOM (program, shadertype,
                                                source, length);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_QCOM_tiled_rendering
static void
_gl_start_tiling_qcom (GLuint x, GLuint y, GLuint width, GLuint height,
                       GLbitfield preserveMask)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.StartTilingQCOM) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.StartTilingQCOM (x, y, width, height, preserveMask);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_end_tiling_qcom (GLbitfield preserveMask)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (dispatch.EndTilingQCOM) &&
        _gl_is_valid_context ()) {
        egl_state = (egl_state_t *) active_state->data;
    
        dispatch.EndTilingQCOM (preserveMask);
        egl_state->state.need_get_error = true;
    }
}
#endif
