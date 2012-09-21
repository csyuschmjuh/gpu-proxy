/* This file implements the server thread side of GLES2 functions.  Thses
 * functions are called by the command buffer.
 *
 * It references to the server thread side of active_state.
 * if active_state is NULL or there is no symbol for the corresponding
 * gl functions, the cached error state is set to GL_INVALID_OPERATION
 * if the cached error has not been set to one of the errors.
 */
#include "config.h"
#include "command_buffer_server.h"

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
extern command_buffer_server_t *command_buffer_server;

exposed_to_tests inline bool
_gl_is_valid_func (command_buffer_server_t *server, void *func)
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

exposed_to_tests inline bool
_gl_is_valid_context (command_buffer_server_t *server)
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

exposed_to_tests inline void
_gl_set_error (command_buffer_server_t *server, GLenum error)
{
    egl_state_t *egl_state;

    if (active_state) {
        egl_state = (egl_state_t *) active_state->data;
 
        if (egl_state->active && egl_state->state.error == GL_NO_ERROR)
            egl_state->state.error = error;
    }
}

/* GLES2 core profile API */
exposed_to_tests void _gl_active_texture (command_buffer_server_t *server, GLenum texture)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glActiveTexture) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (egl_state->state.active_texture == texture)
            return;
        else if (texture > GL_TEXTURE31 || texture < GL_TEXTURE0) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }
        else {
            command_buffer_server->dispatch.glActiveTexture (command_buffer_server, texture);
            /* FIXME: this maybe not right because this texture may be 
             * invalid object, we save here to save time in glGetError() 
             */
            egl_state->state.active_texture = texture;
        }
    }
}

exposed_to_tests void _gl_attach_shader (command_buffer_server_t *server, GLuint program, GLuint shader)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glAttachShader) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glAttachShader (command_buffer_server, program, shader);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_bind_attrib_location (command_buffer_server_t *server, GLuint program, GLuint index, const GLchar *name)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glBindAttribLocation) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glBindAttribLocation (command_buffer_server, program, index, name);
        egl_state->state.need_get_error = true;
    }
    if (name)
        free ((char *)name);
}

exposed_to_tests void _gl_bind_buffer (command_buffer_server_t *server, GLenum target, GLuint buffer)
{
    egl_state_t *egl_state;
    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int count;
    int i;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glBindBuffer) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        attrib_list = &egl_state->state.vertex_attribs;
        vertex_attrib_t *attribs = attrib_list->attribs;
        count = attrib_list->count;

        if (target == GL_ARRAY_BUFFER) {
            if (egl_state->state.array_buffer_binding == buffer)
                return;
            else {
                command_buffer_server->dispatch.glBindBuffer (command_buffer_server, target, buffer);
                egl_state->state.need_get_error = true;

               /* FIXME: we don't know whether it succeeds or not */
               egl_state->state.array_buffer_binding = buffer;
            }
        }
        else if (target == GL_ELEMENT_ARRAY_BUFFER) {
            if (egl_state->state.element_array_buffer_binding == buffer)
                return;
            else {
                command_buffer_server->dispatch.glBindBuffer (command_buffer_server, target, buffer);
                egl_state->state.need_get_error = true;

               /* FIXME: we don't know whether it succeeds or not */
               egl_state->state.element_array_buffer_binding = buffer;
            }
        }
        else {
            _gl_set_error (server, GL_INVALID_ENUM);
        }
    }
}

exposed_to_tests void _gl_bind_framebuffer (command_buffer_server_t *server, GLenum target, GLuint framebuffer)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glBindFramebuffer) &&
        _gl_is_valid_context (server)) {
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
            _gl_set_error (server, GL_INVALID_ENUM);
        }

        command_buffer_server->dispatch.glBindFramebuffer (command_buffer_server, target, framebuffer);
        /* FIXME: should we save it, it will be invalid if the
         * framebuffer is invalid 
         */
        egl_state->state.framebuffer_binding = framebuffer;

        /* egl_state->state.need_get_error = true; */
    }
}

exposed_to_tests void _gl_bind_renderbuffer (command_buffer_server_t *server, GLenum target, GLuint renderbuffer)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glBindRenderbuffer) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (target != GL_RENDERBUFFER) {
            _gl_set_error (server, GL_INVALID_ENUM);
        }

        command_buffer_server->dispatch.glBindRenderbuffer (command_buffer_server, target, renderbuffer);
        /* egl_state->state.need_get_error = true; */
    }
}

exposed_to_tests void _gl_bind_texture (command_buffer_server_t *server, GLenum target, GLuint texture)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glBindTexture) &&
        _gl_is_valid_context (server)) {
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
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        command_buffer_server->dispatch.glBindTexture (command_buffer_server, target, texture);
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

exposed_to_tests void _gl_blend_color (command_buffer_server_t *server, GLclampf red, GLclampf green,
                             GLclampf blue, GLclampf alpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glBlendColor) &&
        _gl_is_valid_context (server)) {
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

        command_buffer_server->dispatch.glBlendColor (command_buffer_server, red, green, blue, alpha);
    }
}

exposed_to_tests void _gl_blend_equation (command_buffer_server_t *server, GLenum mode)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glBlendEquation) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        state = &egl_state->state;
    
        if (state->blend_equation[0] == mode &&
            state->blend_equation[1] == mode)
            return;

        if (! (mode == GL_FUNC_ADD ||
               mode == GL_FUNC_SUBTRACT ||
               mode == GL_FUNC_REVERSE_SUBTRACT)) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        state->blend_equation[0] = mode;
        state->blend_equation[1] = mode;

        command_buffer_server->dispatch.glBlendEquation (command_buffer_server, mode);
    }
}

exposed_to_tests void _gl_blend_equation_separate (command_buffer_server_t *server, GLenum modeRGB, GLenum modeAlpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glBlendEquationSeparate) &&
        _gl_is_valid_context (server)) {
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
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        state->blend_equation[0] = modeRGB;
        state->blend_equation[1] = modeAlpha;

        command_buffer_server->dispatch.glBlendEquationSeparate (command_buffer_server, modeRGB, modeAlpha);
    }
}

exposed_to_tests void _gl_blend_func (command_buffer_server_t *server, GLenum sfactor, GLenum dfactor)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glBlendFunc) &&
        _gl_is_valid_context (server)) {
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
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        state->blend_src[0] = state->blend_src[1] = sfactor;
        state->blend_dst[0] = state->blend_dst[1] = dfactor;

        command_buffer_server->dispatch.glBlendFunc (command_buffer_server, sfactor, dfactor);
    }
}

exposed_to_tests void _gl_blend_func_separate (command_buffer_server_t *server, GLenum srcRGB, GLenum dstRGB,
                                     GLenum srcAlpha, GLenum dstAlpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glBlendFuncSeparate) &&
        _gl_is_valid_context (server)) {
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
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        state->blend_src[0] = srcRGB;
        state->blend_src[1] = srcAlpha;
        state->blend_dst[0] = dstRGB;
        state->blend_dst[0] = dstAlpha;

        command_buffer_server->dispatch.glBlendFuncSeparate (command_buffer_server, srcRGB, dstRGB, srcAlpha, dstAlpha);
    }
}

exposed_to_tests void _gl_buffer_data (command_buffer_server_t *server, GLenum target, GLsizeiptr size,
                             const GLvoid *data, GLenum usage)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glBufferData) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        /* XXX: we skip rest of check, because driver
         * can generate GL_OUT_OF_MEMORY, which cannot check
         */
        command_buffer_server->dispatch.glBufferData (command_buffer_server, target, size, data, usage);
        egl_state = (egl_state_t *) active_state->data;
        egl_state->state.need_get_error = true;
    }

    if (data)
        free ((void *)data);
}

exposed_to_tests void _gl_buffer_sub_data (command_buffer_server_t *server, GLenum target, GLintptr offset,
                                 GLsizeiptr size, const GLvoid *data)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glBufferSubData) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        /* XXX: we skip rest of check, because driver
         * can generate GL_INVALID_VALUE, when offset + data can be out of
         * bound
         */
        command_buffer_server->dispatch.glBufferSubData (command_buffer_server, target, offset, size, data);
        egl_state->state.need_get_error = true;
    }

    if (data)
        free ((void *)data);
}

exposed_to_tests GLenum _gl_check_framebuffer_status (command_buffer_server_t *server, GLenum target)
{
    GLenum result = GL_INVALID_ENUM;
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glCheckFramebufferStatus) &&
        _gl_is_valid_context (server)) {
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
            _gl_set_error (server, GL_INVALID_ENUM);
            return result;
        }

        return command_buffer_server->dispatch.glCheckFramebufferStatus (command_buffer_server, target);
    }

    return result;
}

exposed_to_tests void _gl_clear (command_buffer_server_t *server, GLbitfield mask)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glClear) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (! (mask & GL_COLOR_BUFFER_BIT ||
               mask & GL_DEPTH_BUFFER_BIT ||
               mask & GL_STENCIL_BUFFER_BIT)) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        command_buffer_server->dispatch.glClear (command_buffer_server, mask);
    }
}

exposed_to_tests void _gl_clear_color (command_buffer_server_t *server, GLclampf red, GLclampf green,
                             GLclampf blue, GLclampf alpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glClearColor) &&
        _gl_is_valid_context (server)) {
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

        command_buffer_server->dispatch.glClearColor (command_buffer_server, red, green, blue, alpha);
    }
}

exposed_to_tests void _gl_clear_depthf (command_buffer_server_t *server, GLclampf depth)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glClearDepthf) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        state = &egl_state->state;

        if (state->depth_clear_value == depth)
            return;

        state->depth_clear_value = depth;

        command_buffer_server->dispatch.glClearDepthf (command_buffer_server, depth);
    }
}

exposed_to_tests void _gl_clear_stencil (command_buffer_server_t *server, GLint s)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glClearStencil) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        state = &egl_state->state;

        if (state->stencil_clear_value == s)
            return;

        state->stencil_clear_value = s;

        command_buffer_server->dispatch.glClearStencil (command_buffer_server, s);
    }
}

exposed_to_tests void _gl_color_mask (command_buffer_server_t *server, GLboolean red, GLboolean green,
                            GLboolean blue, GLboolean alpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glColorMask) &&
        _gl_is_valid_context (server)) {
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

        command_buffer_server->dispatch.glColorMask (command_buffer_server, red, green, blue, alpha);
    }
}

exposed_to_tests void _gl_compressed_tex_image_2d (command_buffer_server_t *server, GLenum target, GLint level,
                                         GLenum internalformat,
                                         GLsizei width, GLsizei height,
                                         GLint border, GLsizei imageSize,
                                         const GLvoid *data)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glCompressedTexImage2D) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glCompressedTexImage2D (command_buffer_server, target, level, internalformat,
                                       width, height, border, imageSize,
                                       data);

        egl_state->state.need_get_error = true;
    }

    if (data)
        free ((void *)data);
}

exposed_to_tests void _gl_compressed_tex_sub_image_2d (command_buffer_server_t *server, GLenum target, GLint level,
                                             GLint xoffset, GLint yoffset,
                                             GLsizei width, GLsizei height,
                                             GLenum format, 
                                             GLsizei imageSize,
                                             const GLvoid *data)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glCompressedTexSubImage2D) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glCompressedTexSubImage2D (command_buffer_server, target, level, xoffset, yoffset,
                                          width, height, format, imageSize,
                                          data);

        egl_state->state.need_get_error = true;
    }

    if (data)
        free ((void *)data);
}

exposed_to_tests void _gl_copy_tex_image_2d (command_buffer_server_t *server, GLenum target, GLint level,
                                   GLenum internalformat,
                                   GLint x, GLint y,
                                   GLsizei width, GLsizei height,
                                   GLint border)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glCopyTexImage2D) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glCopyTexImage2D (command_buffer_server, target, level, internalformat,
                                 x, y, width, height, border);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_copy_tex_sub_image_2d (command_buffer_server_t *server, GLenum target, GLint level,
                                       GLint xoffset, GLint yoffset,
                                       GLint x, GLint y,
                                       GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glCopyTexSubImage2D) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glCopyTexSubImage2D (command_buffer_server, target, level, xoffset, yoffset,
                                    x, y, width, height);

        egl_state->state.need_get_error = true;
    }
}

/* This is a sync call */
exposed_to_tests GLuint _gl_create_program  (command_buffer_server_t *server)
{
    GLuint result = 0;
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glCreateProgram) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        result = command_buffer_server->dispatch.glCreateProgram (command_buffer_server);
    }

    return result;
}

/* sync call */
exposed_to_tests GLuint _gl_create_shader (command_buffer_server_t *server, GLenum shaderType)
{
    GLuint result = 0;
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glCreateShader) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (! (shaderType == GL_VERTEX_SHADER ||
               shaderType == GL_FRAGMENT_SHADER)) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return result;
        }

        result = command_buffer_server->dispatch.glCreateShader (command_buffer_server, shaderType);
    }

    return result;
}

exposed_to_tests void _gl_cull_face (command_buffer_server_t *server, GLenum mode)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glCullFace) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (egl_state->state.cull_face_mode == mode)
            return;

        if (! (mode == GL_FRONT ||
               mode == GL_BACK ||
               mode == GL_FRONT_AND_BACK)) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        egl_state->state.cull_face_mode = mode;

        command_buffer_server->dispatch.glCullFace (command_buffer_server, mode);
    }
}

exposed_to_tests void _gl_delete_buffers (command_buffer_server_t *server, GLsizei n, const GLuint *buffers)
{
    egl_state_t *egl_state;
    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int count;
    int i, j;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glDeleteBuffers) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        attrib_list = &egl_state->state.vertex_attribs;
        vertex_attrib_t *attribs = attrib_list->attribs;
        count = attrib_list->count;

        if (n < 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            goto FINISH;
        }

        command_buffer_server->dispatch.glDeleteBuffers (command_buffer_server, n, buffers);

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

exposed_to_tests void _gl_delete_framebuffers (command_buffer_server_t *server, GLsizei n, const GLuint *framebuffers)
{
    egl_state_t *egl_state;
    int i;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glDeleteFramebuffers) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (n < 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            goto FINISH;
        }

        command_buffer_server->dispatch.glDeleteFramebuffers (command_buffer_server, n, framebuffers);

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

exposed_to_tests void _gl_delete_program (command_buffer_server_t *server, GLuint program)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glDeleteProgram) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glDeleteProgram (command_buffer_server, program);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_delete_renderbuffers (command_buffer_server_t *server, GLsizei n, const GLuint *renderbuffers)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glDeleteRenderbuffers) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (n < 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            goto FINISH;
        }

        command_buffer_server->dispatch.glDeleteRenderbuffers (command_buffer_server, n, renderbuffers);
    }

FINISH:
    if (renderbuffers)
        free ((void *)renderbuffers);
}

exposed_to_tests void _gl_delete_shader (command_buffer_server_t *server, GLuint shader)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glDeleteShader) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glDeleteShader (command_buffer_server, shader);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_delete_textures (command_buffer_server_t *server, GLsizei n, const GLuint *textures)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glDeleteTextures) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (n < 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            goto FINISH;
        }

        command_buffer_server->dispatch.glDeleteTextures (command_buffer_server, n, textures);
    }

FINISH:
    if (textures)
        free ((void *)textures);
}

exposed_to_tests void _gl_depth_func (command_buffer_server_t *server, GLenum func)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glDepthFunc) &&
        _gl_is_valid_context (server)) {
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
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        egl_state->state.depth_func = func;

        command_buffer_server->dispatch.glDepthFunc (command_buffer_server, func);
    }
}

exposed_to_tests void _gl_depth_mask (command_buffer_server_t *server, GLboolean flag)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glDepthMask) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (egl_state->state.depth_writemask == flag)
            return;

        egl_state->state.depth_writemask = flag;

        command_buffer_server->dispatch.glDepthMask (command_buffer_server, flag);
    }
}

exposed_to_tests void _gl_depth_rangef (command_buffer_server_t *server, GLclampf nearVal, GLclampf farVal)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glDepthRangef) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (egl_state->state.depth_range[0] == nearVal &&
            egl_state->state.depth_range[1] == farVal)
            return;

        egl_state->state.depth_range[0] = nearVal;
        egl_state->state.depth_range[1] = farVal;

        command_buffer_server->dispatch.glDepthRangef (command_buffer_server, nearVal, farVal);
    }
}

exposed_to_tests void _gl_detach_shader (command_buffer_server_t *server, GLuint program, GLuint shader)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glDetachShader) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        /* XXX: command buffer, error code generated */
        command_buffer_server->dispatch.glDetachShader (command_buffer_server, program, shader);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void
_gl_set_cap (command_buffer_server_t *server, GLenum cap, GLboolean enable)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    bool needs_call = false;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glDisable) &&
        _gl_is_valid_func (server, command_buffer_server->dispatch.glEnable) &&
        _gl_is_valid_context (server)) {
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
                command_buffer_server->dispatch.glEnable (command_buffer_server, cap);
            else
                command_buffer_server->dispatch.glDisable (command_buffer_server, cap);
        }
    }
}

exposed_to_tests void _gl_disable (command_buffer_server_t *server, GLenum cap)
{
    _gl_set_cap (server, cap, GL_FALSE);
}

exposed_to_tests void _gl_enable (command_buffer_server_t *server, GLenum cap)
{
    _gl_set_cap (server, cap, GL_TRUE);
}

exposed_to_tests bool
_gl_index_is_too_large (command_buffer_server_t *server, gles2_state_t *state, GLuint index)
{
    if (index >= state->max_vertex_attribs) {
        if (! state->max_vertex_attribs_queried) {
            /* XXX: command buffer */
            command_buffer_server->dispatch.glGetIntegerv (command_buffer_server, GL_MAX_VERTEX_ATTRIBS,
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

exposed_to_tests void 
_gl_set_vertex_attrib_array (command_buffer_server_t *server, GLuint index, gles2_state_t *state, 
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
                attribs[i].array_enabled = enable;
                break;
            }
        }
    }
        
    /* gles2 spec says at least 8 */
    if (_gl_index_is_too_large (server, state, index)) {
        return;
    }

    if (enable == GL_FALSE)
        command_buffer_server->dispatch.glDisableVertexAttribArray (command_buffer_server, index);
    else
        command_buffer_server->dispatch.glEnableVertexAttribArray (command_buffer_server, index);

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

exposed_to_tests void _gl_disable_vertex_attrib_array (command_buffer_server_t *server, GLuint index)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glDisableVertexAttribArray) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        state = &egl_state->state;

        _gl_set_vertex_attrib_array (server, index, state, GL_FALSE);
    }
}

exposed_to_tests void _gl_enable_vertex_attrib_array (command_buffer_server_t *server, GLuint index)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glEnableVertexAttribArray) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        state = &egl_state->state;

        _gl_set_vertex_attrib_array (server, index, state, GL_TRUE);
    }
}

/* FIXME: we should use pre-allocated buffer if possible */
exposed_to_tests char *
_gl_create_data_array (command_buffer_server_t *server, vertex_attrib_t *attrib, int count)
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

exposed_to_tests void _gl_draw_arrays (command_buffer_server_t *server, GLenum mode, GLint first, GLsizei count)
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

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glDrawArrays) &&
        _gl_is_valid_context (server)) {
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
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }
        else if (count < 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            return;
        }

#ifdef GL_OES_vertex_array_object
        /* if vertex array binding is not 0 */
        if (state->vertex_array_binding) {
            command_buffer_server->dispatch.glDrawArrays (command_buffer_server, mode, first, count);
            state->need_get_error = true;
            return;
        } else
#endif
  
        for (i = 0; i < attrib_list->count; i++) {
            if (! attribs[i].array_enabled)
                continue;
            /* we need to create a separate buffer for it */
            else if (! attribs[i].array_buffer_binding) {
                data = _gl_create_data_array (server, &attribs[i], count);
                if (! data)
                    continue;
                command_buffer_server->dispatch.glVertexAttribPointer (command_buffer_server, attribs[i].index,
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
        command_buffer_server->dispatch.glDrawArrays (command_buffer_server, mode, first, count);

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
_gl_create_indices_array (command_buffer_server_t *server, GLenum mode, GLenum type, int count,
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

exposed_to_tests void _gl_draw_elements (command_buffer_server_t *server, GLenum mode, GLsizei count, GLenum type,
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

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glDrawElements) &&
        _gl_is_valid_context (server)) {
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
            _gl_set_error (server, GL_INVALID_ENUM);
            goto FINISH;
        }
        else if (! (type == GL_UNSIGNED_BYTE  || 
                    type == GL_UNSIGNED_SHORT)) {
            _gl_set_error (server, GL_INVALID_ENUM);
            goto FINISH;
        }
        else if (count < 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            goto FINISH;
        }

#ifdef GL_OES_vertex_array_object
        if (state->vertex_array_binding) {
            command_buffer_server->dispatch.glDrawElements (command_buffer_server, mode, count, type, indices);
            state->need_get_error = true;
            goto FINISH;
        } else
#endif

        for (i = 0; i < attrib_list->count; i++) {
            if (! attribs[i].array_enabled)
                continue;
            /* we need to create a separate buffer for it */
            else if (! attribs[i].array_buffer_binding) {
                data = _gl_create_data_array (server, &attribs[i], count);
                if (! data)
                    continue;
                command_buffer_server->dispatch.glVertexAttribPointer (command_buffer_server, attribs[i].index,
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

        command_buffer_server->dispatch.glDrawElements (command_buffer_server, mode, type, count, indices);

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

exposed_to_tests void _gl_finish (command_buffer_server_t *server)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glFinish) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glFinish (command_buffer_server);
    }
}

exposed_to_tests void _gl_flush (command_buffer_server_t *server)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glFlush) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glFlush (command_buffer_server);
    }
}

exposed_to_tests void _gl_framebuffer_renderbuffer (command_buffer_server_t *server, GLenum target, GLenum attachment,
                                          GLenum renderbuffertarget,
                                          GLenum renderbuffer)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glFramebufferRenderbuffer) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (target != GL_FRAMEBUFFER) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }
        else if (renderbuffertarget != GL_RENDERBUFFER &&
                 renderbuffer != 0) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        command_buffer_server->dispatch.glFramebufferRenderbuffer (command_buffer_server, target, attachment,
                                          renderbuffertarget, 
                                          renderbuffer);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_framebuffer_texture_2d (command_buffer_server_t *server, GLenum target, GLenum attachment,
                                        GLenum textarget, GLuint texture, 
                                        GLint level)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glFramebufferTexture2D) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (target != GL_FRAMEBUFFER) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        command_buffer_server->dispatch.glFramebufferTexture2D (command_buffer_server, target, attachment, textarget,
                                       texture, level);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_front_face (command_buffer_server_t *server, GLenum mode)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glFramebufferTexture2D) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (egl_state->state.front_face == mode)
            return;

        if (! (mode == GL_CCW || mode == GL_CW)) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        egl_state->state.front_face = mode;
        command_buffer_server->dispatch.glFrontFace (command_buffer_server, mode);
    }
}

exposed_to_tests void _gl_gen_buffers (command_buffer_server_t *server, GLsizei n, GLuint *buffers)
{
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGenBuffers) &&
        _gl_is_valid_context (server)) {

        if (n < 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            return;
        }
    
        command_buffer_server->dispatch.glGenBuffers (command_buffer_server, n, buffers);
    }
}

exposed_to_tests void _gl_gen_framebuffers (command_buffer_server_t *server, GLsizei n, GLuint *framebuffers)
{
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGenFramebuffers) &&
        _gl_is_valid_context (server)) {

        if (n < 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            return;
        }
        
        command_buffer_server->dispatch.glGenFramebuffers (command_buffer_server, n, framebuffers);
    }
}

exposed_to_tests void _gl_gen_renderbuffers (command_buffer_server_t *server, GLsizei n, GLuint *renderbuffers)
{
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGenRenderbuffers) &&
        _gl_is_valid_context (server)) {

        if (n < 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            return;
        }
        
        command_buffer_server->dispatch.glGenRenderbuffers (command_buffer_server, n, renderbuffers);
    }
}

exposed_to_tests void _gl_gen_textures (command_buffer_server_t *server, GLsizei n, GLuint *textures)
{
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGenTextures) &&
        _gl_is_valid_context (server)) {

        if (n < 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            return;
        }
        
        command_buffer_server->dispatch.glGenTextures (command_buffer_server, n, textures);
    }
}

exposed_to_tests void _gl_generate_mipmap (command_buffer_server_t *server, GLenum target)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGenerateMipmap) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (! (target == GL_TEXTURE_2D       || 
               target == GL_TEXTURE_CUBE_MAP
#ifdef GL_OES_texture_3D
                                             || 
               target == GL_TEXTURE_3D_OES
#endif
                                          )) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        command_buffer_server->dispatch.glGenerateMipmap (command_buffer_server, target);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_get_booleanv (command_buffer_server_t *server, GLenum pname, GLboolean *params)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetBooleanv) &&
        _gl_is_valid_context (server)) {
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
            command_buffer_server->dispatch.glGetBooleanv (command_buffer_server, pname, params);
            break;
        }
    }
}

exposed_to_tests void _gl_get_floatv (command_buffer_server_t *server, GLenum pname, GLfloat *params)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetFloatv) &&
        _gl_is_valid_context (server)) {
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
            command_buffer_server->dispatch.glGetFloatv (command_buffer_server, pname, params);
            break;
        }
    }
}

exposed_to_tests void _gl_get_integerv (command_buffer_server_t *server, GLenum pname, GLint *params)
{
    egl_state_t *egl_state;
    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int count;
    int i;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetIntegerv) &&
        _gl_is_valid_context (server)) {
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
                command_buffer_server->dispatch.glGetIntegerv (command_buffer_server, pname, params);
                egl_state->state.max_combined_texture_image_units_queried = true;
                egl_state->state.max_combined_texture_image_units = *params;
            } else
                *params = egl_state->state.max_combined_texture_image_units;
            break;
        case GL_MAX_CUBE_MAP_TEXTURE_SIZE:
            if (! egl_state->state.max_cube_map_texture_size_queried) {
                command_buffer_server->dispatch.glGetIntegerv (command_buffer_server, pname, params);
                egl_state->state.max_cube_map_texture_size = *params;
                egl_state->state.max_cube_map_texture_size_queried = true;
            } else
                *params = egl_state->state.max_cube_map_texture_size;
        break;
        case GL_MAX_FRAGMENT_UNIFORM_VECTORS:
            if (! egl_state->state.max_fragment_uniform_vectors_queried) {
                command_buffer_server->dispatch.glGetIntegerv (command_buffer_server, pname, params);
                egl_state->state.max_fragment_uniform_vectors = *params;
                egl_state->state.max_fragment_uniform_vectors_queried = true;
            } else
                *params = egl_state->state.max_fragment_uniform_vectors;
            break;
        case GL_MAX_RENDERBUFFER_SIZE:
            if (! egl_state->state.max_renderbuffer_size_queried) {
                command_buffer_server->dispatch.glGetIntegerv (command_buffer_server, pname, params);
                egl_state->state.max_renderbuffer_size = *params;
                egl_state->state.max_renderbuffer_size_queried = true;
            } else
                *params = egl_state->state.max_renderbuffer_size;
            break;
        case GL_MAX_TEXTURE_IMAGE_UNITS:
            if (! egl_state->state.max_texture_image_units_queried) {
                command_buffer_server->dispatch.glGetIntegerv (command_buffer_server, pname, params);
                egl_state->state.max_texture_image_units = *params;
                egl_state->state.max_texture_image_units_queried = true;
            } else
                *params = egl_state->state.max_texture_image_units;
            break;
        case GL_MAX_VARYING_VECTORS:
            if (! egl_state->state.max_varying_vectors_queried) {
                command_buffer_server->dispatch.glGetIntegerv (command_buffer_server, pname, params);
                egl_state->state.max_varying_vectors = *params;
                egl_state->state.max_varying_vectors_queried = true;
            } else
                *params = egl_state->state.max_varying_vectors;
            break;
        case GL_MAX_TEXTURE_SIZE:
            if (! egl_state->state.max_texture_size_queried) {
                command_buffer_server->dispatch.glGetIntegerv (command_buffer_server, pname, params);
                egl_state->state.max_texture_size = *params;
                egl_state->state.max_texture_size_queried = true;
            } else
                *params = egl_state->state.max_texture_size;
            break;
        case GL_MAX_VERTEX_ATTRIBS:
            if (! egl_state->state.max_vertex_attribs_queried) {
                command_buffer_server->dispatch.glGetIntegerv (command_buffer_server, pname, params);
                egl_state->state.max_vertex_attribs = *params;
                egl_state->state.max_vertex_attribs_queried = true;
            } else
                *params = egl_state->state.max_vertex_attribs;
            break;
        case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
            if (! egl_state->state.max_vertex_texture_image_units_queried) {
                command_buffer_server->dispatch.glGetIntegerv (command_buffer_server, pname, params);
                egl_state->state.max_vertex_texture_image_units = *params;
                egl_state->state.max_vertex_texture_image_units_queried = true;
            } else
                *params = egl_state->state.max_vertex_texture_image_units;
            break;
        case GL_MAX_VERTEX_UNIFORM_VECTORS:
            if (! egl_state->state.max_vertex_uniform_vectors_queried) {
                command_buffer_server->dispatch.glGetIntegerv (command_buffer_server, pname, params);
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
            command_buffer_server->dispatch.glGetIntegerv (command_buffer_server, pname, params);
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

exposed_to_tests void _gl_get_active_attrib (command_buffer_server_t *server, GLuint program, GLuint index,
                                   GLsizei bufsize, GLsizei *length,
                                   GLint *size, GLenum *type, GLchar *name)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetActiveAttrib) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glGetActiveAttrib (command_buffer_server, program, index, bufsize, length,
                                  size, type, name);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_get_active_uniform (command_buffer_server_t *server, GLuint program, GLuint index, 
                                    GLsizei bufsize, GLsizei *length, 
                                    GLint *size, GLenum *type, 
                                    GLchar *name)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetActiveUniform) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glGetActiveUniform (command_buffer_server, program, index, bufsize, length,
                                   size, type, name);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_get_attached_shaders (command_buffer_server_t *server, GLuint program, GLsizei maxCount,
                                      GLsizei *count, GLuint *shaders)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetAttachedShaders) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glGetAttachedShaders (command_buffer_server, program, maxCount, count, shaders);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests GLint _gl_get_attrib_location (command_buffer_server_t *server, GLuint program, const GLchar *name)
{
    egl_state_t *egl_state;
    GLint result = -1;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetAttribLocation) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        result = command_buffer_server->dispatch.glGetAttribLocation (command_buffer_server, program, name);

        egl_state->state.need_get_error = true;
    }
    return result;
}

exposed_to_tests void _gl_get_buffer_parameteriv (command_buffer_server_t *server, GLenum target, GLenum value, GLint *data)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetBufferParameteriv) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glGetBufferParameteriv (command_buffer_server, target, value, data);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests GLenum _gl_get_error (command_buffer_server_t *server)
{
    GLenum error = GL_INVALID_OPERATION;
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetError) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (! egl_state->state.need_get_error) {
            error = egl_state->state.error;
            egl_state->state.error = GL_NO_ERROR;
            return error;
        }

        error = command_buffer_server->dispatch.glGetError (command_buffer_server);

        egl_state->state.need_get_error = false;
        egl_state->state.error = GL_NO_ERROR;
    }

    return error;
}

exposed_to_tests void _gl_get_framebuffer_attachment_parameteriv (command_buffer_server_t *server, GLenum target,
                                                        GLenum attachment,
                                                        GLenum pname,
                                                        GLint *params)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetFramebufferAttachmentParameteriv) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (target != GL_FRAMEBUFFER) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        command_buffer_server->dispatch.glGetFramebufferAttachmentParameteriv (command_buffer_server, target, attachment,
                                                      pname, params);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_get_program_info_log (command_buffer_server_t *server, GLuint program, GLsizei maxLength,
                                      GLsizei *length, GLchar *infoLog)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetProgramInfoLog) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glGetProgramInfoLog (command_buffer_server, program, maxLength, length, infoLog);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_get_programiv (command_buffer_server_t *server, GLuint program, GLenum pname, GLint *params)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetProgramiv) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glGetProgramiv (command_buffer_server, program, pname, params);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_get_renderbuffer_parameteriv (command_buffer_server_t *server, GLenum target,
                                              GLenum pname,
                                              GLint *params)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetRenderbufferParameteriv) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (target != GL_RENDERBUFFER) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        command_buffer_server->dispatch.glGetRenderbufferParameteriv (command_buffer_server, target, pname, params);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_get_shader_info_log (command_buffer_server_t *server, GLuint program, GLsizei maxLength,
                                     GLsizei *length, GLchar *infoLog)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetShaderInfoLog) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glGetShaderInfoLog (command_buffer_server, program, maxLength, length, infoLog);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_get_shader_precision_format (command_buffer_server_t *server, GLenum shaderType, 
                                             GLenum precisionType,
                                             GLint *range, 
                                             GLint *precision)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetShaderPrecisionFormat) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glGetShaderPrecisionFormat (command_buffer_server, shaderType, precisionType,
                                           range, precision);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_get_shader_source  (command_buffer_server_t *server, GLuint shader, GLsizei bufSize, 
                                    GLsizei *length, GLchar *source)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetShaderSource) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glGetShaderSource (command_buffer_server, shader, bufSize, length, source);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_get_shaderiv (command_buffer_server_t *server, GLuint shader, GLenum pname, GLint *params)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetShaderiv) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glGetShaderiv (command_buffer_server, shader, pname, params);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests const GLubyte *_gl_get_string (command_buffer_server_t *server, GLenum name)
{
    GLubyte *result = NULL;
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetString) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (! (name == GL_VENDOR                   || 
               name == GL_RENDERER                 ||
               name == GL_SHADING_LANGUAGE_VERSION ||
               name == GL_EXTENSIONS)) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return NULL;
        }

        result = (GLubyte *)command_buffer_server->dispatch.glGetString (command_buffer_server, name);

        egl_state->state.need_get_error = true;
    }
    return (const GLubyte *)result;
}

exposed_to_tests void _gl_get_tex_parameteriv (command_buffer_server_t *server, GLenum target, GLenum pname, 
                                     GLint *params)
{
    egl_state_t *egl_state;
    int active_texture_index;
    int target_index;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetTexParameteriv) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (! (target == GL_TEXTURE_2D       || 
               target == GL_TEXTURE_CUBE_MAP
#ifdef GL_OES_texture_3D
                                             || 
               target == GL_TEXTURE_3D_OES
#endif
                                          )) {
            _gl_set_error (server, GL_INVALID_ENUM);
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
            _gl_set_error (server, GL_INVALID_ENUM);
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

exposed_to_tests void _gl_get_tex_parameterfv (command_buffer_server_t *server, GLenum target, GLenum pname, GLfloat *params)
{
    GLint paramsi;

    _gl_get_tex_parameteriv (server, target, pname, &paramsi);
    *params = paramsi;
}

exposed_to_tests void _gl_get_uniformiv (command_buffer_server_t *server, GLuint program, GLint location, 
                               GLint *params)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetUniformiv) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glGetUniformiv (command_buffer_server, program, location, params);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_get_uniformfv (command_buffer_server_t *server, GLuint program, GLint location, 
                               GLfloat *params)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetUniformfv) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glGetUniformfv (command_buffer_server, program, location, params);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests GLint _gl_get_uniform_location (command_buffer_server_t *server, GLuint program, const GLchar *name)
{
    GLint result = -1;
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetUniformLocation) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        result = command_buffer_server->dispatch.glGetUniformLocation (command_buffer_server, program, name);

        egl_state->state.need_get_error = true;
    }
    return result;
}

exposed_to_tests void _gl_get_vertex_attribfv (command_buffer_server_t *server, GLuint index, GLenum pname, 
                                     GLfloat *params)
{
    egl_state_t *egl_state;

    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int count;
    int i, found_index = -1;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetVertexAttribfv) &&
        _gl_is_valid_context (server)) {
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
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        /* check index is too large */
        if (_gl_index_is_too_large (server, &egl_state->state, index)) {
            return;
        }

#ifdef GL_OES_vertex_array_object
        /* we cannot use client state */
        if (egl_state->state.vertex_array_binding) {
            command_buffer_server->dispatch.glGetVertexAttribfv (command_buffer_server, index, pname, params);
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

exposed_to_tests void _gl_get_vertex_attribiv (command_buffer_server_t *server, GLuint index, GLenum pname, GLint *params)
{
    GLfloat paramsf[4];
    int i;

    command_buffer_server->dispatch.glGetVertexAttribfv (command_buffer_server, index, pname, paramsf);

    if (pname == GL_CURRENT_VERTEX_ATTRIB) {
        for (i = 0; i < 4; i++)
            params[i] = paramsf[i];
    } else
        *params = paramsf[0];
}

exposed_to_tests void _gl_get_vertex_attrib_pointerv (command_buffer_server_t *server, GLuint index, GLenum pname, 
                                            GLvoid **pointer)
{
    egl_state_t *egl_state;

    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int count;
    int i, found_index = -1;
    
    *pointer = 0;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetVertexAttribPointerv) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        attrib_list = &egl_state->state.vertex_attribs;
        attribs = attrib_list->attribs;
        count = attrib_list->count;

        if (pname != GL_VERTEX_ATTRIB_ARRAY_POINTER) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        /* XXX: check index validity */
        if (_gl_index_is_too_large (server, &egl_state->state, index)) {
            return;
        }

#ifdef GL_OES_vertex_array_object
        /* we cannot use client state */
        if (egl_state->state.vertex_array_binding) {
            command_buffer_server->dispatch.glGetVertexAttribPointerv (command_buffer_server, index, pname, pointer);
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

exposed_to_tests void _gl_hint (command_buffer_server_t *server, GLenum target, GLenum mode)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glHint) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (target == GL_GENERATE_MIPMAP_HINT &&
            egl_state->state.generate_mipmap_hint == mode)
            return;

        if ( !(mode == GL_FASTEST ||
               mode == GL_NICEST  ||
               mode == GL_DONT_CARE)) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        if (target == GL_GENERATE_MIPMAP_HINT)
            egl_state->state.generate_mipmap_hint = mode;

        command_buffer_server->dispatch.glHint (command_buffer_server, target, mode);

        if (target != GL_GENERATE_MIPMAP_HINT)
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests GLboolean _gl_is_buffer (command_buffer_server_t *server, GLuint buffer)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glIsBuffer) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        result = command_buffer_server->dispatch.glIsBuffer (command_buffer_server, buffer);
    }
    return result;
}

exposed_to_tests GLboolean _gl_is_enabled (command_buffer_server_t *server, GLenum cap)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glIsEnabled) &&
        _gl_is_valid_context (server)) {
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
            _gl_set_error (server, GL_INVALID_ENUM);
            break;
        }
    }
    return result;
}

exposed_to_tests GLboolean _gl_is_framebuffer (command_buffer_server_t *server, GLuint framebuffer)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glIsFramebuffer) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        result = command_buffer_server->dispatch.glIsFramebuffer (command_buffer_server, framebuffer);
    }
    return result;
}

exposed_to_tests GLboolean _gl_is_program (command_buffer_server_t *server, GLuint program)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glIsProgram) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        result = command_buffer_server->dispatch.glIsProgram (command_buffer_server, program);
    }
    return result;
}

exposed_to_tests GLboolean _gl_is_renderbuffer (command_buffer_server_t *server, GLuint renderbuffer)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glIsRenderbuffer) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        result = command_buffer_server->dispatch.glIsRenderbuffer (command_buffer_server, renderbuffer);
    }
    return result;
}

exposed_to_tests GLboolean _gl_is_shader (command_buffer_server_t *server, GLuint shader)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glIsShader) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        result = command_buffer_server->dispatch.glIsShader (command_buffer_server, shader);
    }
    return result;
}

exposed_to_tests GLboolean _gl_is_texture (command_buffer_server_t *server, GLuint texture)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glIsTexture) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        result = command_buffer_server->dispatch.glIsTexture (command_buffer_server, texture);
    }
    return result;
}

exposed_to_tests void _gl_line_width (command_buffer_server_t *server, GLfloat width)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glLineWidth) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (egl_state->state.line_width == width)
            return;

        if (width < 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            return;
        }

        egl_state->state.line_width = width;
        command_buffer_server->dispatch.glLineWidth (command_buffer_server, width);
    }
}

exposed_to_tests void _gl_link_program (command_buffer_server_t *server, GLuint program)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glLinkProgram) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glLinkProgram (command_buffer_server, program);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_pixel_storei (command_buffer_server_t *server, GLenum pname, GLint param)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glPixelStorei) &&
        _gl_is_valid_context (server)) {
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
            _gl_set_error (server, GL_INVALID_VALUE);
            return;
        }
        else if (! (pname == GL_PACK_ALIGNMENT ||
                    pname == GL_UNPACK_ALIGNMENT)) {
            _gl_set_error (server, GL_INVALID_VALUE);
            return;
        }

        if (pname == GL_PACK_ALIGNMENT)
           egl_state->state.pack_alignment = param;
        else
           egl_state->state.unpack_alignment = param;

        command_buffer_server->dispatch.glPixelStorei (command_buffer_server, pname, param);
    }
}

exposed_to_tests void _gl_polygon_offset (command_buffer_server_t *server, GLfloat factor, GLfloat units)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glPolygonOffset) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (egl_state->state.polygon_offset_factor == factor &&
            egl_state->state.polygon_offset_units == units)
            return;

        egl_state->state.polygon_offset_factor = factor;
        egl_state->state.polygon_offset_units = units;

        command_buffer_server->dispatch.glPolygonOffset (command_buffer_server, factor, units);
    }
}

/* sync call */
exposed_to_tests void _gl_read_pixels (command_buffer_server_t *server, GLint x, GLint y,
                             GLsizei width, GLsizei height,
                             GLenum format, GLenum type, GLvoid *data)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glReadPixels) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glReadPixels (command_buffer_server, x, y, width, height, format, type, data);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_compile_shader (command_buffer_server_t *server, GLuint shader)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glCompileShader) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glCompileShader (command_buffer_server, shader);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_release_shader_compiler (command_buffer_server_t *server)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glReleaseShaderCompiler) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glReleaseShaderCompiler (command_buffer_server);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_renderbuffer_storage (command_buffer_server_t *server, GLenum target, GLenum internalformat,
                                      GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glRenderbufferStorage) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glRenderbufferStorage (command_buffer_server, target, internalformat, width, height);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_sample_coverage (command_buffer_server_t *server, GLclampf value, GLboolean invert)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glSampleCoverage) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (value == egl_state->state.sample_coverage_value &&
            invert == egl_state->state.sample_coverage_invert)
            return;

        egl_state->state.sample_coverage_invert = invert;
        egl_state->state.sample_coverage_value = value;

        command_buffer_server->dispatch.glSampleCoverage (command_buffer_server, value, invert);
    }
}

exposed_to_tests void _gl_scissor (command_buffer_server_t *server, GLint x, GLint y, GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glScissor) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (x == egl_state->state.scissor_box[0]     &&
            y == egl_state->state.scissor_box[1]     &&
            width == egl_state->state.scissor_box[2] &&
            height == egl_state->state.scissor_box[3])
            return;

        if (width < 0 || height < 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            return;
        }

        egl_state->state.scissor_box[0] = x;
        egl_state->state.scissor_box[1] = y;
        egl_state->state.scissor_box[2] = width;
        egl_state->state.scissor_box[3] = height;

        command_buffer_server->dispatch.glScissor (command_buffer_server, x, y, width, height);
    }
}

exposed_to_tests void _gl_shader_binary (command_buffer_server_t *server, GLsizei n, const GLuint *shaders,
                               GLenum binaryformat, const void *binary,
                               GLsizei length)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glShaderBinary) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glShaderBinary (command_buffer_server, n, shaders, binaryformat, binary, length);
        egl_state->state.need_get_error = true;
    }
    if (binary)
        free ((void *)binary);
}

exposed_to_tests void _gl_shader_source (command_buffer_server_t *server, GLuint shader, GLsizei count,
                               const GLchar **string, const GLint *length)
{
    egl_state_t *egl_state;
    int i;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glShaderSource) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glShaderSource (command_buffer_server, shader, count, string, length);
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

exposed_to_tests void _gl_stencil_func_separate (command_buffer_server_t *server, GLenum face, GLenum func,
                                       GLint ref, GLuint mask)
{
    egl_state_t *egl_state;
    bool needs_call = false;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glStencilFuncSeparate) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (! (face == GL_FRONT         ||
               face == GL_BACK          ||
               face == GL_FRONT_AND_BACK)) {
            _gl_set_error (server, GL_INVALID_ENUM);
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
            _gl_set_error (server, GL_INVALID_ENUM);
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
        command_buffer_server->dispatch.glStencilFuncSeparate (command_buffer_server, face, func, ref, mask);
}

exposed_to_tests void _gl_stencil_func (command_buffer_server_t *server, GLenum func, GLint ref, GLuint mask)
{
    _gl_stencil_func_separate (server, GL_FRONT_AND_BACK, func, ref, mask);
}

exposed_to_tests void _gl_stencil_mask_separate (command_buffer_server_t *server, GLenum face, GLuint mask)
{
    egl_state_t *egl_state;
    bool needs_call = false;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glStencilMaskSeparate) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (! (face == GL_FRONT         ||
               face == GL_BACK          ||
               face == GL_FRONT_AND_BACK)) {
            _gl_set_error (server, GL_INVALID_ENUM);
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
        command_buffer_server->dispatch.glStencilMaskSeparate (command_buffer_server, face, mask);
}

exposed_to_tests void _gl_stencil_mask (command_buffer_server_t *server, GLuint mask)
{
    _gl_stencil_mask_separate (server, GL_FRONT_AND_BACK, mask);
}

exposed_to_tests void _gl_stencil_op_separate (command_buffer_server_t *server, GLenum face, GLenum sfail, 
                                     GLenum dpfail, GLenum dppass)
{
    egl_state_t *egl_state;
    bool needs_call = false;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glStencilOpSeparate) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (! (face == GL_FRONT         ||
               face == GL_BACK          ||
               face == GL_FRONT_AND_BACK)) {
            _gl_set_error (server, GL_INVALID_ENUM);
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
            _gl_set_error (server, GL_INVALID_ENUM);
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
            _gl_set_error (server, GL_INVALID_ENUM);
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
            _gl_set_error (server, GL_INVALID_ENUM);
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
        command_buffer_server->dispatch.glStencilOpSeparate (command_buffer_server, face, sfail, dpfail, dppass);
}

exposed_to_tests void _gl_stencil_op (command_buffer_server_t *server, GLenum sfail, GLenum dpfail, GLenum dppass)
{
    _gl_stencil_op_separate (server, GL_FRONT_AND_BACK, sfail, dpfail, dppass);
}

exposed_to_tests void _gl_tex_image_2d (command_buffer_server_t *server, GLenum target, GLint level, 
                              GLint internalformat,
                              GLsizei width, GLsizei height, GLint border,
                              GLenum format, GLenum type, 
                              const GLvoid *data)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glTexImage2D) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glTexImage2D (command_buffer_server, target, level, internalformat, width, height,
                             border, format, type, data);
        egl_state->state.need_get_error = true;
    }
    if (data)
      free ((void *)data);
}

exposed_to_tests void _gl_tex_parameteri (command_buffer_server_t *server, GLenum target, GLenum pname, GLint param)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    int active_texture_index;
    int target_index;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glTexParameteri) &&
        _gl_is_valid_context (server)) {
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
            _gl_set_error (server, GL_INVALID_ENUM);
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
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        if (pname == GL_TEXTURE_MAG_FILTER &&
            ! (param == GL_NEAREST ||
               param == GL_LINEAR ||
               param == GL_NEAREST_MIPMAP_NEAREST ||
               param == GL_LINEAR_MIPMAP_NEAREST  ||
               param == GL_NEAREST_MIPMAP_LINEAR  ||
               param == GL_LINEAR_MIPMAP_LINEAR)) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }
        else if (pname == GL_TEXTURE_MIN_FILTER &&
                 ! (param == GL_NEAREST ||
                    param == GL_LINEAR)) {
            _gl_set_error (server, GL_INVALID_ENUM);
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
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        command_buffer_server->dispatch.glTexParameteri (command_buffer_server, target, pname, param);
    }
}

exposed_to_tests void _gl_tex_parameterf (command_buffer_server_t *server, GLenum target, GLenum pname, GLfloat param)
{
    GLint parami = param;
    _gl_tex_parameteri (server, target, pname, parami);
}

exposed_to_tests void _gl_tex_sub_image_2d (command_buffer_server_t *server, GLenum target, GLint level,
                                  GLint xoffset, GLint yoffset,
                                  GLsizei width, GLsizei height,
                                  GLenum format, GLenum type, 
                                  const GLvoid *data)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glTexSubImage2D) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glTexSubImage2D (command_buffer_server, target, level, xoffset, yoffset,
                                width, height, format, type, data);
        egl_state->state.need_get_error = true;
    }
    if (data)
        free ((void *)data);
}

exposed_to_tests void _gl_uniform1f (command_buffer_server_t *server, GLint location, GLfloat v0)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glUniform1f) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glUniform1f (command_buffer_server, location, v0);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_uniform2f (command_buffer_server_t *server, GLint location, GLfloat v0, GLfloat v1)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glUniform2f) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glUniform2f (command_buffer_server, location, v0, v1);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_uniform3f (command_buffer_server_t *server, GLint location, GLfloat v0, GLfloat v1, 
                           GLfloat v2)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glUniform3f) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glUniform3f (command_buffer_server, location, v0, v1, v2);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_uniform4f (command_buffer_server_t *server, GLint location, GLfloat v0, GLfloat v1, 
                           GLfloat v2, GLfloat v3)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glUniform4f) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glUniform4f (command_buffer_server, location, v0, v1, v2, v3);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_uniform1i (command_buffer_server_t *server, GLint location, GLint v0)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glUniform1i) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glUniform1i (command_buffer_server, location, v0);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_uniform_2i (command_buffer_server_t *server, GLint location, GLint v0, GLint v1)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glUniform2i) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glUniform2i (command_buffer_server, location, v0, v1);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_uniform3i (command_buffer_server_t *server, GLint location, GLint v0, GLint v1, GLint v2)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glUniform3i) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glUniform3i (command_buffer_server, location, v0, v1, v2);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_uniform4i (command_buffer_server_t *server, GLint location, GLint v0, GLint v1, 
                           GLint v2, GLint v3)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glUniform4i) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glUniform4i (command_buffer_server, location, v0, v1, v2, v3);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void
_gl_uniform_fv (command_buffer_server_t *server, int i, GLint location,
                GLsizei count, const GLfloat *value)
{
    egl_state_t *egl_state;

    if (! _gl_is_valid_context (server))
        goto FINISH;

    switch (i) {
    case 1:
        if(! _gl_is_valid_func (server, command_buffer_server->dispatch.glUniform1fv))
            goto FINISH;
        command_buffer_server->dispatch.glUniform1fv (command_buffer_server, location, count, value);
        break;
    case 2:
        if(! _gl_is_valid_func (server, command_buffer_server->dispatch.glUniform2fv))
            goto FINISH;
        command_buffer_server->dispatch.glUniform2fv (command_buffer_server, location, count, value);
        break;
    case 3:
        if(! _gl_is_valid_func (server, command_buffer_server->dispatch.glUniform3fv))
            goto FINISH;
        command_buffer_server->dispatch.glUniform3fv (command_buffer_server, location, count, value);
        break;
    default:
        if(! _gl_is_valid_func (server, command_buffer_server->dispatch.glUniform4fv))
            goto FINISH;
        command_buffer_server->dispatch.glUniform4fv (command_buffer_server, location, count, value);
        break;
    }

    egl_state = (egl_state_t *) active_state->data;
    egl_state->state.need_get_error = true;

FINISH:
    if (value)
        free ((GLfloat *)value);
}

exposed_to_tests void _gl_uniform1fv (command_buffer_server_t *server, GLint location, GLsizei count, 
                            const GLfloat *value)
{
    _gl_uniform_fv (server, 1, location, count, value);
}

exposed_to_tests void _gl_uniform2fv (command_buffer_server_t *server, GLint location, GLsizei count, 
                            const GLfloat *value)
{
    _gl_uniform_fv (server, 2, location, count, value);
}

exposed_to_tests void _gl_uniform3fv (command_buffer_server_t *server, GLint location, GLsizei count, 
                            const GLfloat *value)
{
    _gl_uniform_fv (server, 3, location, count, value);
}

exposed_to_tests void _gl_uniform4fv (command_buffer_server_t *server, GLint location, GLsizei count, 
                            const GLfloat *value)
{
    _gl_uniform_fv (server, 4, location, count, value);
}

exposed_to_tests void
_gl_uniform_iv (command_buffer_server_t *server,
                int i,
                GLint location,
                GLsizei count,
                const GLint *value)
{
    egl_state_t *egl_state;

    if (! _gl_is_valid_context (server))
        goto FINISH;

    switch (i) {
    case 1:
        if(! _gl_is_valid_func (server, command_buffer_server->dispatch.glUniform1iv))
            goto FINISH;
        command_buffer_server->dispatch.glUniform1iv (command_buffer_server, location, count, value);
        break;
    case 2:
        if(! _gl_is_valid_func (server, command_buffer_server->dispatch.glUniform2iv))
            goto FINISH;
        command_buffer_server->dispatch.glUniform2iv (command_buffer_server, location, count, value);
        break;
    case 3:
        if(! _gl_is_valid_func (server, command_buffer_server->dispatch.glUniform3iv))
            goto FINISH;
        command_buffer_server->dispatch.glUniform3iv (command_buffer_server, location, count, value);
        break;
    default:
        if(! _gl_is_valid_func (server, command_buffer_server->dispatch.glUniform4iv))
            goto FINISH;
        command_buffer_server->dispatch.glUniform4iv (command_buffer_server, location, count, value);
        break;
    }

    egl_state = (egl_state_t *) active_state->data;
    egl_state->state.need_get_error = true;

FINISH:
    if (value)
        free ((GLint *)value);
}

exposed_to_tests void _gl_uniform1iv (command_buffer_server_t *server, GLint location, GLsizei count, 
                            const GLint *value)
{
    _gl_uniform_iv (server, 1, location, count, value);
}

exposed_to_tests void _gl_uniform2iv (command_buffer_server_t *server, GLint location, GLsizei count, 
                            const GLint *value)
{
    _gl_uniform_iv (server, 2, location, count, value);
}

exposed_to_tests void _gl_uniform3iv (command_buffer_server_t *server, GLint location, GLsizei count, 
                            const GLint *value)
{
    _gl_uniform_iv (server, 3, location, count, value);
}

exposed_to_tests void _gl_uniform4iv (command_buffer_server_t *server, GLint location, GLsizei count, 
                            const GLint *value)
{
    _gl_uniform_iv (server, 4, location, count, value);
}

exposed_to_tests void _gl_use_program (command_buffer_server_t *server, GLuint program)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glUseProgram) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (egl_state->state.current_program == program)
            return;

        command_buffer_server->dispatch.glUseProgram (command_buffer_server, program);
        /* FIXME: this maybe not right because this program may be invalid
         * object, we save here to save time in glGetError() */
        egl_state->state.current_program = program;
        /* FIXME: do we need to have this ? */
        // egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_validate_program (command_buffer_server_t *server, GLuint program)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glValidateProgram) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glValidateProgram (command_buffer_server, program);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_vertex_attrib1f (command_buffer_server_t *server, GLuint index, GLfloat v0)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glVertexAttrib1f) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (_gl_index_is_too_large (server, &egl_state->state, index)) {
            return;
        }

        command_buffer_server->dispatch.glVertexAttrib1f (command_buffer_server, index, v0);
    }
}

exposed_to_tests void _gl_vertex_attrib2f (command_buffer_server_t *server, GLuint index, GLfloat v0, GLfloat v1)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glVertexAttrib2f) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (_gl_index_is_too_large (server, &egl_state->state, index)) {
            return;
        }

        command_buffer_server->dispatch.glVertexAttrib2f (command_buffer_server, index, v0, v1);
    }
}

exposed_to_tests void _gl_vertex_attrib3f (command_buffer_server_t *server, GLuint index, GLfloat v0, 
                                 GLfloat v1, GLfloat v2)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glVertexAttrib3f) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (_gl_index_is_too_large (server, &egl_state->state, index)) {
            return;
        }

        command_buffer_server->dispatch.glVertexAttrib3f (command_buffer_server, index, v0, v1, v2);
    }
}

exposed_to_tests void _gl_vertex_attrib4f (command_buffer_server_t *server, GLuint index, GLfloat v0, GLfloat v1, 
                                 GLfloat v2, GLfloat v3)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glVertexAttrib3f) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (_gl_index_is_too_large (server, &egl_state->state, index))
            return;

        command_buffer_server->dispatch.glVertexAttrib4f (command_buffer_server, index, v0, v1, v2, v3);
    }
}

exposed_to_tests void
_gl_vertex_attrib_fv (command_buffer_server_t *server, int i, GLuint index, const GLfloat *v)
{
    egl_state_t *egl_state;
    
    if (! _gl_is_valid_context (server))
        goto FINISH;
    
    egl_state = (egl_state_t *) active_state->data;

    if (_gl_index_is_too_large (server, &egl_state->state, index))
        goto FINISH;

    switch (i) {
        case 1:
            if(! _gl_is_valid_func (server, command_buffer_server->dispatch.glVertexAttrib1fv))
                goto FINISH;
            command_buffer_server->dispatch.glVertexAttrib1fv (command_buffer_server, index, v);
            break;
        case 2:
            if(! _gl_is_valid_func (server, command_buffer_server->dispatch.glVertexAttrib2fv))
                goto FINISH;
            command_buffer_server->dispatch.glVertexAttrib2fv (command_buffer_server, index, v);
            break;
        case 3:
            if(! _gl_is_valid_func (server, command_buffer_server->dispatch.glVertexAttrib3fv))
                goto FINISH;
            command_buffer_server->dispatch.glVertexAttrib3fv (command_buffer_server, index, v);
            break;
        default:
            if(! _gl_is_valid_func (server, command_buffer_server->dispatch.glVertexAttrib4fv))
                goto FINISH;
            command_buffer_server->dispatch.glVertexAttrib4fv (command_buffer_server, index, v);
            break;
    }

FINISH:
    if (v)
        free ((GLfloat *)v);
}

exposed_to_tests void _gl_vertex_attrib1fv (command_buffer_server_t *server, GLuint index, const GLfloat *v)
{
    _gl_vertex_attrib_fv (server, 1, index, v);
}

exposed_to_tests void _gl_vertex_attrib2fv (command_buffer_server_t *server, GLuint index, const GLfloat *v)
{
    _gl_vertex_attrib_fv (server, 2, index, v);
}

exposed_to_tests void _gl_vertex_attrib3fv (command_buffer_server_t *server, GLuint index, const GLfloat *v)
{
    _gl_vertex_attrib_fv (server, 3, index, v);
}

exposed_to_tests void _gl_vertex_attrib4fv (command_buffer_server_t *server, GLuint index, const GLfloat *v)
{
    _gl_vertex_attrib_fv (server, 4, index, v);
}

exposed_to_tests void _gl_vertex_attrib_pointer (command_buffer_server_t *server, GLuint index, GLint size, 
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

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glVertexAttribPointer) &&
        _gl_is_valid_context (server)) {
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
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }
        else if (size > 4 || size < 1 || stride < 0) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        /* check max_vertex_attribs */
        if (_gl_index_is_too_large (server, &egl_state->state, index)) {
            return;
        }
        
#ifdef GL_OES_vertex_array_object
        if (egl_state->state.vertex_array_binding) {
            command_buffer_server->dispatch.glVertexAttribPointer (command_buffer_server, index, size, type, normalized,
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
            command_buffer_server->dispatch.glVertexAttribPointer (command_buffer_server, index, size, type, normalized,
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

exposed_to_tests void _gl_viewport (command_buffer_server_t *server, GLint x, GLint y, GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glViewport) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (egl_state->state.viewport[0] == x     &&
            egl_state->state.viewport[1] == y     &&
            egl_state->state.viewport[2] == width &&
            egl_state->state.viewport[3] == height)
            return;

        if (x < 0 || y < 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            return;
        }

        egl_state->state.viewport[0] = x;
        egl_state->state.viewport[1] = y;
        egl_state->state.viewport[2] = width;
        egl_state->state.viewport[3] = height;

        command_buffer_server->dispatch.glViewport (command_buffer_server, x, y, width, height);
    }
}
/* end of GLES2 core profile */

#ifdef GL_OES_EGL_image
static void
_gl_egl_image_target_texture_2d_oes (command_buffer_server_t *server, GLenum target, GLeglImageOES image)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glEGLImageTargetTexture2DOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (target != GL_TEXTURE_2D) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        command_buffer_server->dispatch.glEGLImageTargetTexture2DOES (command_buffer_server, target, image);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_egl_image_target_renderbuffer_storage_oes (command_buffer_server_t *server, GLenum target, 
                                               GLeglImageOES image)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glEGLImageTargetRenderbufferStorageOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
#ifndef HAS_GLES2
        if (target != GL_RENDERBUFFER_OES) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }
#endif

        command_buffer_server->dispatch.glEGLImageTargetRenderbufferStorageOES (command_buffer_server, target, image);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_OES_get_program_binary
static void
_gl_get_program_binary_oes (command_buffer_server_t *server, GLuint program, GLsizei bufSize, 
                            GLsizei *length, GLenum *binaryFormat, 
                            GLvoid *binary)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetProgramBinaryOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glGetProgramBinaryOES (command_buffer_server, program, bufSize, length, 
                                      binaryFormat, binary);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_program_binary_oes (command_buffer_server_t *server, GLuint program, GLenum binaryFormat,
                        const GLvoid *binary, GLint length)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glProgramBinaryOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glProgramBinaryOES (command_buffer_server, program, binaryFormat, binary, length);
        egl_state->state.need_get_error = true;
    }

    if (binary)
        free ((void *)binary);
}
#endif

#ifdef GL_OES_mapbuffer
static void* 
_gl_map_buffer_oes (command_buffer_server_t *server, GLenum target, GLenum access)
{
    egl_state_t *egl_state;
    void *result = NULL;
    
    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glMapBufferOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (access != GL_WRITE_ONLY_OES) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return result;
        }
        else if (! (target == GL_ARRAY_BUFFER ||
                    target == GL_ELEMENT_ARRAY_BUFFER)) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return result;
        }

        result = command_buffer_server->dispatch.glMapBufferOES (command_buffer_server, target, access);
        egl_state->state.need_get_error = true;
    }
    return result;
}

static GLboolean
_gl_unmap_buffer_oes (command_buffer_server_t *server, GLenum target)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glUnmapBufferOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (! (target == GL_ARRAY_BUFFER ||
               target == GL_ELEMENT_ARRAY_BUFFER)) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return result;
        }

        result = command_buffer_server->dispatch.glUnmapBufferOES (command_buffer_server, target);
        egl_state->state.need_get_error = true;
    }
    return result;
}

static void
_gl_get_buffer_pointerv_oes (command_buffer_server_t *server, GLenum target, GLenum pname, GLvoid **params)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetBufferPointervOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (! (target == GL_ARRAY_BUFFER ||
               target == GL_ELEMENT_ARRAY_BUFFER)) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        command_buffer_server->dispatch.glGetBufferPointervOES (command_buffer_server, target, pname, params);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_OES_texture_3D
static void
_gl_tex_image_3d_oes (command_buffer_server_t *server, GLenum target, GLint level, GLenum internalformat,
                      GLsizei width, GLsizei height, GLsizei depth,
                      GLint border, GLenum format, GLenum type,
                      const GLvoid *pixels)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glTexImage3DOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glTexImage3DOES (command_buffer_server, target, level, internalformat, 
                                width, height, depth, 
                                border, format, type, pixels);
        egl_state->state.need_get_error = true;
    }
    if (pixels)
        free ((void *)pixels);
}

static void
_gl_tex_sub_image_3d_oes (command_buffer_server_t *server, GLenum target, GLint level,
                          GLint xoffset, GLint yoffset, GLint zoffset,
                          GLsizei width, GLsizei height, GLsizei depth,
                          GLenum format, GLenum type, const GLvoid *data)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glTexSubImage3DOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glTexSubImage3DOES (command_buffer_server, target, level, 
                                   xoffset, yoffset, zoffset,
                                   width, height, depth, 
                                   format, type, data);
        egl_state->state.need_get_error = true;
    }
    if (data)
        free ((void *)data);
}

static void
_gl_copy_tex_sub_image_3d_oes (command_buffer_server_t *server, GLenum target, GLint level,
                               GLint xoffset, GLint yoffset, GLint zoffset,
                               GLint x, GLint y,
                               GLint width, GLint height)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glCopyTexSubImage3DOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glCopyTexSubImage3DOES (command_buffer_server, target, level,
                                       xoffset, yoffset, zoffset,
                                       x, y, width, height);

        egl_state->state.need_get_error = true;
    }
}

static void
_gl_compressed_tex_image_3d_oes (command_buffer_server_t *server, GLenum target, GLint level,
                                 GLenum internalformat,
                                 GLsizei width, GLsizei height, 
                                 GLsizei depth,
                                 GLint border, GLsizei imageSize,
                                 const GLvoid *data)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glCompressedTexImage3DOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glCompressedTexImage3DOES (command_buffer_server, target, level, internalformat,
                                          width, height, depth,
                                          border, imageSize, data);

        egl_state->state.need_get_error = true;
    }
    if (data)
        free ((void *)data);
}

static void
_gl_compressed_tex_sub_image_3d_oes (command_buffer_server_t *server, GLenum target, GLint level,
                                     GLint xoffset, GLint yoffset, 
                                     GLint zoffset,
                                     GLsizei width, GLsizei height, 
                                     GLsizei depth,
                                     GLenum format, GLsizei imageSize,
                                     const GLvoid *data)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glCompressedTexSubImage3DOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        command_buffer_server->dispatch.glCompressedTexSubImage3DOES (command_buffer_server, target, level,
                                             xoffset, yoffset, zoffset,
                                             width, height, depth,
                                             format, imageSize, data);

        egl_state->state.need_get_error = true;
    }

    if (data)
        free ((void *)data);
}

static void
_gl_framebuffer_texture_3d_oes (command_buffer_server_t *server, GLenum target, GLenum attachment,
                                GLenum textarget, GLuint texture,
                                GLint level, GLint zoffset)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glFramebufferTexture3DOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (target != GL_FRAMEBUFFER) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        command_buffer_server->dispatch.glFramebufferTexture3DOES (command_buffer_server, target, attachment, textarget,
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
_gl_bind_vertex_array_oes (command_buffer_server_t *server, GLuint array)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glBindVertexArrayOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (egl_state->state.vertex_array_binding == array)
            return;

        command_buffer_server->dispatch.glBindVertexArrayOES (command_buffer_server, array);
        egl_state->state.need_get_error = true;
        /* FIXME: should we save this ? */
        egl_state->state.vertex_array_binding = array;
    }
}

static void
_gl_delete_vertex_arrays_oes (command_buffer_server_t *server, GLsizei n, const GLuint *arrays)
{
    egl_state_t *egl_state;
    int i;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glDeleteVertexArraysOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        if (n <= 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            goto FINISH;
        }

        command_buffer_server->dispatch.glDeleteVertexArraysOES (command_buffer_server, n, arrays);

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
_gl_gen_vertex_arrays_oes (command_buffer_server_t *server, GLsizei n, GLuint *arrays)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGenVertexArraysOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        if (n <= 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            return;
        }

        command_buffer_server->dispatch.glGenVertexArraysOES (command_buffer_server, n, arrays);
    }
}

static GLboolean
_gl_is_vertex_array_oes (command_buffer_server_t *server, GLuint array)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glIsVertexArrayOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        result = command_buffer_server->dispatch.glIsVertexArrayOES (command_buffer_server, array);

        if (result == GL_FALSE && 
            egl_state->state.vertex_array_binding == array)
            egl_state->state.vertex_array_binding = 0;
    }
    return result;
}
#endif

#ifdef GL_AMD_performance_monitor
static void
_gl_get_perf_monitor_groups_amd (command_buffer_server_t *server, GLint *numGroups, GLsizei groupSize, 
                                 GLuint *groups)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetPerfMonitorGroupsAMD) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        command_buffer_server->dispatch.glGetPerfMonitorGroupsAMD (command_buffer_server, numGroups, groupSize, groups);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_get_perf_monitor_counters_amd (command_buffer_server_t *server, GLuint group, GLint *numCounters, 
                                   GLint *maxActiveCounters, 
                                   GLsizei counterSize,
                                   GLuint *counters)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetPerfMonitorCountersAMD) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        command_buffer_server->dispatch.glGetPerfMonitorCountersAMD (command_buffer_server, group, numCounters,
                                            maxActiveCounters,
                                            counterSize, counters);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_get_perf_monitor_group_string_amd (command_buffer_server_t *server, GLuint group, GLsizei bufSize, 
                                       GLsizei *length, 
                                       GLchar *groupString)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetPerfMonitorGroupStringAMD) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        command_buffer_server->dispatch.glGetPerfMonitorGroupStringAMD (command_buffer_server, group, bufSize, length,
                                               groupString);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_get_perf_monitor_counter_string_amd (command_buffer_server_t *server, GLuint group, GLuint counter, 
                                         GLsizei bufSize, 
                                         GLsizei *length, 
                                         GLchar *counterString)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetPerfMonitorCounterStringAMD) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        command_buffer_server->dispatch.glGetPerfMonitorCounterStringAMD (command_buffer_server, group, counter, bufSize, 
                                                 length, counterString);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_get_perf_monitor_counter_info_amd (command_buffer_server_t *server, GLuint group, GLuint counter, 
                                       GLenum pname, GLvoid *data)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetPerfMonitorCounterInfoAMD) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        command_buffer_server->dispatch.glGetPerfMonitorCounterInfoAMD (command_buffer_server, group, counter, 
                                               pname, data); 
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_gen_perf_monitors_amd (command_buffer_server_t *server, GLsizei n, GLuint *monitors) 
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGenPerfMonitorsAMD) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        command_buffer_server->dispatch.glGenPerfMonitorsAMD (command_buffer_server, n, monitors); 
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_delete_perf_monitors_amd (command_buffer_server_t *server, GLsizei n, GLuint *monitors) 
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glDeletePerfMonitorsAMD) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        command_buffer_server->dispatch.glDeletePerfMonitorsAMD (command_buffer_server, n, monitors); 
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_select_perf_monitor_counters_amd (command_buffer_server_t *server, GLuint monitor, GLboolean enable,
                                      GLuint group, GLint numCounters,
                                      GLuint *countersList) 
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glSelectPerfMonitorCountersAMD) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        command_buffer_server->dispatch.glSelectPerfMonitorCountersAMD (command_buffer_server, monitor, enable, group,
                                               numCounters, countersList); 
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_begin_perf_monitor_amd (command_buffer_server_t *server, GLuint monitor)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glBeginPerfMonitorAMD) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        command_buffer_server->dispatch.glBeginPerfMonitorAMD (command_buffer_server, monitor); 
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_end_perf_monitor_amd (command_buffer_server_t *server, GLuint monitor)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glEndPerfMonitorAMD) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        command_buffer_server->dispatch.glEndPerfMonitorAMD (command_buffer_server, monitor); 
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_get_perf_monitor_counter_data_amd (command_buffer_server_t *server, GLuint monitor, GLenum pname,
                                       GLsizei dataSize, GLuint *data,
                                       GLint *bytesWritten)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetPerfMonitorCounterDataAMD) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        command_buffer_server->dispatch.glGetPerfMonitorCounterDataAMD (command_buffer_server, monitor, pname, dataSize,
                                               data, bytesWritten); 
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_ANGLE_framebuffer_blit
static void
_gl_blit_framebuffer_angle (command_buffer_server_t *server, GLint srcX0, GLint srcY0, 
                            GLint srcX1, GLint srcY1,
                            GLint dstX0, GLint dstY0, 
                            GLint dstX1, GLint dstY1,
                            GLbitfield mask, GLenum filter)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glBlitFramebufferANGLE) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        command_buffer_server->dispatch.glBlitFramebufferANGLE (command_buffer_server, srcX0, srcY0, srcX1, srcY1,
                                       dstX0, dstY0, dstX1, dstY1,
                                       mask, filter);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_ANGLE_framebuffer_multisample
static void
_gl_renderbuffer_storage_multisample_angle (command_buffer_server_t *server, GLenum target, GLsizei samples,
                                            GLenum internalformat,
                                            GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glRenderbufferStorageMultisampleANGLE) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        command_buffer_server->dispatch.glRenderbufferStorageMultisampleANGLE (command_buffer_server, target, samples,
                                                      internalformat,
                                                      width, height);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_APPLE_framebuffer_multisample
static void
_gl_renderbuffer_storage_multisample_apple (command_buffer_server_t *server, GLenum target, GLsizei samples,
                                            GLenum internalformat, 
                                            GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glRenderbufferStorageMultisampleAPPLE) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        command_buffer_server->dispatch.glRenderbufferStorageMultisampleAPPLE (command_buffer_server, target, samples,
                                                      internalformat,
                                                      width, height);
        egl_state->state.need_get_error = true;
    }
}

static void
gl_resolve_multisample_framebuffer_apple (void)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glResolveMultisampleFramebufferAPPLE) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        command_buffer_server->dispatch.glResolveMultisampleFramebufferAPPLE (command_buffer_server);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_EXT_discard_framebuffer
static void
gl_discard_framebuffer_ext (command_buffer_server_t *server,
                            GLenum target, GLsizei numAttachments,
                            const GLenum *attachments)
{
    egl_state_t *egl_state;
    int i;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glDiscardFramebufferEXT) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        if (target != GL_FRAMEBUFFER) {
            _gl_set_error (server, GL_INVALID_ENUM);
            goto FINISH;
        }

        for (i = 0; i < numAttachments; i++) {
            if (! (attachments[i] == GL_COLOR_ATTACHMENT0  ||
                   attachments[i] == GL_DEPTH_ATTACHMENT   ||
                   attachments[i] == GL_STENCIL_ATTACHMENT ||
                   attachments[i] == GL_COLOR_EXT          ||
                   attachments[i] == GL_DEPTH_EXT          ||
                   attachments[i] == GL_STENCIL_EXT)) {
                _gl_set_error (server, GL_INVALID_ENUM);
                goto FINISH;
            }
        }
        command_buffer_server->dispatch.glDiscardFramebufferEXT (command_buffer_server, target, numAttachments, 
                                        attachments);
    }
FINISH:
    if (attachments)
        free ((void *)attachments);
}
#endif

#ifdef GL_EXT_multi_draw_arrays
static void
_gl_multi_draw_arrays_ext (command_buffer_server_t *server, GLenum mode, const GLint *first, 
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
_gl_renderbuffer_storage_multisample_ext (command_buffer_server_t *server, GLenum target, GLsizei samples,
                                          GLenum internalformat,
                                          GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glRenderbufferStorageMultisampleEXT) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        command_buffer_server->dispatch.glRenderbufferStorageMultisampleEXT (command_buffer_server, target, samples, 
                                                    internalformat, 
                                                    width, height);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_framebuffer_texture_2d_multisample_ext (command_buffer_server_t *server, GLenum target, 
                                            GLenum attachment,
                                            GLenum textarget, 
                                            GLuint texture,
                                            GLint level, GLsizei samples)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glFramebufferTexture2DMultisampleEXT) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        if (target != GL_FRAMEBUFFER) {
           _gl_set_error (server, GL_INVALID_ENUM);
           return;
        }

        command_buffer_server->dispatch.glFramebufferTexture2DMultisampleEXT (command_buffer_server, target, attachment, 
                                                     textarget, texture, 
                                                     level, samples);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_IMG_multisampled_render_to_texture
static void
_gl_renderbuffer_storage_multisample_img (command_buffer_server_t *server, GLenum target, GLsizei samples,
                                          GLenum internalformat,
                                          GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glRenderbufferStorageMultisampleIMG) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        command_buffer_server->dispatch.glRenderbufferStorageMultisampleIMG (command_buffer_server, target, samples, 
                                                    internalformat, 
                                                    width, height);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_framebuffer_texture_2d_multisample_img (command_buffer_server_t *server, GLenum target, 
                                            GLenum attachment,
                                            GLenum textarget, 
                                            GLuint texture,
                                            GLint level, GLsizei samples)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glFramebufferTexture2DMultisampleIMG) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        if (target != GL_FRAMEBUFFER) {
           _gl_set_error (server, GL_INVALID_ENUM);
           return;
        }

        command_buffer_server->dispatch.glFramebufferTexture2DMultisampleIMG (command_buffer_server, target, attachment,
                                                     textarget, texture,
                                                     level, samples);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_NV_fence
static void
_gl_delete_fences_nv (command_buffer_server_t *server, GLsizei n, const GLuint *fences)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glDeleteFencesNV) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (n <= 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            goto FINISH;
        }
    
        command_buffer_server->dispatch.glDeleteFencesNV (command_buffer_server, n, fences); 
        egl_state->state.need_get_error = true;
    }
FINISH:
    if (fences)
        free ((void *)fences);
}

static void
_gl_gen_fences_nv (command_buffer_server_t *server, GLsizei n, GLuint *fences)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGenFencesNV) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (n <= 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            return;
        }
    
        command_buffer_server->dispatch.glGenFencesNV (command_buffer_server, n, fences); 
        egl_state->state.need_get_error = true;
    }
}

static GLboolean
_gl_is_fence_nv (command_buffer_server_t *server, GLuint fence)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glIsFenceNV) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        result = command_buffer_server->dispatch.glIsFenceNV (command_buffer_server, fence);
        egl_state->state.need_get_error = true;
    }
    return result;
}

static GLboolean
_gl_test_fence_nv (command_buffer_server_t *server, GLuint fence)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glTestFenceNV) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        result = command_buffer_server->dispatch.glTestFenceNV (command_buffer_server, fence);
        egl_state->state.need_get_error = true;
    }
    return result;
}

static void 
_gl_get_fenceiv_nv (command_buffer_server_t *server, GLuint fence, GLenum pname, int *params)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetFenceivNV) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glGetFenceivNV (command_buffer_server, fence, pname, params);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_finish_fence_nv (command_buffer_server_t *server, GLuint fence)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glFinishFenceNV) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glFinishFenceNV (command_buffer_server, fence);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_set_fence_nv (command_buffer_server_t *server, GLuint fence, GLenum condition)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glSetFenceNV) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glSetFenceNV (command_buffer_server, fence, condition);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_NV_coverage_sample
static void
_gl_coverage_mask_nv (command_buffer_server_t *server, GLboolean mask)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glCoverageMaskNV) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glCoverageMaskNV (command_buffer_server, mask);
    }
}

static void
_gl_coverage_operation_nv (command_buffer_server_t *server, GLenum operation)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glCoverageOperationNV) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (! (operation == GL_COVERAGE_ALL_FRAGMENTS_NV  ||
               operation == GL_COVERAGE_EDGE_FRAGMENTS_NV ||
               operation == GL_COVERAGE_AUTOMATIC_NV)) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        command_buffer_server->dispatch.glCoverageOperationNV (command_buffer_server, operation);
    }
}
#endif

#ifdef GL_QCOM_driver_control
static void
_gl_get_driver_controls_qcom (command_buffer_server_t *server, GLint *num, GLsizei size, 
                              GLuint *driverControls)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetDriverControlsQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glGetDriverControlsQCOM (command_buffer_server, num, size, driverControls);
    }
}

static void
_gl_get_driver_control_string_qcom (command_buffer_server_t *server, GLuint driverControl, GLsizei bufSize,
                                    GLsizei *length, 
                                    GLchar *driverControlString)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glGetDriverControlStringQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glGetDriverControlStringQCOM (command_buffer_server, driverControl, bufSize,
                                             length, driverControlString);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_enable_driver_control_qcom (command_buffer_server_t *server, GLuint driverControl)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glEnableDriverControlQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glEnableDriverControlQCOM (command_buffer_server, driverControl);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_disable_driver_control_qcom (command_buffer_server_t *server, GLuint driverControl)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glDisableDriverControlQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glDisableDriverControlQCOM (command_buffer_server, driverControl);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_QCOM_extended_get
static void
_gl_ext_get_textures_qcom (command_buffer_server_t *server, GLuint *textures, GLint maxTextures, 
                           GLint *numTextures)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glExtGetTexturesQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glExtGetTexturesQCOM (command_buffer_server, textures, maxTextures, numTextures);
    }
}

static void
_gl_ext_get_buffers_qcom (command_buffer_server_t *server, GLuint *buffers, GLint maxBuffers, 
                          GLint *numBuffers) 
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glExtGetBuffersQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glExtGetBuffersQCOM (command_buffer_server, buffers, maxBuffers, numBuffers);
    }
}

static void
_gl_ext_get_renderbuffers_qcom (command_buffer_server_t *server, GLuint *renderbuffers, 
                                GLint maxRenderbuffers,
                                GLint *numRenderbuffers)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glExtGetRenderbuffersQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glExtGetRenderbuffersQCOM (command_buffer_server, renderbuffers, maxRenderbuffers,
                                          numRenderbuffers);
    }
}

static void
_gl_ext_get_framebuffers_qcom (command_buffer_server_t *server, GLuint *framebuffers, GLint maxFramebuffers,
                               GLint *numFramebuffers)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glExtGetFramebuffersQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glExtGetFramebuffersQCOM (command_buffer_server, framebuffers, maxFramebuffers,
                                         numFramebuffers);
    }
}

static void
_gl_ext_get_tex_level_parameteriv_qcom (command_buffer_server_t *server, GLuint texture, GLenum face, 
                                        GLint level, GLenum pname, 
                                        GLint *params)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glExtGetTexLevelParameterivQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glExtGetTexLevelParameterivQCOM (command_buffer_server, texture, face, level,
                                                pname, params);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_ext_tex_object_state_overridei_qcom (command_buffer_server_t *server, GLenum target, GLenum pname, 
                                         GLint param)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glExtTexObjectStateOverrideiQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glExtTexObjectStateOverrideiQCOM (command_buffer_server, target, pname, param);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_ext_get_tex_sub_image_qcom (command_buffer_server_t *server, GLenum target, GLint level,
                                GLint xoffset, GLint yoffset, 
                                GLint zoffset,
                                GLsizei width, GLsizei height, 
                                GLsizei depth,
                                GLenum format, GLenum type, GLvoid *texels)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glExtGetTexSubImageQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glExtGetTexSubImageQCOM (command_buffer_server, target, level,
                                        xoffset, yoffset, zoffset,
                                        width, height, depth,
                                        format, type, texels);
    egl_state->state.need_get_error = true;
    }
}

static void
_gl_ext_get_buffer_pointerv_qcom (command_buffer_server_t *server, GLenum target, GLvoid **params)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glExtGetBufferPointervQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glExtGetBufferPointervQCOM (command_buffer_server, target, params);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_QCOM_extended_get2
static void
_gl_ext_get_shaders_qcom (command_buffer_server_t *server, GLuint *shaders, GLint maxShaders, 
                          GLint *numShaders)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glExtGetShadersQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glExtGetShadersQCOM (command_buffer_server, shaders, maxShaders, numShaders);
    }
}

static void
_gl_ext_get_programs_qcom (command_buffer_server_t *server, GLuint *programs, GLint maxPrograms,
                           GLint *numPrograms)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glExtGetProgramsQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glExtGetProgramsQCOM (command_buffer_server, programs, maxPrograms, numPrograms);
    }
}

static GLboolean
_gl_ext_is_program_binary_qcom (command_buffer_server_t *server, GLuint program)
{
    egl_state_t *egl_state;
    bool result = false;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glExtIsProgramBinaryQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        result = command_buffer_server->dispatch.glExtIsProgramBinaryQCOM (command_buffer_server, program);
        egl_state->state.need_get_error = true;
    }
    return result;
}

static void
_gl_ext_get_program_binary_source_qcom (command_buffer_server_t *server, GLuint program, GLenum shadertype,
                                        GLchar *source, GLint *length)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glExtGetProgramBinarySourceQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glExtGetProgramBinarySourceQCOM (command_buffer_server, program, shadertype,
                                                source, length);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_QCOM_tiled_rendering
static void
_gl_start_tiling_qcom (command_buffer_server_t *server, GLuint x, GLuint y, GLuint width, GLuint height,
                       GLbitfield preserveMask)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glStartTilingQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glStartTilingQCOM (command_buffer_server, x, y, width, height, preserveMask);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_end_tiling_qcom (command_buffer_server_t *server, GLbitfield preserveMask)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, command_buffer_server->dispatch.glEndTilingQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        command_buffer_server->dispatch.glEndTilingQCOM (command_buffer_server, preserveMask);
        egl_state->state.need_get_error = true;
    }
}
#endif
