#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <string.h>
#include <stdlib.h>

/* XXX: we should move it to the srv */
#include "gpuprocess_dispatch_private.h"
#include "gpuprocess_gles2_srv_private.h"
extern gpuprocess_dispatch_t 	dispatch;

#include "gpuprocess_thread_private.h"
#include "gpuprocess_types_private.h"
#include "gpuprocess_gles2_cli_private.h"

/* XXX: where should we put mutx ? */
extern gpu_mutex_t	global_mutex;

extern gpu_signal_t	cond;
extern __thread gl_cli_states_t cli_states;
extern __thread v_link_list_t *active_state;

/* XXX: There are lots of state checking, instead of doing them on
 * client thread, the client thread maybe should just create a buffer, and
 * let server thread to do the checking - thuse potentially client thread
 * can return quickly.  The drawback is that we have lots of command
 */

static inline v_bool_t
_is_error_state_or_func (v_link_list_t *list, void *func)
{
    egl_state_t *state;

    if (! list || ! func)
	return FALSE;

    state = (egl_state_t *) list->data;

    if (! state ||
	! (state->display == EGL_NO_DISPLAY &&
	   state->context == EGL_NO_CONTEXT &&
	   state->readable == EGL_NO_SURFACE &&
	   state->drawable == EGL_NO_SURFACE)) {
	if (state && state->state.error == GL_NO_ERROR)
	    state->state.error = GL_INVALID_OPERATION;
	return FALSE;
    }

    if (state->state.error != GL_NO_ERROR)
	return FALSE;

    return TRUE;
}

/* GLES2 core profile API */
void glActiveTexture (GLenum texture)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    if (_is_error_state_or_func (active_state, dispatch.ActiveTexture))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (egl_state->state.active_texture == texture)
	goto FINISH;
    else if (texture > GL_TEXTURE31 || texture < GL_TEXTURE0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    /* XXX: create a command buffer */
    dispatch.ActiveTexture (texture);

    /* FIXME: this maybe not right because this texture may be invalid
     * object, we save here to save time in glGetError() */
    egl_state->state.active_texture = texture;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glAttachShader (GLuint program, GLuint shader)
{
    egl_state_t *egl_state;

    if (_is_error_state_or_func (active_state, dispatch.AttachShader))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create a command buffer */
    dispatch.AttachShader (program, shader);

    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

void glBindAttribLocation (GLuint program, GLuint index, const GLchar *name)
{
    gles2_state_t *state;
    egl_state_t *egl_state;

    if (_is_error_state_or_func (active_state,
				 dispatch.BindAttribLocation))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create a command buffer */
    dispatch.BindAttribLocation (program, index, name);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

void glBindBuffer (GLenum target, GLuint buffer)
{
    egl_state_t *egl_state;
    v_vertex_attrib_list_t *attrib_list = &cli_states.vertex_attribs;
    v_vertex_attrib_t *attribs = attrib_list->attribs;
    int count = attrib_list->count;
    int i;

    if (!(target == GL_ARRAY_BUFFER || target == GL_ELEMENT_ARRAY_BUFFER) ||
	! _is_error_state_or_func (active_state, dispatch.BindBuffer))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (target == GL_ARRAY_BUFFER &&
	buffer == egl_state->state.array_buffer_binding)
	goto FINISH;
    else if (target == GL_ELEMENT_ARRAY_BUFFER &&
	     buffer == egl_state->state.element_array_buffer_binding)
	goto FINISH;

    /* XXX: create a command buffer */
    dispatch.BindBuffer (target, buffer);

    egl_state->state.need_get_error = TRUE;

    /* FIXME: we don't know whether it succeeds or not */
    if (target == GL_ARRAY_BUFFER)
	egl_state->state.array_buffer_binding = buffer;
    else
	egl_state->state.element_array_buffer_binding = buffer;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);

    /* update client state */
    if (target == GL_ARRAY_BUFFER) {
	for (i = 0; i < count; i++) {
	    attribs[i].array_buffer_binding = buffer;
	}
    }
}

void glBindFramebuffer (GLenum target, GLuint framebuffer)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.BindFramebuffer))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (target == GL_FRAMEBUFFER &&
	egl_state->state.framebuffer_binding == framebuffer)
	goto FINISH;

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
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    /* XXX: create a command buffer, according to spec, no error generated */
    dispatch.BindFramebuffer (target, framebuffer);
    /* should we save it ? */
    egl_state->state.framebuffer_binding = framebuffer;

    //egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glBindRenderbuffer (GLenum target, GLuint renderbuffer)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (egl_state->mutex);

    if(! _is_error_state_or_func (active_state,
				  dispatch.BindRenderbuffer))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (target != GL_RENDERBUFFER) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }
    /* XXX: create a command buffer, according to spec, no error generated */
    dispatch.BindRenderbuffer (target, renderbuffer);
    //egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glBindTexture (GLenum target, GLuint texture)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.BindTexture))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (target == GL_TEXTURE_2D &&
	egl_state->state.texture_binding[0] == texture)
	goto FINISH;
    else if (target == GL_TEXTURE_CUBE_MAP &&
	     egl_state->state.texture_binding[1] == texture)
	goto FINISH;
#ifdef GL_OES_texture_3D
    else if (target == GL_TEXTURE_3D_OES &&
	     egl_state->state.texture_binding_3d == texture)
	goto FINISH;
#endif

    if (! (target == GL_TEXTURE_2D || target == GL_TEXTURE_CUBE_MAP
#ifdef GL_OES_texture_3D
	   || target == GL_TEXTURE_3D_OES
#endif
	  )) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }
    /* XXX: create a command buffer, according to spec, no error generated */
    dispatch.BindTexture (target, texture);
    egl_state->state.need_get_error = TRUE;

    /* FIXME: do we need to save them ? */
    if (target == GL_TEXTURE_2D)
	egl_state->state.texture_binding[0] = texture;
    else if (target == GL_TEXTURE_CUBE_MAP)
	egl_state->state.texture_binding[1] = texture;
#ifdef GL_OES_texture_3D
    else
	egl_state->state.texture_binding_3d = texture;
#endif

    //egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glBlendColor (GLclampf red, GLclampf green,
		   GLclampf blue, GLclampf alpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.BlendColor))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    state = &egl_state->state;

    if (state->blend_color[0] == red &&
	state->blend_color[1] == green &&
	state->blend_color[2] == blue &&
	state->blend_color[3] == alpha)
	goto FINISH;

    state->blend_color[0] = red;
    state->blend_color[1] = green;
    state->blend_color[2] = blue;
    state->blend_color[3] = alpha;

    /* XXX: command buffer */
    dispatch.BlendColor (red, green, blue, alpha);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glBlendEquation (GLenum mode)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.BlendEquation))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);
    state = &egl_state->state;

    if (! (mode == GL_FUNC_ADD ||
	   mode == GL_FUNC_SUBTRACT ||
	   mode == GL_FUNC_REVERSE_SUBTRACT)) {
	if (state->error == GL_NO_ERROR)
	    state->error = GL_INVALID_ENUM;
	goto FINISH;
    }

    if (state->blend_equation[0] == mode &&
	state->blend_equation[1] == mode)
	goto FINISH;

    state->blend_equation[0] = mode;
    state->blend_equation[1] = mode;

    /* XXX: command buffer */
    dispatch.BlendEquation (mode);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glBlendEquationSeparate (GLenum modeRGB, GLenum modeAlpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.BlendEquationSeparate))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    state = &egl_state->state;

    if (! (modeRGB == GL_FUNC_ADD ||
	   modeRGB == GL_FUNC_SUBTRACT ||
	   modeRGB == GL_FUNC_REVERSE_SUBTRACT) ||
    	! (modeAlpha == GL_FUNC_ADD ||
	   modeAlpha == GL_FUNC_SUBTRACT ||
	   modeAlpha == GL_FUNC_REVERSE_SUBTRACT)) {
	if (state->error == GL_NO_ERROR)
	    state->error = GL_INVALID_ENUM;
	goto FINISH;
    }

    if (state->blend_equation[0] == modeRGB &&
	state->blend_equation[1] == modeAlpha)
	goto FINISH;

    state->blend_equation[0] = modeRGB;
    state->blend_equation[1] = modeAlpha;

    /* XXX: command buffer */
    dispatch.BlendEquationSeparate (modeRGB, modeAlpha);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glBlendFunc (GLenum sfactor, GLenum dfactor)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.BlendFunc))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    state = &egl_state->state;

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
	if (state->error == GL_NO_ERROR)
	    state->error = GL_INVALID_ENUM;
	goto FINISH;
    }

    if (state->blend_src_rgb == sfactor &&
	state->blend_src_alpha == sfactor &&
	state->blend_dst_rgb == dfactor &&
	state->blend_dst_alpha == dfactor)
	goto FINISH;

    state->blend_src_rgb = state->blend_src_alpha = sfactor;
    state->blend_dst_rgb = state->blend_dst_alpha = dfactor;

    /* XXX: command buffer */
    dispatch.BlendFunc (sfactor, dfactor);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glBlendFuncSeparate (GLenum srcRGB, GLenum dstRGB,
			  GLenum srcAlpha, GLenum dstAlpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.BlendFuncSeparate))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    state = &egl_state->state;

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
	if (state->error == GL_NO_ERROR)
	    state->error = GL_INVALID_ENUM;
	goto FINISH;
    }

    if (state->blend_src_rgb == srcRGB &&
	state->blend_src_alpha == srcAlpha &&
	state->blend_dst_rgb == dstRGB &&
	state->blend_dst_alpha == dstAlpha)
	goto FINISH;

    state->blend_src_rgb = srcRGB;
    state->blend_src_alpha = srcAlpha;
    state->blend_dst_rgb = dstRGB;
    state->blend_dst_alpha = dstAlpha;

    /* XXX: command buffer */
    dispatch.BlendFuncSeparate (srcRGB, dstRGB, srcAlpha, dstAlpha);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glBufferData (GLenum target, GLsizeiptr size,
		   const GLvoid *data, GLenum usage)
{
    egl_state_t *egl_state;

    if (! _is_error_state_or_func (active_state, dispatch.BufferData))
	return;

    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create a command buffer, we skip rest of check, because driver
     * can generate GL_OUT_OF_MEMORY, which cannot check
     */
    dispatch.BufferData (target, size, data, usage);
    egl_state = (egl_state_t *) active_state->data;
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

void glBufferSubData (GLenum target, GLintptr offset,
		      GLsizeiptr size, const GLvoid *data)
{
    egl_state_t *egl_state;

    if (! _is_error_state_or_func (active_state, dispatch.BufferSubData))
	return;

    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create a command buffer, we skip rest of check, because driver
     * can generate GL_INVALID_VALUE, when offset + data can be out of
     * bound
     */
    dispatch.BufferSubData (target, offset, size, data);
    egl_state = (egl_state_t *) active_state->data;
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

GLenum glCheckFramebufferStatus (GLenum target)
{
    GLenum result = GL_INVALID_ENUM;
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.CheckFramebufferStatus))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

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
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    /* XXX: create a command buffer, no need to set error code */
    dispatch.CheckFramebufferStatus (target);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
    return result;
}

void glClear (GLbitfield mask)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.Clear))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (! (mask && GL_COLOR_BUFFER_BIT ||
	   mask && GL_DEPTH_BUFFER_BIT ||
	   mask && GL_STENCIL_BUFFER_BIT)) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    /* XXX: create a command buffer, no need to set error code */
    dispatch.Clear (mask);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glClearColor (GLclampf red, GLclampf green,
		   GLclampf blue, GLclampf alpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.ClearColor))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    state = &egl_state->state;

    if (state->color_clear_value[0] == red &&
	state->color_clear_value[1] == green &&
	state->color_clear_value[2] == blue &&
	state->color_clear_value[3] == alpha)
	goto FINISH;

    state->color_clear_value[0] = red;
    state->color_clear_value[1] = green;
    state->color_clear_value[2] = blue;
    state->color_clear_value[3] = alpha;

    /* XXX: command buffer */
    dispatch.ClearColor (red, green, blue, alpha);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glClearDepthf (GLclampf depth)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.ClearDepthf))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    state = &egl_state->state;

    if (state->depth_clear_value == depth)
	goto FINISH;

    state->depth_clear_value = depth;

    /* XXX: command buffer */
    dispatch.ClearDepthf (depth);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glClearStencil (GLint s)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.ClearStencil))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    state = &egl_state->state;

    if (state->stencil_clear_value == s)
	goto FINISH;

    state->stencil_clear_value = s;

    /* XXX: command buffer */
    dispatch.ClearStencil (s);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glColorMask (GLboolean red, GLboolean green,
		  GLboolean blue, GLboolean alpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.ColorMask))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    state = &egl_state->state;

    if (state->color_writemask[0] == red &&
	state->color_writemask[1] == green &&
	state->color_writemask[2] == blue &&
	state->color_writemask[3] == alpha)
	goto FINISH;

    state->color_writemask[0] = red;
    state->color_writemask[1] = green;
    state->color_writemask[2] = blue;
    state->color_writemask[3] = alpha;

    /* XXX: command buffer */
    dispatch.ColorMask (red, green, blue, alpha);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glCompressedTexImage2D (GLenum target, GLint level,
			     GLenum internalformat,
			     GLsizei width, GLsizei height,
			     GLint border, GLsizei imageSize,
			     const GLvoid *data)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.CompressedTexImage2D))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);
    /* XXX: command buffer */
    dispatch.CompressedTexImage2D (target, level, internalformat,
				   width, height, border, imageSize,
				   data);

    egl_state->state.need_get_error = TRUE;
    gpu_mutex_unlock (egl_state->mutex);
}

void glCompressedTexSubImage2D (GLenum target, GLint level,
				GLint xoffset, GLint yoffset,
				GLsizei width, GLsizei height,
				GLenum format, GLsizei imageSize,
				const GLvoid *data)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.CompressedTexSubImage2D))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: command buffer */
    dispatch.CompressedTexSubImage2D (target, level, xoffset, yoffset,
				      width, height, format, imageSize,
				      data);

    egl_state->state.need_get_error = TRUE;
    gpu_mutex_unlock (egl_state->mutex);
}

void glCopyTexImage2D (GLenum target, GLint level,
		       GLenum internalformat,
		       GLint x, GLint y,
		       GLsizei width, GLsizei height,
		       GLint border)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.CopyTexImage2D))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: command buffer */
    dispatch.CopyTexImage2D (target, level, internalformat,
			     x, y, width, height, border);

    egl_state->state.need_get_error = TRUE;
    gpu_mutex_unlock (egl_state->mutex);
}

void glCopyTexSubImage2D (GLenum target, GLint level,
			  GLint xoffset, GLint yoffset,
			  GLint x, GLint y,
			  GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.CopyTexSubImage2D))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: command buffer */
    dispatch.CopyTexSubImage2D (target, level, xoffset, yoffset,
			     x, y, width, height);

    egl_state->state.need_get_error = TRUE;
    gpu_mutex_unlock (egl_state->mutex);
}

/* This is a sync call */
GLuint glCreateProgram (void)
{
    GLuint result = 0;
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.CreateProgram))
	return result;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: command buffer and wait for signal, no error code generated */
    result = dispatch.CreateProgram ();

    gpu_mutex_unlock (egl_state->mutex);

    return result;
}

GLuint glCreateShader (GLenum shaderType)
{
    egl_state_t *egl_state;

    GLuint result = 0;

    if(! _is_error_state_or_func (active_state,
				  dispatch.CreateShader))
	return result;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (! (shaderType == GL_VERTEX_SHADER ||
	   shaderType == GL_FRAGMENT_SHADER)) {
	egl_state = (egl_state_t *) active_state->data;
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error == GL_INVALID_ENUM;
	goto FINISH;
    }

    /* XXX: command buffer and wait for signal, no error code generated */
    result = dispatch.CreateShader (shaderType);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);

    return result;
}

void glCullFace (GLenum mode)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.CullFace))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (egl_state->state.cull_face_mode == mode)
	goto FINISH;

    if (! (mode == GL_FRONT ||
	   mode == GL_BACK ||
	   mode == GL_FRONT_AND_BACK)) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error == GL_INVALID_ENUM;
	goto FINISH;
    }

    egl_state->state.cull_face_mode = mode;

    /* XXX: command buffer, no error code generated */
    dispatch.CullFace (mode);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glDeleteBuffers (GLsizei n, const GLuint *buffers)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.DeleteBuffers))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (n < 0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error == GL_INVALID_VALUE;
	goto FINISH;
    }

    /* XXX: command buffer, no error code generated */
    dispatch.DeleteBuffers (n, buffers);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glDeleteFramebuffers (GLsizei n, const GLuint *framebuffers)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.DeleteFramebuffers))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (n < 0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error == GL_INVALID_VALUE;
	goto FINISH;
    }

    /* XXX: command buffer, no error code generated */
    dispatch.DeleteFramebuffers (n, framebuffers);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glDeleteProgram (GLuint program)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.DeleteProgram))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: command buffer */
    dispatch.DeleteProgram (program);

    egl_state->state.need_get_error = TRUE;
    gpu_mutex_unlock (egl_state->mutex);
}

void glDeleteRenderbuffers (GLsizei n, const GLuint *renderbuffers)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.DeleteRenderbuffers))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (n < 0) {

	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error == GL_INVALID_VALUE;
	goto FINISH;
    }

    /* XXX: command buffer, no error code generated */
    dispatch.DeleteRenderbuffers (n, renderbuffers);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glDeleteShader (GLuint shader)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.DeleteShader))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: command buffer */
    dispatch.DeleteShader (shader);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

void glDeleteTextures (GLsizei n, const GLuint *textures)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.DeleteTextures))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (n < 0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error == GL_INVALID_VALUE;
	goto FINISH;
    }

    /* XXX: command buffer, no error code generated */
    dispatch.DeleteTextures (n, textures);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glDepthFunc (GLenum func)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.DepthFunc))
	    return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (egl_state->state.depth_func == func)
	goto FINISH;

    if (! (func == GL_NEVER ||
	   func == GL_LESS ||
	   func == GL_EQUAL ||
	   func == GL_LEQUAL ||
	   func == GL_GREATER ||
	   func == GL_NOTEQUAL ||
	   func == GL_GEQUAL ||
	   func == GL_ALWAYS)) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_VALUE;
	goto FINISH;
    }

    egl_state->state.depth_func = func;

    /* XXX: command buffer, no error code generated */
    dispatch.DepthFunc (func);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glDepthMask (GLboolean flag)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.DepthMask))
	    return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (egl_state->state.depth_writemask == flag)
	goto FINISH;

    egl_state->state.depth_writemask = flag;

    /* XXX: command buffer, no error code generated */
    dispatch.DepthMask (egl_state->state.depth_writemask);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glDepthRangef (GLclampf nearVal, GLclampf farVal)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.DepthRangef))
	    return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (egl_state->state.depth_range[0] == nearVal &&
	egl_state->state.depth_range[1] == farVal)
	goto FINISH;

    egl_state->state.depth_range[0] = nearVal;
    egl_state->state.depth_range[1] = farVal;

    /* XXX: command buffer, no error code generated */
    dispatch.DepthRangef (nearVal, farVal);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glDetachShader (GLuint program, GLuint shader)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.DetachShader))
	    return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: command buffer, error code generated */
    dispatch.DetachShader (program, shader);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

static void
_gl_set_cap (GLenum cap, GLboolean enable)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    v_bool_t needs_call = FALSE;

    if(! _is_error_state_or_func (active_state,
				  dispatch.Disable))
	    return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);
    state = &egl_state->state;

    switch (cap) {
    case GL_BLEND:
	if (state->blend != enable) {
	    state->blend = enable;
	    needs_call = TRUE;
	}
	break;
    case GL_CULL_FACE:
	if (state->cull_face != enable) {
	    state->cull_face = enable;
	    needs_call = TRUE;
	}
	break;
    case GL_DEPTH_TEST:
	if (state->depth_test != enable) {
	    state->depth_test = enable;
	    needs_call = TRUE;
	}
	break;
    case GL_DITHER:
	if (state->dither != enable) {
	    state->dither = enable;
	    needs_call = TRUE;
	}
	break;
    case GL_POLYGON_OFFSET_FILL:
	if (state->polygon_offset_fill != enable) {
	    state->polygon_offset_fill = enable;
	    needs_call = TRUE;
	}
	break;
    case GL_SAMPLE_ALPHA_TO_COVERAGE:
	if (state->sample_alpha_to_coverage != enable) {
	    state->sample_alpha_to_coverage = enable;
	    needs_call = TRUE;
	}
	break;
    case GL_SAMPLE_COVERAGE:
	if (state->sample_coverage != enable) {
	    state->sample_coverage = enable;
	    needs_call = TRUE;
	}
	break;
    case GL_SCISSOR_TEST:
	if (state->scissor_test != enable) {
	    state->scissor_test = enable;
	    needs_call = TRUE;
	}
	break;
    case GL_STENCIL_TEST:
	if (state->stencil_test != enable) {
	    state->stencil_test = enable;
	    needs_call = TRUE;
	}
	break;
    default:
	needs_call = TRUE;
	state->need_get_error = TRUE;
	break;
    }

    /* XXX: command buffer, no error generate */
    if (needs_call) {
	if (enable)
	    dispatch.Enable (cap);
	else
	    dispatch.Disable (cap);
    }

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glDisable (GLenum cap)
{
    _gl_set_cap (cap, GL_FALSE);
}

void glEnable (GLenum cap)
{
    _gl_set_cap (cap, GL_TRUE);
}

static v_bool_t
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
	    return TRUE;
	}
    }

    return FALSE;
}

void glDisableVertexAttribArray (GLuint index)
{
    egl_state_t *egl_state;

    v_vertex_attrib_list_t *attrib_list = &cli_states.vertex_attribs;
    v_vertex_attrib_t *attribs = attrib_list->attribs;
    int count = attrib_list->count;
    int i, found_index = -1;

    GLint bound_buffer = 0;

    /* look into client state */
    for (i = 0; i < count; i++) {
	if (attribs[i].index == index) {
	    if (attribs[i].array_enabled == GL_FALSE)
		return;
	    else {
		found_index = i;
		attribs[i].array_enabled = GL_FALSE;
		break;
	    }
	}
    }

    if(! _is_error_state_or_func (active_state,
				  dispatch.DisableVertexAttribArray))
	    return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* gles2 spec says at least 8 */
    /* XXX: command buffer */
    if (_gl_index_is_too_large (&egl_state->state, index)) {
	gpu_mutex_unlock (egl_state->mutex);
	return;
    }

    /* XXX: command buffer, error code generated */
    dispatch.DisableVertexAttribArray (index);

    bound_buffer = egl_state->state.array_buffer_binding;
    gpu_mutex_unlock (egl_state->mutex);

    /* update client state */
    if (found_index != -1)
	return;

    if (i < NUM_EMBEDDED) {
	attribs[i].array_enabled = GL_FALSE;
	attribs[i].index = index;
	attribs[i].size = 0;
	attribs[i].stride = 0;
	attribs[i].type = GL_FLOAT;
	attribs[i].array_normalized = GL_FALSE;
	attribs[i].pointer = NULL;
	attribs[i].data = NULL;
	memset (attribs[i].current_attrib, 0, sizeof (GLfloat) * 4);
	attribs[i].array_buffer_binding = bound_buffer;
	attrib_list->count ++;
    }
    else {
	v_vertex_attrib_t *new_attribs = (v_vertex_attrib_t *)malloc (sizeof (v_vertex_attrib_t) * (count + 1));

	memcpy (new_attribs, attribs, (count+1) * sizeof (v_vertex_attrib_t));
	if (attribs != attrib_list->embedded_attribs)
	    free (attribs);

	new_attribs[count].array_enabled = GL_FALSE;
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

void glEnableVertexAttribArray (GLuint index)
{
    egl_state_t *egl_state;

    v_vertex_attrib_list_t *attrib_list = &cli_states.vertex_attribs;
    v_vertex_attrib_t *attribs = attrib_list->attribs;
    int count = attrib_list->count;
    int i, found_index = -1;
    GLint bound_buffer = 0;

    /* look into client state */
    for (i = 0; i < count; i++) {
	if (attribs[i].index == index) {
	    if (attribs[i].array_enabled == GL_TRUE)
		return;
	    else {
		found_index = i;
		attribs[i].array_enabled = GL_TRUE;
		break;
	    }
	}
    }

    if(! _is_error_state_or_func (active_state,
				  dispatch.EnableVertexAttribArray))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);
    /* gles2 spec says at least 8 */
    /* XXX: command buffer */
    if (_gl_index_is_too_large (&egl_state->state, index)) {
	gpu_mutex_unlock (egl_state->mutex);
	return;
    }

    /* XXX: command buffer, error code generated */
    dispatch.EnableVertexAttribArray (index);

    bound_buffer = egl_state->state.array_buffer_binding;

    gpu_mutex_unlock (egl_state->mutex);

    /* update client state */
    if (found_index != -1)
	return;

    if (i < NUM_EMBEDDED) {
	attribs[i].array_enabled = GL_TRUE;
	attribs[i].index = index;
	attribs[i].size = 0;
	attribs[i].stride = 0;
	attribs[i].type = GL_FLOAT;
	attribs[i].array_normalized = GL_FALSE;
	attribs[i].pointer = NULL;
	attribs[i].data = NULL;
	attribs[i].array_buffer_binding = bound_buffer;
	memset (attribs[i].current_attrib, 0, sizeof (GLfloat) * 4);
    }
    else {
	v_vertex_attrib_t *new_attribs = (v_vertex_attrib_t *)malloc (sizeof (v_vertex_attrib_t) * (count + 1));

	memcpy (new_attribs, attribs, (count+1) * sizeof (v_vertex_attrib_t));
	if (attribs != attrib_list->embedded_attribs)
	    free (attribs);

	new_attribs[count].array_enabled = GL_TRUE;
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

/* FIXME: we should use pre-allocated buffer if possible */
static char *
_gl_create_data_array (v_vertex_attrib_t *attrib, int count)
{
    int i;
    char *data;

    if(attrib->type == GL_BYTE || attrib->type == GL_UNSIGNED_BYTE) {
	data = (char *)malloc (sizeof (char) * count * attrib->size);

     	for (i = 0; i < count; i++)
		memcpy (data + i * attrib->size,
			attrib->pointer + attrib->stride * i, attrib->size);
    }
    else if (attrib->type == GL_SHORT || attrib->type == GL_UNSIGNED_SHORT) {
	data = (char *)malloc (sizeof (short) * count * attrib->size);

     	for (i = 0; i < count; i++)
		memcpy (attrib->data + i * attrib->size,
			attrib->pointer + attrib->stride * i,
			attrib->size * sizeof (short));
    }
    else if (attrib->type == GL_FLOAT || attrib->type == GL_FIXED) {
	data = (char *)malloc (sizeof (float) * count * attrib->size);

     	for (i = 0; i < count; i++)
		memcpy (attrib->data + i * attrib->size,
			attrib->pointer + attrib->stride * i,
			attrib->size * sizeof (float));
    }

    return data;
}

void glDrawArrays (GLenum mode, GLint first, GLsizei count)
{
    egl_state_t *egl_state;
    char *data;
    v_link_list_t *array_data = NULL;
    v_link_list_t *array, *new_array_data;

    v_vertex_attrib_list_t *attrib_list = &cli_states.vertex_attribs;
    v_vertex_attrib_t *attribs = attrib_list->attribs;
    int i, found_index = -1;
    int n = 0;

    if(! _is_error_state_or_func (active_state,
				  dispatch.DrawArrays))
	return;

    egl_state = (egl_state_t *)active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (mode != GL_POINTS && mode != GL_LINE_STRIP &&
	mode != GL_LINE_LOOP && mode != GL_LINES &&
	mode != GL_TRIANGLE_STRIP && mode != GL_TRIANGLE_FAN &&
	mode != GL_TRIANGLES) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	gpu_mutex_unlock (egl_state->mutex);
	return;
    }
    else if (count < 0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_VALUE;
	gpu_mutex_unlock (egl_state->mutex);
	return;
    }

#ifdef GL_OES_vertex_array_object
    if (egl_state->state.vertex_array_binding) {
	dispatch.DrawArrays (mode, first, count);
	egl_state->state.need_get_error = TRUE;
	gpu_mutex_unlock (egl_state->mutex);
	return;
    }
#endif

    /* check buffer binding, if there is a buffer, we don't need to
     * copy data
     */
    for (i = 0; i < attrib_list->count; i++) {
	if (! attribs[i].array_enabled)
	    continue;
	else if (egl_state->state.array_buffer_binding) {
	    dispatch.VertexAttribPointer (attribs[i].index,
					  attribs[i].size,
					  attribs[i].type,
					  attribs[i].array_normalized,
					  attribs[i].stride,
					  attribs[i].pointer);
    	    egl_state->state.need_get_error = TRUE;
	}
 	/* we need to create a separate buffer for it */
	else {
	    data = _gl_create_data_array (&attribs[i], count);
	    dispatch.VertexAttribPointer (attribs[i].index,
					  attribs[i].size,
					  attribs[i].type,
					  attribs[i].array_normalized,
					  0,
					  data);
	    if (array_data == NULL) {
		array_data = (v_link_list_t *)malloc (sizeof (v_link_list_t));
		array_data->prev = NULL;
		array_data->next = NULL;
		array_data->data = data;
	    }
	    else {
		array = array_data;
		while (array->next)
		    array = array->next;

		new_array_data = (v_link_list_t *)malloc (sizeof (v_link_list_t));
		new_array_data->prev = array;
		new_array_data->next = NULL;
		array_data->next = new_array_data;
		new_array_data->data = data;
	    }
	}
    }

    // we need put DrawArrays
    dispatch.DrawArrays (mode, first, count);

    array = array_data;
    while (array != NULL) {
	new_array_data = array;
	array = array->next;
	free (new_array_data->data);
	free (new_array_data);
    }


    gpu_mutex_unlock (egl_state->mutex);
}

/* FIXME: we should use pre-allocated buffer if possible */
static char *
_gl_create_indices_array (GLenum mode, GLenum type, int count,
			  char *indices)
{
    char *data;
    int length;

    if(mode == GL_POINTS) {
	if (type == GL_UNSIGNED_BYTE) {
	    length = sizeof (char) * count;
	    data = (char *)malloc (length);
	    memcpy (data, indices, length);
	}
	else if (type == GL_UNSIGNED_SHORT) {
	    length = sizeof (unsigned short) * count;
	    data = (char *)malloc (length);
	    memcpy (data, indices, length);
	}
    }
    else if (mode == GL_LINE_STRIP) {
	if (type == GL_UNSIGNED_BYTE) {
	    length = sizeof (char) * (count + 1);
	    data = (char *)malloc (length);
	    memcpy (data, indices, length);
	}
	else if (type == GL_UNSIGNED_SHORT) {
	    length = sizeof (unsigned short) * (count + 1);
	    data = (char *)malloc (length);
	    memcpy (data, indices, length);
	}
    }
    else if (mode == GL_LINE_LOOP) {
	if (type == GL_UNSIGNED_BYTE) {
	    length = sizeof (char) * count;
	    data = (char *)malloc (length);
	    memcpy (data, indices, length);
	}
	else if (type == GL_UNSIGNED_SHORT) {
	    length = sizeof (unsigned short) * count;
	    data = (char *)malloc (length);
	    memcpy (data, indices, length);
	}
    }
    else if (mode == GL_LINES) {
	if (type == GL_UNSIGNED_BYTE) {
	    length = sizeof (char) * count * 2;
	    data = (char *)malloc (length);
	    memcpy (data, indices, length);
	}
	else if (type == GL_UNSIGNED_SHORT) {
	    length = sizeof (unsigned short) * count * 2;
	    data = (char *)malloc (length);
	    memcpy (data, indices, length);
	}
    }
    else if (mode == GL_TRIANGLE_STRIP || mode == GL_TRIANGLE_FAN) {
	if (type == GL_UNSIGNED_BYTE) {
	    length = sizeof (char) * (count + 2);
	    data = (char *)malloc (length);
	    memcpy (data, indices, length);
	}
	else if (type == GL_UNSIGNED_SHORT) {
	    length = sizeof (unsigned short) * (count + 2);
	    data = (char *)malloc (length);
	    memcpy (data, indices, length);
	}
    }
    else if (mode == GL_TRIANGLES) {
	if (type == GL_UNSIGNED_BYTE) {
	    length = sizeof (char) * (count * 3);
	    data = (char *)malloc (length);
	    memcpy (data, indices, length);
	}
	else if (type == GL_UNSIGNED_SHORT) {
	    length = sizeof (unsigned short) * (count * 3);
	    data = (char *)malloc (length);
	    memcpy (data, indices, length);
	}
    }

    return data;
}

void glDrawElements (GLenum mode, GLsizei count, GLenum type,
		     const GLvoid *indices)
{
    egl_state_t *egl_state;
    char *data;
    v_link_list_t *array_data = NULL;
    v_link_list_t *array, *new_array_data;

    v_vertex_attrib_list_t *attrib_list = &cli_states.vertex_attribs;
    v_vertex_attrib_t *attribs = attrib_list->attribs;
    int i, found_index = -1;
    int n = 0;

    if(! _is_error_state_or_func (active_state,
				  dispatch.DrawElements))
	return;

    egl_state = (egl_state_t *)active_state->data;
    gpu_mutex_lock (egl_state->mutex);

#ifdef GL_OES_vertex_array_object
    if (egl_state->state.vertex_array_binding) {
	dispatch.DrawElements (mode, count, type, indices);
	egl_state->state.need_get_error = TRUE;
	gpu_mutex_unlock (egl_state->mutex);
	return;
    }
#endif

    if (mode != GL_POINTS && mode != GL_LINE_STRIP &&
	mode != GL_LINE_LOOP && mode != GL_LINES &&
	mode != GL_TRIANGLE_STRIP && mode != GL_TRIANGLE_FAN &&
	mode != GL_TRIANGLES) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	gpu_mutex_unlock (egl_state->mutex);
	return;
    }
    else if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	gpu_mutex_unlock (egl_state->mutex);
	return;
    }
    else if (count < 0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_VALUE;
	gpu_mutex_unlock (egl_state->mutex);
	return;
    }

    /* check buffer binding, if there is a buffer, we don't need to 
     * copy data
     */

    for (i = 0; i < attrib_list->count; i++) {
	if (! attribs[i].array_enabled)
	    continue;
	else if (egl_state->state.array_buffer_binding) {
	    dispatch.VertexAttribPointer (attribs[i].index,
					  attribs[i].size,
					  attribs[i].type,
					  attribs[i].array_normalized,
					  attribs[i].stride,
					  attribs[i].pointer);
    	    egl_state->state.need_get_error = TRUE;
	}
 	/* we need to create a separate buffer for it */
	else {
	    data = _gl_create_data_array (&attribs[i], count);
	    dispatch.VertexAttribPointer (attribs[i].index,
					  attribs[i].size,
					  attribs[i].type,
					  attribs[i].array_normalized,
					  0,
					  data);
	    if (array_data == NULL) {
		array_data = (v_link_list_t *)malloc (sizeof (v_link_list_t));
		array_data->prev = NULL;
		array_data->next = NULL;
		array_data->data = data;
	    }
	    else {
		array = array_data;
		while (array->next)
		    array = array->next;

		new_array_data = (v_link_list_t *)malloc (sizeof (v_link_list_t));
		new_array_data->prev = array;
		new_array_data->next = NULL;
		array_data->next = new_array_data;
		new_array_data->data = data;
	    }
	}
    }

    /* create indices, when not using element array buffer */
    if (egl_state->state.element_array_buffer_binding == 0) {
	data = _gl_create_indices_array (mode, type, count, (char *)indices);

	// we need put DrawArrays
 	dispatch.DrawElements (mode, type, count, (const GLvoid *)data);
    }
    else {
	data = NULL;
	dispatch.DrawElements (mode, type, count, indices);
    }

    array = array_data;
    while (array != NULL) {
	new_array_data = array;
	array = array->next;
	free (new_array_data->data);
	free (new_array_data);
    }
    if (data)
	free (data);

    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

void glFinish (void)
{
    egl_state_t *egl_state;
    /* XXX: put to a command buffer, wait for signal */
    if(! _is_error_state_or_func (active_state,
				  dispatch.Finish))
	return;

    egl_state = (egl_state_t *)active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    dispatch.Finish ();

    gpu_mutex_unlock (egl_state->mutex);
}

void glFlush (void)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.Flush))
	return;

    egl_state = (egl_state_t *)active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    dispatch.Flush ();

    gpu_mutex_unlock (egl_state->mutex);
}

void glFramebufferRenderbuffer (GLenum target, GLenum attachment,
				GLenum renderbuffertarget,
				GLenum renderbuffer)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.FramebufferRenderbuffer))
	return;

    egl_state = (egl_state_t *) active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    if (target != GL_FRAMEBUFFER) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }
    else if (renderbuffertarget != GL_RENDERBUFFER &&
	    renderbuffer != 0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    dispatch.FramebufferRenderbuffer (target, attachment,
				      renderbuffertarget, renderbuffer);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glFramebufferTexture2D (GLenum target, GLenum attachment,
			     GLenum textarget, GLuint texture, GLint level)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.FramebufferTexture2D))
	return;

    egl_state = (egl_state_t *) active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    if (target != GL_FRAMEBUFFER) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    dispatch.FramebufferTexture2D (target, attachment, textarget,
				   texture, level);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glFrontFace (GLenum mode)
{
    egl_state_t *egl_state;


    if(! _is_error_state_or_func (active_state,
				  dispatch.FrontFace))
	return;

    egl_state = (egl_state_t *) active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    if (! (mode == GL_CCW || mode == GL_CW)) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    if (egl_state->state.front_face != mode) {
	/* XXX: create a command buffer */
	egl_state->state.front_face = mode;
	dispatch.FrontFace (mode);
    }

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glGenBuffers (GLsizei n, GLuint *buffers)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GenBuffers))
	return;

    egl_state = (egl_state_t *) active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    if (n < 0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_VALUE;
	goto FINISH;
    }

    /* XXX: create a command buffer and wait for signal */
    dispatch.GenBuffers (n, buffers);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glGenFramebuffers (GLsizei n, GLuint *framebuffers)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GenFramebuffers))
	return;

    egl_state = (egl_state_t *) active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    if (n < 0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_VALUE;
	goto FINISH;
    }

    /* XXX: create a command buffer and wait for signal */
    dispatch.GenFramebuffers (n, framebuffers);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glGenRenderbuffers (GLsizei n, GLuint *renderbuffers)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GenRenderbuffers))
	return;

    egl_state = (egl_state_t *) active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    if (n < 0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_VALUE;
	goto FINISH;
    }

    /* XXX: create a command buffer and wait for signal */
    dispatch.GenRenderbuffers (n, renderbuffers);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glGenTextures (GLsizei n, GLuint *textures)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GenTextures))
	return;

    egl_state = (egl_state_t *)active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    if (n < 0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_VALUE;
	goto FINISH;
    }

    /* XXX: create a command buffer and wait for signal */
    dispatch.GenTextures (n, textures);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glGenerateMipmap (GLenum target)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GenerateMipmap))
	return;

    egl_state = (egl_state_t *) active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    if (! (target == GL_TEXTURE_2D || target == GL_TEXTURE_CUBE_MAP
#ifdef GL_OES_texture_3D
	|| target == GL_TEXTURE_3D_OES
#endif
	)) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_VALUE;
	goto FINISH;
    }

    /* XXX: create a command buffer and wait for signal */
    dispatch.GenerateMipmap (target);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glGetBooleanv (GLenum pname, GLboolean *params)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetBooleanv))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

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
	/* XXX: command buffer, and wait for signal */
	dispatch.GetBooleanv (pname, params);
	break;
    }

    gpu_mutex_unlock (egl_state->mutex);
}

void glGetFloatv (GLenum pname, GLfloat *params)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetFloatv))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    switch (pname) {
    case GL_BLEND_COLOR:
	memcpy (params, egl_state->state.blend_color, sizeof (GLfloat) * 4);
	break;
    case GL_BLEND_DST_ALPHA:
	*params = egl_state->state.blend_dst_alpha;
	break;
    case GL_BLEND_DST_RGB:
	*params = egl_state->state.blend_dst_rgb;
	break;
    case GL_BLEND_EQUATION_ALPHA:
	*params = egl_state->state.blend_equation[1];
	break;
    case GL_BLEND_EQUATION_RGB:
	*params = egl_state->state.blend_equation[0];
	break;
    case GL_BLEND_SRC_ALPHA:
	*params = egl_state->state.blend_src_alpha;
	break;
    case GL_BLEND_SRC_RGB:
	*params = egl_state->state.blend_src_rgb;
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
	/* XXX: command buffer, and wait for signal */
	dispatch.GetFloatv (pname, params);
	break;
    }
FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glGetIntegerv (GLenum pname, GLint *params)
{
    egl_state_t *egl_state;
    v_vertex_attrib_list_t *attrib_list = &cli_states.vertex_attribs;
    v_vertex_attrib_t *attribs = attrib_list->attribs;
    int count = attrib_list->count;
    int i;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetIntegerv))
	return;

    egl_state = (egl_state_t *)active_state->data;
    gpu_mutex_lock (egl_state->mutex);

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
	if (egl_state->state.max_combined_texture_image_units == 8) {
	    dispatch.GetIntegerv (pname, params);
	    egl_state->state.max_combined_texture_image_units = *params;
	} else
	    *params = egl_state->state.max_combined_texture_image_units;
	break;
    case GL_MAX_CUBE_MAP_TEXTURE_SIZE:
	if (egl_state->state.max_cube_map_texture_size == 16) {
	    dispatch.GetIntegerv (pname, params);
	    egl_state->state.max_cube_map_texture_size = *params;
	} else
	    *params = egl_state->state.max_cube_map_texture_size;
	break;
    case GL_MAX_FRAGMENT_UNIFORM_VECTORS:
	if (egl_state->state.max_fragment_uniform_vectors == 16) {
	    dispatch.GetIntegerv (pname, params);
	    egl_state->state.max_fragment_uniform_vectors = *params;
	} else
	    *params = egl_state->state.max_fragment_uniform_vectors;
	break;
    case GL_MAX_RENDERBUFFER_SIZE:
	if (egl_state->state.max_renderbuffer_size == 1) {
	    dispatch.GetIntegerv (pname, params);
	    egl_state->state.max_renderbuffer_size = *params;
	} else
	    *params = egl_state->state.max_renderbuffer_size;
	break;
    case GL_MAX_TEXTURE_IMAGE_UNITS:
	if (egl_state->state.max_texture_image_units == 8) {
	    dispatch.GetIntegerv (pname, params);
	    egl_state->state.max_texture_image_units = *params;
	} else
	    *params = egl_state->state.max_texture_image_units;
	break;
    case GL_MAX_VARYING_VECTORS:
	if (egl_state->state.max_varying_vectors == 8) {
	    dispatch.GetIntegerv (pname, params);
	    egl_state->state.max_varying_vectors = *params;
	} else
	    *params = egl_state->state.max_varying_vectors;
	break;
    case GL_MAX_TEXTURE_SIZE:
	if (egl_state->state.max_texture_size == 64) {
	    dispatch.GetIntegerv (pname, params);
	    egl_state->state.max_texture_size = *params;
	} else
	    *params = egl_state->state.max_texture_size;
	break;
    case GL_MAX_VERTEX_ATTRIBS:
	if (! egl_state->state.max_vertex_attribs_queried) {
	    dispatch.GetIntegerv (pname, params);
	    egl_state->state.max_vertex_attribs = *params;
	} else
	    *params = egl_state->state.max_vertex_attribs;
	break;
    case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
	if (egl_state->state.max_vertex_texture_image_units == 0) {
	    dispatch.GetIntegerv (pname, params);
	    egl_state->state.max_vertex_texture_image_units = *params;
	} else
	    *params = egl_state->state.max_vertex_texture_image_units;
	break;
    case GL_MAX_VERTEX_UNIFORM_VECTORS:
	if (egl_state->state.max_vertex_uniform_vectors == 0) {
	    dispatch.GetIntegerv (pname, params);
	    egl_state->state.max_vertex_uniform_vectors = *params;
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
	/* XXX: command buffer, and wait for signal */
	dispatch.GetIntegerv (pname, params);
	break;
    }

FINISH:
    if (pname == GL_ARRAY_BUFFER_BINDING) {
	egl_state->state.array_buffer_binding = *params;
	/* update client state */
	for (i = 0; i < count; i++) {
	    attribs[i].array_buffer_binding = *params;
	}
    }
    else if (pname = GL_ELEMENT_ARRAY_BUFFER_BINDING)
	egl_state->state.array_buffer_binding = *params;
#ifdef GL_OES_vertex_array_object
    else if (pname = GL_VERTEX_ARRAY_BINDING_OES)
	egl_state->state.vertex_array_binding = *params;
#endif
    gpu_mutex_unlock (egl_state->mutex);
}

void glGetActiveAttrib (GLuint program, GLuint index,
			GLsizei bufsize, GLsizei *length,
			GLint *size, GLenum *type, GLchar *name)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetActiveAttrib))
	return;

    egl_state = (egl_state_t *)active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetActiveAttrib (program, index, bufsize, length,
			      size, type, name);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

void glGetActiveUniform (GLuint program, GLuint index, GLsizei bufsize,
			 GLsizei *length, GLint *size, GLenum *type,
			 GLchar *name)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetActiveUniform))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetActiveUniform (program, index, bufsize, length,
			       size, type, name);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

void glGetAttachedShaders (GLuint program, GLsizei maxCount,
			   GLsizei *count, GLuint *shaders)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetAttachedShaders))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetAttachedShaders (program, maxCount, count, shaders);

    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GLint glGetAttribLocation (GLuint program, const GLchar *name)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetAttribLocation))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetAttribLocation (program, name);

    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

void glGetBufferParameteriv (GLenum target, GLenum value, GLint *data)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetBufferParameteriv))
	return;

    egl_state = (egl_state_t *)active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetBufferParameteriv (target, value, data);

    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GLenum glGetError (void)
{
    egl_state_t *egl_state;

    GLenum error = GL_INVALID_OPERATION;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetError))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (! egl_state->state.need_get_error) {
	error = egl_state->state.error;
	egl_state->state.error = GL_NO_ERROR;
	goto FINISH;
    }

    /* XXX: create a command buffer and wait for signal */
    error = dispatch.GetError ();

    egl_state->state.need_get_error = FALSE;
    egl_state->state.error = GL_NO_ERROR;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);

    return error;
}

void glGetFramebufferAttachmentParameteriv (GLenum target,
					    GLenum attachment,
					    GLenum pname,
					    GLint *params)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetFramebufferAttachmentParameteriv))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (target != GL_FRAMEBUFFER) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetFramebufferAttachmentParameteriv (target, attachment,
						  pname, params);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glGetProgramInfoLog (GLuint program, GLsizei maxLength,
			  GLsizei *length, GLchar *infoLog)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetProgramInfoLog))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetProgramInfoLog (program, maxLength, length, infoLog);

    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glGetProgramiv (GLuint program, GLenum pname, GLint *params)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetProgramiv))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetProgramiv (program, pname, params);

    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

void glGetRenderbufferParameteriv (GLenum target,
				   GLenum pname,
				   GLint *params)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetRenderbufferParameteriv))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (target != GL_RENDERBUFFER) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetRenderbufferParameteriv (target, pname, params);

    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glGetShaderInfoLog (GLuint program, GLsizei maxLength,
			 GLsizei *length, GLchar *infoLog)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetShaderInfoLog))
	return;

    egl_state = (egl_state_t *)active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetShaderInfoLog (program, maxLength, length, infoLog);

    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

void glGetShaderPrecisionFormat (GLenum shaderType, GLenum precisionType,
				 GLint *range, GLint *precision)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetShaderPrecisionFormat))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetShaderPrecisionFormat (shaderType, precisionType,
				       range, precision);

    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

void glGetShaderSource (GLuint shader, GLsizei bufSize, GLsizei *length,
			GLchar *source)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetShaderSource))
	return;

    egl_state = (egl_state_t *)active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetShaderSource (shader, bufSize, length, source);

    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

void glGetShaderiv (GLuint shader, GLenum pname, GLint *params)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetShaderiv))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetShaderiv (shader, pname, params);

    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

const GLubyte *glGetString (GLenum name)
{
    GLubyte *result = NULL;

    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetString))
	return result;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (! (name == GL_VENDOR || name == GL_RENDERER ||
	   name == GL_SHADING_LANGUAGE_VERSION ||
	   name == GL_EXTENSIONS)) {
	if (egl_state->state.error != GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    /* XXX: create a command buffer and wait for signal */
    result = (GLubyte *)dispatch.GetString (name);

    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
    return (const GLubyte *)result;
}

void glGetTexParameteriv (GLenum target, GLenum pname, GLint *params)
{
    egl_state_t *egl_state;
    int active_texture_index;
    int target_index;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetTexParameteriv))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (! (target == GL_TEXTURE_2D || target == GL_TEXTURE_CUBE_MAP
#ifdef GL_OES_texture_3D
	   || target == GL_TEXTURE_3D_OES
#endif
	  )) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    if (! (pname == GL_TEXTURE_MAG_FILTER ||
	   pname == GL_TEXTURE_MIN_FILTER ||
	   pname == GL_TEXTURE_WRAP_S     ||
	   pname == GL_TEXTURE_WRAP_T
#ifdef GL_OES_texture_3D
	   || pname == GL_TEXTURE_WRAP_R_OES
#endif
	)) {
	egl_state = (egl_state_t *) active_state->data;
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
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

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glGetTexParameterfv (GLenum target, GLenum pname, GLfloat *params)
{
    GLint paramsi;

    glGetTexParameteriv (target, pname, &paramsi);
    *params = paramsi;
}

void glGetUniformiv (GLuint program, GLint location, GLint *params)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetUniformiv))
	return;

    egl_state = (egl_state_t *)active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetUniformiv (program, location, params);

    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

void glGetUniformfv (GLuint program, GLint location, GLfloat *params)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetUniformfv))
	return;

    egl_state = (egl_state_t *)active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetUniformfv (program, location, params);

    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GLint glGetUniformLocation (GLuint program, const GLchar *name)
{
    GLint result = -1;
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetUniformLocation))
	return result;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create a command buffer and wait for signal */
    result = dispatch.GetUniformLocation (program, name);

    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);

    return result;
}

void glGetVertexAttribfv (GLuint index, GLenum pname, GLfloat *params)
{
    egl_state_t *egl_state;

    v_vertex_attrib_list_t *attrib_list = &cli_states.vertex_attribs;
    v_vertex_attrib_t *attribs = attrib_list->attribs;
    int count = attrib_list->count;
    int i, found_index = -1;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetVertexAttribfv))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (! (pname == GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING ||
	   pname == GL_VERTEX_ATTRIB_ARRAY_ENABLED	 ||
	   pname == GL_VERTEX_ATTRIB_ARRAY_SIZE		 ||
	   pname == GL_VERTEX_ATTRIB_ARRAY_STRIDE	 ||
	   pname == GL_VERTEX_ATTRIB_ARRAY_TYPE		 ||
	   pname == GL_VERTEX_ATTRIB_ARRAY_NORMALIZED	 ||
	   pname == GL_CURRENT_VERTEX_ATTRIB)) {
	if (egl_state->state.error != GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	gpu_mutex_unlock (egl_state->mutex);
	return;
    }

    /* gles2 spec says at least 8 */
    /* XXX: command buffer */
    if (_gl_index_is_too_large (&egl_state->state, index)) {
	gpu_mutex_unlock (egl_state->mutex);
	return;
    }

#ifdef GL_OES_vertex_array_object
    /* we cannot use client state */
    if (egl_state->state.vertex_array_binding) {
	dispatch.GetVertexAttribfv (index, pname, params);
	gpu_mutex_unlock (egl_state->mutex);
	return;
    }
#endif

    gpu_mutex_unlock (egl_state->mutex);

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
	*params = 0;
	break;
    case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
	*params = FALSE;
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

void glGetVertexAttribiv (GLuint index, GLenum pname, GLint *params)
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

void glGetVertexAttribPointerv (GLuint index, GLenum pname, GLvoid **pointer)
{
    egl_state_t *egl_state;

    v_vertex_attrib_list_t *attrib_list = &cli_states.vertex_attribs;
    v_vertex_attrib_t *attribs = attrib_list->attribs;
    int count = attrib_list->count;
    int i, found_index = -1;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetVertexAttribPointerv))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (pname != GL_VERTEX_ATTRIB_ARRAY_POINTER) {
	if (egl_state->state.error != GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	gpu_mutex_unlock (egl_state->mutex);
	return;
    }

    /* gles2 spec says at least 8 */
    /* XXX: command buffer */
    if (_gl_index_is_too_large (&egl_state->state, index)) {
	gpu_mutex_unlock (egl_state->mutex);
	return;
    }

    gpu_mutex_unlock (egl_state->mutex);

    /* look into client state */
    for (i = 0; i < count; i++) {
	if (attribs[i].index == index) {
	    *pointer = attribs[i].pointer;
	    return;
	}
    }

   *pointer = 0;
}

void glHint (GLenum target, GLenum mode)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.Hint))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if ( !(mode == GL_FASTEST ||
	   mode == GL_NICEST  ||
	   mode == GL_DONT_CARE)) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    if (target == GL_GENERATE_MIPMAP_HINT)
	egl_state->state.generate_mipmap_hint = mode;

    /* XXX: create a command buffer, no wait */
    dispatch.Hint (target, mode);

    if (target != GL_GENERATE_MIPMAP_HINT)
	egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

GLboolean glIsBuffer (GLuint buffer)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if(! _is_error_state_or_func (active_state,
				  dispatch.IsBuffer))
	return result;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create a command buffer, wait for signal */
    result = dispatch.IsBuffer (buffer);

    gpu_mutex_unlock (egl_state->mutex);
    return result;
}

GLboolean glIsEnabled (GLenum cap)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if(! _is_error_state_or_func (active_state,
				  dispatch.IsEnabled))
	return result;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (! (cap == GL_BLEND 		      ||
	   cap == GL_CULL_FACE		      ||
	   cap == GL_DEPTH_TEST		      ||
	   cap == GL_DITHER		      ||
	   cap == GL_POLYGON_OFFSET_FILL      ||
	   cap == GL_SAMPLE_ALPHA_TO_COVERAGE ||
	   cap == GL_SAMPLE_COVERAGE	      ||
	   cap == GL_SCISSOR_TEST	      ||
	   cap == GL_STENCIL_TEST)) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

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
	break;
    }

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
    return result;
}

GLboolean glIsFramebuffer (GLuint framebuffer)
{
    GLboolean result = GL_FALSE;
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.IsFramebuffer))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, wait for signal */
    result = dispatch.IsFramebuffer (framebuffer);

    gpu_mutex_unlock (egl_state->mutex);
    return result;
}

GLboolean glIsProgram (GLuint program)
{
    GLboolean result = GL_FALSE;
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.IsProgram))
	return result;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, wait for signal */
    result = dispatch.IsProgram (program);

    gpu_mutex_unlock (egl_state->mutex);
    return result;
}

GLboolean glIsRenderbuffer (GLuint renderbuffer)
{
    GLboolean result = GL_FALSE;
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.IsRenderbuffer))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, wait for signal */
    result = dispatch.IsRenderbuffer (renderbuffer);

    gpu_mutex_unlock (egl_state->mutex);
    return result;
}

GLboolean glIsShader (GLuint shader)
{
    GLboolean result = GL_FALSE;
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.IsShader))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, wait for signal */
    result = dispatch.IsShader (shader);

    gpu_mutex_unlock (egl_state->mutex);
    return result;
}

GLboolean glIsTexture (GLuint texture)
{
    GLboolean result = GL_FALSE;
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.IsTexture))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, wait for signal */
    result = dispatch.IsTexture (texture);
FINISH:
    gpu_mutex_unlock (egl_state->mutex);
    return result;
}

void glLineWidth (GLfloat width)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.LineWidth))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (width < 0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_VALUE;
	goto FINISH;
    }

    if (egl_state->state.line_width != width) {
	egl_state->state.line_width = width;
    	/* XXX: create command buffer, no wait */
    	dispatch.LineWidth (width);
    }

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glLinkProgram (GLuint program)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.LinkProgram))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, wait for signal */
    dispatch.LinkProgram (program);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glPixelStorei (GLenum pname, GLint param)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.PixelStorei))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if ((pname == GL_PACK_ALIGNMENT &&
	 egl_state->state.pack_alignment == param) ||
	(pname == GL_UNPACK_ALIGNMENT &&
	 egl_state->state.unpack_alignment == param))
	goto FINISH;
    else if (! (param == 1 ||
		param == 2 ||
		param == 4 ||
		param == 8)) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_VALUE;
	goto FINISH;
    }
    else if (! (pname == GL_PACK_ALIGNMENT ||
		pname == GL_UNPACK_ALIGNMENT)) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    /* XXX: create command buffer, no wait */
    dispatch.PixelStorei (pname, param);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glPolygonOffset (GLfloat factor, GLfloat units)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.PolygonOffset))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (egl_state->state.polygon_offset_factor == factor &&
	egl_state->state.polygon_offset_units == units)
	goto FINISH;

    egl_state->state.polygon_offset_factor = factor;
    egl_state->state.polygon_offset_units = units;

    /* XXX: create command buffer, no wait */
    dispatch.PolygonOffset (factor, units);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glReadPixels (GLint x, GLint y,
		   GLsizei width, GLsizei height,
		   GLenum format, GLenum type, GLvoid *data)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.ReadPixels))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, wait for signal */
    dispatch.ReadPixels (x, y, width, height, format, type, data);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

void glReleaseShaderCompiler (void)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.ReleaseShaderCompiler))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.ReleaseShaderCompiler ();
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

void glRenderbufferStorage (GLenum target, GLenum internalformat,
			   GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.RenderbufferStorage))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.RenderbufferStorage (target, internalformat, width, height);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

void glSampleCoverage (GLclampf value, GLboolean invert)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.SampleCoverage))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (value == egl_state->state.sample_coverage_value &&
	invert == egl_state->state.sample_coverage_invert)
	goto FINISH;

    egl_state->state.sample_coverage_invert = invert;
    egl_state->state.sample_coverage_value = value;

    /* XXX: create command buffer, no wait */
    dispatch.SampleCoverage (value, invert);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glScissor (GLint x, GLint y, GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.Scissor))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (x == egl_state->state.scissor_box[0] &&
	y == egl_state->state.scissor_box[1] &&
	width == egl_state->state.scissor_box[2] &&
	height == egl_state->state.scissor_box[3])
	goto FINISH;
    else if (width < 0 || height < 0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_VALUE;
	goto FINISH;
    }

    egl_state->state.scissor_box[0] = x;
    egl_state->state.scissor_box[1] = y;
    egl_state->state.scissor_box[2] = width;
    egl_state->state.scissor_box[3] = height;

    /* XXX: create command buffer, no wait */
    dispatch.Scissor (x, y, width, height);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glShaderBinary (GLsizei n, const GLuint *shaders,
		     GLenum binaryformat, const void *binary,
		     GLsizei length)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.ShaderBinary))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, wait for signal */
    dispatch.ShaderBinary (n, shaders, binaryformat, binary, length);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

void glShaderSource (GLuint shader, GLsizei count,
		     const GLchar **string, const GLint *length)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.ShaderSource))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, wait for signal */
    dispatch.ShaderSource (shader, count, string, length);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

void glStencilFunc (GLenum func, GLint ref, GLuint mask)
{
    glStencilFuncSeparate (GL_FRONT_AND_BACK, func, ref, mask);
}

void glStencilFuncSeparate (GLenum face, GLenum func,
			    GLint ref, GLuint mask)
{
    egl_state_t *egl_state;
    v_bool_t needs_call = FALSE;

    if(! _is_error_state_or_func (active_state,
				  dispatch.StencilFuncSeparate))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (! (face == GL_FRONT 	||
	   face == GL_BACK	||
	   face == GL_FRONT_AND_BACK)) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }
    else if (! (func == GL_NEVER	||
	   func == GL_LESS	||
	   func == GL_LEQUAL	||
	   func == GL_GREATER	||
	   func == GL_GEQUAL	||
	   func == GL_EQUAL	||
	   func == GL_NOTEQUAL	||
	   func == GL_ALWAYS)) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    switch (face) {
    case GL_FRONT:
	if (func != egl_state->state.stencil_func ||
	    ref != egl_state->state.stencil_ref ||
	    mask != egl_state->state.stencil_value_mask) {
	    egl_state->state.stencil_func = func;
	    egl_state->state.stencil_ref = ref;
	    egl_state->state.stencil_value_mask = mask;
	    needs_call = TRUE;
	}
	break;
    case GL_BACK:
	if (func != egl_state->state.stencil_back_func ||
	    ref != egl_state->state.stencil_back_ref ||
	    mask != egl_state->state.stencil_back_value_mask) {
	    egl_state->state.stencil_back_func = func;
	    egl_state->state.stencil_back_ref = ref;
	    egl_state->state.stencil_back_value_mask = mask;
	    needs_call = TRUE;
	}
	break;
    default:
	if (func != egl_state->state.stencil_back_func ||
	    func != egl_state->state.stencil_func ||
	    ref != egl_state->state.stencil_back_ref ||
	    ref != egl_state->state.stencil_ref ||
	    mask != egl_state->state.stencil_back_value_mask ||
	    mask != egl_state->state.stencil_value_mask) {
	    egl_state->state.stencil_back_func = func;
	    egl_state->state.stencil_func = func;
	    egl_state->state.stencil_back_ref = ref;
	    egl_state->state.stencil_ref;
	    egl_state->state.stencil_back_value_mask = mask;
	    egl_state->state.stencil_value_mask = mask;
	    needs_call = TRUE;
	}
	break;
    }

    /* XXX: create command buffer, no wait */
    if (needs_call)
	dispatch.StencilFuncSeparate (face, func, ref, mask);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glStencilMaskSeparate (GLenum face, GLuint mask)
{
    egl_state_t *egl_state;
    v_bool_t needs_call = FALSE;

    if(! _is_error_state_or_func (active_state,
				  dispatch.StencilMaskSeparate))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (! (face == GL_FRONT 	||
	   face == GL_BACK	||
	   face == GL_FRONT_AND_BACK)) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    switch (face) {
    case GL_FRONT:
	if (mask != egl_state->state.stencil_writemask) {
	    egl_state->state.stencil_writemask = mask;
	    needs_call = TRUE;
	}
	break;
    case GL_BACK:
	if (mask != egl_state->state.stencil_back_writemask) {
	    egl_state->state.stencil_back_writemask = mask;
	    needs_call = TRUE;
	}
	break;
    default:
	if (mask != egl_state->state.stencil_back_writemask ||
	    mask != egl_state->state.stencil_writemask) {
	    egl_state->state.stencil_back_writemask = mask;
	    egl_state->state.stencil_writemask = mask;
	    needs_call = TRUE;
	}
	break;
    }

    /* XXX: create command buffer, no wait */
    if (needs_call)
	dispatch.StencilMaskSeparate (face, mask);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glStencilMask (GLuint mask)
{
    glStencilMaskSeparate (GL_FRONT_AND_BACK, mask);
}

void glStencilOpSeparate (GLenum face, GLenum sfail, GLenum dpfail,
			  GLenum dppass)
{
    egl_state_t *egl_state;
    v_bool_t needs_call = FALSE;

    if(! _is_error_state_or_func (active_state,
				  dispatch.StencilOpSeparate))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (! (face == GL_FRONT 	||
	   face == GL_BACK	||
	   face == GL_FRONT_AND_BACK)) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }
    else if (! (sfail == GL_KEEP 	||
		sfail == GL_ZERO 	||
		sfail == GL_REPLACE	||
		sfail == GL_INCR	||
		sfail == GL_INCR_WRAP	||
		sfail == GL_DECR	||
		sfail == GL_DECR_WRAP	||
		sfail == GL_INVERT)) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }
    else if (! (dpfail == GL_KEEP 	||
		dpfail == GL_ZERO 	||
		dpfail == GL_REPLACE	||
		dpfail == GL_INCR	||
		dpfail == GL_INCR_WRAP	||
		dpfail == GL_DECR	||
		dpfail == GL_DECR_WRAP	||
		dpfail == GL_INVERT)) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }
    else if (! (dppass == GL_KEEP 	||
		dppass == GL_ZERO 	||
		dppass == GL_REPLACE	||
		dppass == GL_INCR	||
		dppass == GL_INCR_WRAP	||
		dppass == GL_DECR	||
		dppass == GL_DECR_WRAP	||
		dppass == GL_INVERT)) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    switch (face) {
    case GL_FRONT:
	if (sfail != egl_state->state.stencil_fail ||
	    dpfail != egl_state->state.stencil_pass_depth_fail ||
	    dppass != egl_state->state.stencil_pass_depth_pass) {
	    egl_state->state.stencil_fail = sfail;
	    egl_state->state.stencil_pass_depth_fail = dpfail;
	    egl_state->state.stencil_pass_depth_pass = dppass;
	    needs_call = TRUE;
	}
	break;
    case GL_BACK:
	if (sfail != egl_state->state.stencil_back_fail ||
	    dpfail != egl_state->state.stencil_back_pass_depth_fail ||
	    dppass != egl_state->state.stencil_back_pass_depth_pass) {
	    egl_state->state.stencil_back_fail = sfail;
	    egl_state->state.stencil_back_pass_depth_fail = dpfail;
	    egl_state->state.stencil_back_pass_depth_pass = dppass;
	    needs_call = TRUE;
	}
	break;
    default:
	if (sfail != egl_state->state.stencil_fail ||
	    dpfail != egl_state->state.stencil_pass_depth_fail ||
	    dppass != egl_state->state.stencil_pass_depth_pass ||
	    sfail != egl_state->state.stencil_back_fail ||
	    dpfail != egl_state->state.stencil_back_pass_depth_fail ||
	    dppass != egl_state->state.stencil_back_pass_depth_pass) {
	    egl_state->state.stencil_fail = sfail;
	    egl_state->state.stencil_pass_depth_fail = dpfail;
	    egl_state->state.stencil_pass_depth_pass = dppass;
	    egl_state->state.stencil_back_fail = sfail;
	    egl_state->state.stencil_back_pass_depth_fail = dpfail;
	    egl_state->state.stencil_back_pass_depth_pass = dppass;
	    needs_call = TRUE;
	}
	break;
    }

    /* XXX: create command buffer, no wait */
    if (needs_call)
	dispatch.StencilOpSeparate (face, sfail, dpfail, dppass);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glStencilOp (GLenum sfail, GLenum dpfail, GLenum dppass)
{
    glStencilOpSeparate (GL_FRONT_AND_BACK, sfail, dpfail, dppass);
}

void glTexImage2D (GLenum target, GLint level, GLint internalformat,
		   GLsizei width, GLsizei height, GLint border,
		   GLenum format, GLenum type, const GLvoid *data)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.TexImage2D))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, wait for signal */
    dispatch.TexImage2D (target, level, internalformat, width, height,
			 border, format, type, data);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

void glTexParameteri (GLenum target, GLenum pname, GLint param)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    int active_texture_index;
    int target_index;

    if(! _is_error_state_or_func (active_state,
				  dispatch.TexParameteri))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    state = &egl_state->state;
    active_texture_index = state->active_texture - GL_TEXTURE0;

    if (! (target == GL_TEXTURE_2D || target == GL_TEXTURE_CUBE_MAP
#ifdef GL_OES_texture_3D
	   || target == GL_TEXTURE_3D_OES
#endif
	  )) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
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
	    goto FINISH;
    }
    else if (pname == GL_TEXTURE_MIN_FILTER) {
	if (state->texture_min_filter[active_texture_index][target_index] != param) {
	    state->texture_min_filter[active_texture_index][target_index] = param;
	}
	else
	    goto FINISH;
    }
    else if (pname == GL_TEXTURE_WRAP_S) {
	if (state->texture_wrap_s[active_texture_index][target_index] != param) {
	    state->texture_wrap_s[active_texture_index][target_index] = param;
	}
	else
	    goto FINISH;
    }
    else if (pname == GL_TEXTURE_WRAP_T) {
	if (state->texture_wrap_t[active_texture_index][target_index] != param) {
	    state->texture_wrap_t[active_texture_index][target_index] = param;
	}
	else
	    goto FINISH;
    }
#ifdef GL_OES_texture_3D
    else if (pname == GL_TEXTURE_WRAP_R_OES) {
	if (state->texture_3d_wrap_r[active_texture_index] != param) {
	    state->texture_3d_wrap_r[active_texture_index] == param;
	}
	else
	    goto FINISH;
    }
#endif

    if (! (pname == GL_TEXTURE_MAG_FILTER ||
	   pname == GL_TEXTURE_MIN_FILTER ||
	   pname == GL_TEXTURE_WRAP_S     ||
	   pname == GL_TEXTURE_WRAP_T
#ifdef GL_OES_texture_3D
	   || target == GL_TEXTURE_WRAP_R_OES
#endif
	)) {
	egl_state = (egl_state_t *) active_state->data;
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    if (pname == GL_TEXTURE_MAG_FILTER &&
	! (param == GL_NEAREST ||
	   param == GL_LINEAR ||
	   param == GL_NEAREST_MIPMAP_NEAREST ||
	   param == GL_LINEAR_MIPMAP_NEAREST ||
	   param == GL_NEAREST_MIPMAP_LINEAR ||
	   param == GL_LINEAR_MIPMAP_LINEAR)) {
	egl_state = (egl_state_t *) active_state->data;
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }
    else if (pname == GL_TEXTURE_MIN_FILTER &&
	     ! (param == GL_NEAREST ||
		param == GL_LINEAR)) {
	egl_state = (egl_state_t *) active_state->data;
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }
    else if ((pname == GL_TEXTURE_WRAP_S ||
	      pname == GL_TEXTURE_WRAP_T
#ifdef GL_OES_texture_3D
	      || pname == GL_TEXTURE_WRAP_R_OES
#endif
	    ) &&
	    ! (param == GL_CLAMP_TO_EDGE ||
	       param == GL_MIRRORED_REPEAT ||
	       param == GL_REPEAT)) {
	egl_state = (egl_state_t *) active_state->data;
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    /* XXX: command buffer and no wait */
    dispatch.TexParameteri (target, pname, param);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glTexParameterf (GLenum target, GLenum pname, GLfloat param)
{
    GLint parami = param;
    glTexParameteri (target, pname, parami);
}

void glTexSubImage2D (GLenum target, GLint level,
		      GLint xoffset, GLint yoffset,
		      GLsizei width, GLsizei height,
		      GLenum format, GLenum type, const GLvoid *data)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.TexSubImage2D))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.TexSubImage2D (target, level, xoffset, yoffset,
			    width, height, format, type, data);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glUniform1f (GLint location, GLfloat v0)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.Uniform1f))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.Uniform1f (location, v0);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glUniform2f (GLint location, GLfloat v0, GLfloat v1)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.Uniform2f))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.Uniform2f (location, v0, v1);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glUniform3f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.Uniform3f))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.Uniform3f (location, v0, v1, v2);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

void glUniform4f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2,
		  GLfloat v3)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.Uniform4f))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.Uniform4f (location, v0, v1, v2, v3);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glUniform1i (GLint location, GLint v0)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.Uniform1i))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, not wait */
    dispatch.Uniform1i (location, v0);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

void glUniform2i (GLint location, GLint v0, GLint v1)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.Uniform2i))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, not wait */
    dispatch.Uniform2i (location, v0, v1);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glUniform3i (GLint location, GLint v0, GLint v1, GLint v2)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.Uniform3i))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.Uniform3i (location, v0, v1, v2);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glUniform4i (GLint location, GLint v0, GLint v1, GLint v2,
		  GLint v3)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.Uniform4i))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, not wait */
    dispatch.Uniform4i (location, v0, v1, v2, v3);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

static void
_gl_uniform_fv (int i, GLint location,
		GLsizei count, const GLfloat *value)
{
    egl_state_t *egl_state;

    if (i == 1) {
	if(! _is_error_state_or_func (active_state,
				      dispatch.Uniform1fv))
	    return;
    }
    else if (i == 2) {
	if(! _is_error_state_or_func (active_state,
				      dispatch.Uniform2fv))
	    return;
    }
    else if (i == 3) {
	if(! _is_error_state_or_func (active_state,
				      dispatch.Uniform3fv))
	    return;
    }
    else {
	if(! _is_error_state_or_func (active_state,
				      dispatch.Uniform4fv))
	    return;
    }

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, not wait */
    if (i == 1)
	dispatch.Uniform1fv (location, count, value);
    else if (i == 2)
	dispatch.Uniform2fv (location, count, value);
    else if (i == 3)
	dispatch.Uniform3fv (location, count, value);
    else
	dispatch.Uniform4fv (location, count, value);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glUniform1fv (GLint location, GLsizei count, const GLfloat *value)
{
    _gl_uniform_fv (1, location, count, value);
}

void glUniform2fv (GLint location, GLsizei count, const GLfloat *value)
{
    _gl_uniform_fv (2, location, count, value);
}

void glUniform3fv (GLint location, GLsizei count, const GLfloat *value)
{
    _gl_uniform_fv (3, location, count, value);
}

void glUniform4fv (GLint location, GLsizei count, const GLfloat *value)
{
    _gl_uniform_fv (4, location, count, value);
}

static void
_gl_uniform_iv (int i, GLint location,
		GLsizei count, const GLint *value)
{
    egl_state_t *egl_state;

    if (i == 1) {
	if(! _is_error_state_or_func (active_state,
				      dispatch.Uniform1iv))
	goto FINISH;
    }
    else if (i == 2) {
	if(! _is_error_state_or_func (active_state,
				      dispatch.Uniform2iv))
	goto FINISH;
    }
    else if (i == 3) {
	if(! _is_error_state_or_func (active_state,
				      dispatch.Uniform3iv))
	goto FINISH;
    }
    else {
	if(! _is_error_state_or_func (active_state,
				      dispatch.Uniform4iv))
	goto FINISH;
    }

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    if (i == 1)
	dispatch.Uniform1iv (location, count, value);
    else if (i == 2)
	dispatch.Uniform2iv (location, count, value);
    else if (i == 3)
	dispatch.Uniform3iv (location, count, value);
    else
	dispatch.Uniform4iv (location, count, value);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glUniform1iv (GLint location, GLsizei count, const GLint *value)
{
    _gl_uniform_iv (1, location, count, value);
}

void glUniform2iv (GLint location, GLsizei count, const GLint *value)
{
    _gl_uniform_iv (2, location, count, value);
}

void glUniform3iv (GLint location, GLsizei count, const GLint *value)
{
    _gl_uniform_iv (3, location, count, value);
}

void glUniform4iv (GLint location, GLsizei count, const GLint *value)
{
    _gl_uniform_iv (4, location, count, value);
}

void glUseProgram (GLuint program)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.UseProgram))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (egl_state->state.current_program == program)
	goto FINISH;

    /* XXX: create command buffer, no wait signal */
    dispatch.UseProgram (program);
    /* FIXME: this maybe not right because this texture may be invalid
     * object, we save here to save time in glGetError() */
    egl_state->state.current_program = program;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glValidateProgram (GLuint program)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.ValidateProgram))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (egl_state->state.current_program == program)
	goto FINISH;

    /* XXX: create command buffer, no wait signal */
    dispatch.ValidateProgram (program);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glVertexAttrib1f (GLuint index, GLfloat v0)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.VertexAttrib1f))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (_gl_index_is_too_large (&egl_state->state, index)) {
	gpu_mutex_unlock (egl_state->mutex);
	return;
    }

    /* XXX: create command buffer, no wait signal */
    dispatch.VertexAttrib1f (index, v0);

    gpu_mutex_unlock (egl_state->mutex);
}

void glVertexAttrib2f (GLuint index, GLfloat v0, GLfloat v1)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.VertexAttrib2f))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (_gl_index_is_too_large (&egl_state->state, index)) {
	gpu_mutex_unlock (egl_state->mutex);
	return;
    }

    /* XXX: create command buffer, no wait signal */
    dispatch.VertexAttrib2f (index, v0, v1);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glVertexAttrib3f (GLuint index, GLfloat v0, GLfloat v1, GLfloat v2)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.VertexAttrib3f))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (_gl_index_is_too_large (&egl_state->state, index)) {
	gpu_mutex_unlock (egl_state->mutex);
	return;
    }

    /* XXX: create command buffer, no wait signal */
    dispatch.VertexAttrib3f (index, v0, v1, v2);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glVertexAttrib4f (GLuint index, GLfloat v0, GLfloat v1, GLfloat v2,
		       GLfloat v3)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.VertexAttrib4f))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (_gl_index_is_too_large (&egl_state->state, index))
	goto FINISH;

    /* XXX: create command buffer, no wait signal */
    dispatch.VertexAttrib4f (index, v0, v1, v2, v3);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

static void
_gl_vertex_attrib_fv (int i, GLuint index, const GLfloat *v)
{
    egl_state_t *egl_state;

    if (i == 1) {
	if(! _is_error_state_or_func (active_state,
				      dispatch.VertexAttrib1fv))
	    return;
    }
    else if (i == 2) {
	if(! _is_error_state_or_func (active_state,
				      dispatch.VertexAttrib2fv))
	    return;
    }
    else if (i == 3) {
	if(! _is_error_state_or_func (active_state,
				      dispatch.VertexAttrib3fv))
	    return;
    }
    else {
	if(! _is_error_state_or_func (active_state,
				      dispatch.VertexAttrib4fv))
	    return;
    }

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (_gl_index_is_too_large (&egl_state->state, index))
	goto FINISH;

    /* XXX: create command buffer, no wait signal */
    if (i == 1)
	dispatch.VertexAttrib1fv (index, v);
    else if (i == 2)
	dispatch.VertexAttrib2fv (index, v);
    else if (i == 3)
	dispatch.VertexAttrib3fv (index, v);
    else
	dispatch.VertexAttrib4fv (index, v);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

void glVertexAttrib1fv (GLuint index, const GLfloat *v)
{
    _gl_vertex_attrib_fv (1, index, v);
}

void glVertexAttrib2fv (GLuint index, const GLfloat *v)
{
    _gl_vertex_attrib_fv (2, index, v);
}

void glVertexAttrib3fv (GLuint index, const GLfloat *v)
{
    _gl_vertex_attrib_fv (3, index, v);
}

void glVertexAttrib4fv (GLuint index, const GLfloat *v)
{
    _gl_vertex_attrib_fv (4, index, v);
}

void glVertexAttribPointer (GLuint index, GLint size, GLenum type,
			    GLboolean normalized, GLsizei stride,
			    const GLvoid *pointer)
{
    egl_state_t *egl_state;
    v_vertex_attrib_list_t *attrib_list = &cli_states.vertex_attribs;
    v_vertex_attrib_t *attribs = attrib_list->attribs;
    int count = attrib_list->count;
    int i, found_index = -1;
    GLint bound_buffer = 0;

    if (! active_state && ! active_state->data)
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

#ifdef GL_OES_vertex_array_object

    if (egl_state->state.vertex_array_binding) {
	dispatch.VertexAttribPointer (index, size, type, normalized,
				      stride, pointer);
	egl_state->state.need_get_error = TRUE;
	gpu_mutex_unlock (egl_state->mutex);
	return;
    }
#endif
    /* check existing client state */
    for (i = 0; i < count; i++) {
	if (attribs[i].index == index) {
	    if (attribs[i].size == size &&
		attribs[i].type == type &&
		attribs[i].stride == stride) {
		attribs[i].array_normalized = normalized;
		attribs[i].pointer = (GLvoid *)pointer;
		return;
	    }
	    else {
		found_index = i;
		break;
	    }
	}
    }

    if (! (type == GL_BYTE 		||
	   type == GL_UNSIGNED_BYTE	||
	   type == GL_SHORT		||
	   type == GL_UNSIGNED_SHORT	||
	   type == GL_FLOAT)) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	gpu_mutex_unlock (egl_state->mutex);
	return;
    }
    else if (size > 4 || size < 1 || stride < 0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_VALUE;
	gpu_mutex_unlock (egl_state->mutex);
	return;
    }

    /* check max_vertex_attribs */
    if (_gl_index_is_too_large (&egl_state->state, index)) {
	gpu_mutex_unlock (egl_state->mutex);
	return;
    }

    bound_buffer = egl_state->state.array_buffer_binding;

    gpu_mutex_unlock (egl_state->mutex);

    if (found_index != -1) {
	attribs[found_index].size = size;
	attribs[found_index].stride = stride;
	attribs[found_index].type = type;
	attribs[found_index].array_normalized = normalized;
	attribs[found_index].pointer = (GLvoid *)pointer;
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
	v_vertex_attrib_t *new_attribs = (v_vertex_attrib_t *)malloc (sizeof (v_vertex_attrib_t) * (count + 1));

	memcpy (new_attribs, attribs, (count+1) * sizeof (v_vertex_attrib_t));
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

void glViewport (GLint x, GLint y, GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.Viewport))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (egl_state->state.viewport[0] == x &&
	egl_state->state.viewport[1] == y &&
	egl_state->state.viewport[2] == width &&
	egl_state->state.viewport[3] == height)
	goto FINISH;

    if (x < 0 || y < 0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_VALUE;
	goto FINISH;
    }

    egl_state->state.viewport[0] = x;
    egl_state->state.viewport[1] = y;
    egl_state->state.viewport[2] = width;
    egl_state->state.viewport[3] = height;

    /* XXX: create command buffer, no wait signal */
    dispatch.Viewport (x, y, width, height);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}
/* end of GLES2 core profile */

#ifdef GL_OES_EGL_image
GL_APICALL void GL_APIENTRY
glEGLImageTargetTexture2DOES (GLenum target, GLeglImageOES image)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.EGLImageTargetTexture2DOES))
	return;

    gpu_mutex_lock (egl_state->mutex);

    egl_state = (egl_state_t *) active_state->data;

    if (target != GL_TEXTURE_2D) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    /* XXX: create command buffer, no wait signal */
    dispatch.EGLImageTargetTexture2DOES (target, image);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glEGLImageTargetRenderbufferStorageOES (GLenum target, GLeglImageOES image)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.EGLImageTargetRenderbufferStorageOES))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (target != GL_RENDERBUFFER_OES) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    /* XXX: create command buffer, no wait signal */
    dispatch.EGLImageTargetRenderbufferStorageOES (target, image);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}
#endif

#ifdef GL_OES_get_program_binary
GL_APICALL void GL_APIENTRY
glGetProgramBinaryOES (GLuint program, GLsizei bufSize, GLsizei *length,
		       GLenum *binaryFormat, GLvoid *binary)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetProgramBinaryOES))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait signal */
    dispatch.GetProgramBinaryOES (program, bufSize, length, binaryFormat,
				  binary);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glProgramBinaryOES (GLuint program, GLenum binaryFormat,
		    const GLvoid *binary, GLint length)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.ProgramBinaryOES))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait signal */
    dispatch.ProgramBinaryOES (program, binaryFormat, binary, length);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}
#endif

#ifdef GL_OES_mapbuffer
GL_APICALL void* GL_APIENTRY glMapBufferOES (GLenum target, GLenum access)
{
    egl_state_t *egl_state;
    void *result = NULL;

    if(! _is_error_state_or_func (active_state,
				  dispatch.MapBufferOES))
	return result;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (access != GL_WRITE_ONLY_OES) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }
    else if (! (target == GL_ARRAY_BUFFER ||
		target == GL_ELEMENT_ARRAY_BUFFER)) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    /* XXX: create command buffer, wait signal */
    result = dispatch.MapBufferOES (target, access);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
    return result;
}

GL_APICALL GLboolean GL_APIENTRY
glUnmapBufferOES (GLenum target)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if(! _is_error_state_or_func (active_state,
				  dispatch.UnmapBufferOES))
	return result;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (! (target == GL_ARRAY_BUFFER ||
	   target == GL_ELEMENT_ARRAY_BUFFER)) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    /* XXX: create command buffer, wait signal */
    result = dispatch.UnmapBufferOES (target);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
    return result;
}

GL_APICALL void GL_APIENTRY
glGetBufferPointervOES (GLenum target, GLenum pname, GLvoid **params)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetBufferPointervOES)) {
	*params = NULL;
	return;
    }

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (! (target == GL_ARRAY_BUFFER ||
	   target == GL_ELEMENT_ARRAY_BUFFER)) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	    *params = NULL;
	goto FINISH;
    }

    /* XXX: create command buffer, wait signal */
    dispatch.GetBufferPointervOES (target, pname, params);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}
#endif

#ifdef GL_OES_texture_3D
GL_APICALL void GL_APIENTRY
glTexImage3DOES (GLenum target, GLint level, GLenum internalformat,
		 GLsizei width, GLsizei height, GLsizei depth,
		 GLint border, GLenum format, GLenum type,
		 const GLvoid *pixels)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.TexImage3DOES))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, wait for signal */
    dispatch.TexImage3DOES (target, level, internalformat, width, height,
			    depth, border, format, type, pixels);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glTexSubImage3DOES (GLenum target, GLint level,
		    GLint xoffset, GLint yoffset, GLint zoffset,
		    GLsizei width, GLsizei height, GLsizei depth,
		    GLenum format, GLenum type, const GLvoid *data)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.TexSubImage3DOES))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.TexSubImage3DOES (target, level, xoffset, yoffset, zoffset,
			       width, height, depth, format, type, data);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glCopyTexSubImage3DOES (GLenum target, GLint level,
			GLint xoffset, GLint yoffset, GLint zoffset,
			GLint x, GLint y,
			GLint width, GLint height)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.CopyTexSubImage3DOES))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: command buffer */
    dispatch.CopyTexSubImage3DOES (target, level,
				   xoffset, yoffset, zoffset,
				   x, y, width, height);

    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glCompressedTexImage3DOES (GLenum target, GLint level,
			   GLenum internalformat,
			   GLsizei width, GLsizei height, GLsizei depth,
			   GLint border, GLsizei imageSize,
			   const GLvoid *data)
{
    egl_state_t *egl_state;


    if(! _is_error_state_or_func (active_state,
				  dispatch.CompressedTexImage3DOES))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: command buffer */
    dispatch.CompressedTexImage3DOES (target, level, internalformat,
				      width, height, depth,
				      border, imageSize, data);

    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glCompressedTexSubImage3DOES (GLenum target, GLint level,
			      GLint xoffset, GLint yoffset, GLint zoffset,
			      GLsizei width, GLsizei height, GLsizei depth,
			      GLenum format, GLsizei imageSize,
			      const GLvoid *data)
{
    egl_state_t *egl_state;


    if(! _is_error_state_or_func (active_state,
				  dispatch.CompressedTexSubImage3DOES))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: command buffer */
    dispatch.CompressedTexSubImage3DOES (target, level,
					 xoffset, yoffset, zoffset,
					 width, height, depth,
					 format, imageSize, data);

    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glFramebufferTexture3DOES (GLenum target, GLenum attachment,
			   GLenum textarget, GLuint texture,
			   GLint level, GLint zoffset)
{
    egl_state_t *egl_state;


    if(! _is_error_state_or_func (active_state,
				  dispatch.FramebufferTexture3DOES))
	return;

    egl_state = (egl_state_t *)active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    if (target != GL_FRAMEBUFFER) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    dispatch.FramebufferTexture3DOES (target, attachment, textarget,
				      texture, level, zoffset);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
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
GL_APICALL void GL_APIENTRY
glBindVertexArrayOES (GLuint array)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.BindVertexArrayOES))
	return;

    egl_state = (egl_state_t *)active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    dispatch.BindVertexArrayOES (array);
    egl_state->state.need_get_error = TRUE;
    /* should we save this ? */
    egl_state->state.vertex_array_binding = array;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glDeleteVertexArraysOES (GLsizei n, const GLuint *arrays)
{
    egl_state_t *egl_state;
    int i;

    /* XXX: any error generated ? */
    if (n <= 0)
	return;

    if(! _is_error_state_or_func (active_state,
				  dispatch.DeleteVertexArraysOES))
	return;

    egl_state = (egl_state_t *)active_state->data;
        /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    dispatch.DeleteVertexArraysOES (n, arrays);

    /* matching vertex_array_binding ? */
    for (i = 0; i < n; i++) {
	if (arrays[i] == egl_state->state.vertex_array_binding) {
	    egl_state->state.vertex_array_binding = 0;
	    break;
	}
    }

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glGenVertexArraysOES (GLsizei n, GLuint *arrays)
{
    egl_state_t *egl_state;

    /* XXX: any error generated ? */
    if (n <= 0)
	return;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GenVertexArraysOES))
	return;

    egl_state = (egl_state_t *)active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);


    dispatch.GenVertexArraysOES (n, arrays);

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL GLboolean GL_APIENTRY
glIsVertexArrayOES (GLuint array)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if(! _is_error_state_or_func (active_state,
				  dispatch.IsVertexArrayOES))
	return result;

    egl_state = (egl_state_t *) active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    result = dispatch.IsVertexArrayOES (array);

    if (result == GL_FALSE && egl_state->state.vertex_array_binding == array)
	egl_state->state.vertex_array_binding = 0;

    gpu_mutex_unlock (egl_state->mutex);
    return result;
}
#endif

#ifdef GL_AMD_performance_monitor
GL_APICALL void GL_APIENTRY
glGetPerfMonitorGroupsAMD (GLint *numGroups, GLsizei groupSize, 
			   GLuint *groups)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetPerfMonitorGroupsAMD))
	return;

    egl_state = (egl_state_t *) active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    dispatch.GetPerfMonitorGroupsAMD (numGroups, groupSize, groups);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glGetPerfMonitorCountersAMD (GLuint group, GLint *numCounters, 
			     GLint *maxActiveCounters, GLsizei counterSize,
			     GLuint *counters)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetPerfMonitorCountersAMD))
	return;

    egl_state = (egl_state_t *)active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    dispatch.GetPerfMonitorCountersAMD (group, numCounters,
					maxActiveCounters,
					counterSize, counters);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glGetPerfMonitorGroupStringAMD (GLuint group, GLsizei bufSize, 
				GLsizei *length, GLchar *groupString)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetPerfMonitorGroupStringAMD))
	return;

    egl_state = (egl_state_t *)active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    dispatch.GetPerfMonitorGroupStringAMD (group, bufSize, length,
					   groupString);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glGetPerfMonitorCounterStringAMD (GLuint group, GLuint counter, 
				  GLsizei bufSize, 
				  GLsizei *length, GLchar *counterString)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetPerfMonitorCounterStringAMD))
	return;

    egl_state = (egl_state_t *)active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    dispatch.GetPerfMonitorCounterStringAMD (group, counter, bufSize, 
					     length, counterString);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glGetPerfMonitorCounterInfoAMD (GLuint group, GLuint counter, 
				GLenum pname, GLvoid *data)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetPerfMonitorCounterInfoAMD))
	return;

    egl_state = (egl_state_t *)active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    dispatch.GetPerfMonitorCounterInfoAMD (group, counter, pname, data); 
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glGenPerfMonitorsAMD (GLsizei n, GLuint *monitors) 
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GenPerfMonitorsAMD))
	return;

    egl_state = (egl_state_t *)active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    dispatch.GenPerfMonitorsAMD (n, monitors); 

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glDeletePerfMonitorsAMD (GLsizei n, GLuint *monitors) 
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state, 
				  dispatch.DeletePerfMonitorsAMD))
	return;

    egl_state = (egl_state_t *)active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    dispatch.DeletePerfMonitorsAMD (n, monitors); 
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glSelectPerfMonitorCountersAMD (GLuint monitor, GLboolean enable,
				GLuint group, GLint numCounters,
				GLuint *countersList) 
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state, 
				  dispatch.SelectPerfMonitorCountersAMD))
	return;

    egl_state = (egl_state_t *)active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    dispatch.SelectPerfMonitorCountersAMD (monitor, enable, group,
					   numCounters, countersList); 
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glBeginPerfMonitorAMD (GLuint monitor)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state, 
				  dispatch.BeginPerfMonitorAMD))
	return;

    egl_state = (egl_state_t *)active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    dispatch.BeginPerfMonitorAMD (monitor); 
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glEndPerfMonitorAMD (GLuint monitor)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state, 
				  dispatch.EndPerfMonitorAMD))
	return;

    egl_state = (egl_state_t *)active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    dispatch.EndPerfMonitorAMD (monitor); 
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glGetPerfMonitorCounterDataAMD (GLuint monitor, GLenum pname,
				GLsizei dataSize, GLuint *data,
				GLint *bytesWritten)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetPerfMonitorCounterDataAMD))
	return;

    egl_state = (egl_state_t *)active_state->data;
        /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    dispatch.GetPerfMonitorCounterDataAMD (monitor, pname, dataSize,
					   data, bytesWritten); 
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}
#endif

#ifdef GL_ANGLE_framebuffer_blit
GL_APICALL void GL_APIENTRY
glBlitFramebufferANGLE (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
			GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
			GLbitfield mask, GLenum filter)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state, 
				  dispatch.BlitFramebufferANGLE))
	return;

    egl_state = (egl_state_t *)active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    dispatch.BlitFramebufferANGLE (srcX0, srcY0, srcX1, srcY1,
				   dstX0, dstY0, dstX1, dstY1,
				   mask, filter);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}
#endif

#ifdef GL_ANGLE_framebuffer_multisample
GL_APICALL void GL_APIENTRY
glRenderbufferStorageMultisampleANGLE (GLenum target, GLsizei samples, 
				       GLenum internalformat, 
				       GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state, 
				  dispatch.RenderbufferStorageMultisampleANGLE))
	return;

    egl_state = (egl_state_t *)active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    dispatch.RenderbufferStorageMultisampleANGLE (target, samples,
						  internalformat,
						  width, height);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}
#endif

#ifdef GL_APPLE_framebuffer_multisample
GL_APICALL void GL_APIENTRY
glRenderbufferStorageMultisampleAPPLE (GLenum target, GLsizei samples, 
				       GLenum internalformat, 
				       GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state, 
				  dispatch.RenderbufferStorageMultisampleAPPLE))
	return;

    egl_state = (egl_state_t *)active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    dispatch.RenderbufferStorageMultisampleAPPLE (target, samples,
						  internalformat,
						  width, height);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glResolveMultisampleFramebufferAPPLE (void) 
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state, 
				  dispatch.ResolveMultisampleFramebufferAPPLE))
	return;

    egl_state = (egl_state_t *)active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    dispatch.ResolveMultisampleFramebufferAPPLE ();
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}
#endif

#ifdef GL_EXT_discard_framebuffer
GL_APICALL void GL_APIENTRY
glDiscardFramebufferEXT (GLenum target, GLsizei numAttachments,
			 const GLenum *attachments)
{
    egl_state_t *egl_state;
    int i;

    if(! _is_error_state_or_func (active_state, 
				  dispatch.DiscardFramebufferEXT))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    if (target != GL_FRAMEBUFFER) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    for (i = 0; i < numAttachments; i++) {
	if (! (attachments[i] == GL_COLOR_ATTACHMENT0  ||
	       attachments[i] == GL_DEPTH_ATTACHMENT   ||
	       attachments[i] == GL_STENCIL_ATTACHMENT ||
	       attachments[i] == GL_COLOR_EXT	       ||
	       attachments[i] == GL_DEPTH_EXT	       ||
	       attachments[i] == GL_STENCIL_EXT)) {
	    if (egl_state->state.error == GL_NO_ERROR)
		egl_state->state.error = GL_INVALID_ENUM;
	    goto FINISH;
	}
    }

    dispatch.DiscardFramebufferEXT (target, numAttachments, attachments);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
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
GL_APICALL void GL_APIENTRY
glRenderbufferStorageMultisampleEXT (GLenum target, GLsizei samples,
				     GLenum internalformat,
				     GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state, 
				  dispatch.RenderbufferStorageMultisampleEXT))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.RenderbufferStorageMultisampleEXT (target, samples, 
						internalformat, 
						width, height);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glFramebufferTexture2DMultisampleEXT (GLenum target, GLenum attachment,
				      GLenum textarget, GLuint texture,
				      GLint level, GLsizei samples)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state, 
				  dispatch.FramebufferTexture2DMultisampleEXT))
	return;

    egl_state = (egl_state_t *)active_state->data;
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (egl_state->mutex);

    if (target != GL_FRAMEBUFFER) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    dispatch.FramebufferTexture2DMultisampleEXT (target, attachment, 
						 textarget, texture, 
						 level, samples);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}
#endif

#ifdef GL_IMG_multisampled_render_to_texture
GL_APICALL void GL_APIENTRY
glRenderbufferStorageMultisampleIMG (GLenum target, GLsizei samples,
				     GLenum internalformat,
				     GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state, 
				  dispatch.RenderbufferStorageMultisampleIMG))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.RenderbufferStorageMultisampleIMG (target, samples, 
						internalformat, 
						width, height);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glFramebufferTexture2DMultisampleIMG (GLenum target, GLenum attachment,
				      GLenum textarget, GLuint texture,
				      GLint level, GLsizei samples)
{
    egl_state_t *egl_state;

    /* XXX: put to a command buffer, no wait for signal */
    if(! _is_error_state_or_func (active_state, 
				  dispatch.FramebufferTexture2DMultisampleIMG))
	return;

    egl_state = (egl_state_t *)active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (target != GL_FRAMEBUFFER) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    dispatch.FramebufferTexture2DMultisampleIMG (target, attachment,
						 textarget, texture,
						 level, samples);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}
#endif

#ifdef GL_NV_fence
GL_APICALL void GL_APIENTRY
glDeleteFencesNV (GLsizei n, const GLuint *fences)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.DeleteFencesNV))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.DeleteFencesNV (n, fences); 
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glGenFencesNV (GLsizei n, GLuint *fences)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GenFencesNV))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.GenFencesNV (n, fences); 
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL GLboolean GL_APIENTRY
glIsFenceNV (GLuint fence)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if(! _is_error_state_or_func (active_state,
				  dispatch.IsFenceNV))
	return result;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    result = dispatch.IsFenceNV (fence);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
    return result;
}

GL_APICALL GLboolean GL_APIENTRY
glTestFenceNV (GLuint fence)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if(! _is_error_state_or_func (active_state,
				  dispatch.TestFenceNV))
	return result;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    result = dispatch.TestFenceNV (fence);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
    return result;
}

GL_APICALL GLboolean GL_APIENTRY
glGetFenceivNV (GLuint fence, GLenum pname, int *params)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetFenceivNV))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.GetFenceivNV (fence, pname, params);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL GLboolean GL_APIENTRY
glFinishFenceivNV (GLuint fence)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.FinishFenceNV))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.FinishFenceNV (fence);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL GLboolean GL_APIENTRY
glSetFenceNV (GLuint fence, GLenum condition)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.SetFenceNV))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.SetFenceNV (fence, condition);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}
#endif

#ifdef GL_NV_coverage_sample
GL_APICALL void GL_APIENTRY
glCoverageMaskNV (GLboolean mask)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.CoverageMaskNV))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.CoverageMaskNV (mask);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glCoverageOperationNV (GLenum operation)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.CoverageOperationNV))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    if (! (operation == GL_COVERAGE_ALL_FRAGMENTS_NV  ||
	   operation == GL_COVERAGE_EDGE_FRAGMENTS_NV ||
	   operation == GL_COVERAGE_AUTOMATIC_NV)) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    /* XXX: create command buffer, no wait */
    dispatch.CoverageOperationNV (operation);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}
#endif

#ifdef GL_QCOM_driver_control
GL_APICALL void GL_APIENTRY
glGetDriverControlsQCOM (GLint *num, GLsizei size, GLuint *driverControls)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetDriverControlsQCOM))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.GetDriverControlsQCOM (num, size, driverControls);

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glGetDriverControlStringQCOM (GLuint driverControl, GLsizei bufSize,
			      GLsizei *length, GLchar *driverControlString)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.GetDriverControlStringQCOM))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.GetDriverControlStringQCOM (driverControl, bufSize,
					 length, driverControlString);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glEnableDriverControlQCOM (GLuint driverControl)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.EnableDriverControlQCOM))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.EnableDriverControlQCOM (driverControl);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glDisableDriverControlQCOM (GLuint driverControl)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.DisableDriverControlQCOM))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.DisableDriverControlQCOM (driverControl);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}
#endif

#ifdef GL_QCOM_extended_get
GL_APICALL void GL_APIENTRY
glExtGetTexturesQCOM (GLuint *textures, GLint maxTextures, 
		      GLint *numTextures)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.ExtGetTexturesQCOM))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.ExtGetTexturesQCOM (textures, maxTextures, numTextures);

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glExtGetBuffersQCOM (GLuint *buffers, GLint maxBuffers, GLint *numBuffers) 
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.ExtGetBuffersQCOM))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.ExtGetBuffersQCOM (buffers, maxBuffers, numBuffers);

FINISH:
    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glExtGetRenderbuffersQCOM (GLuint *renderbuffers, GLint maxRenderbuffers,
			   GLint *numRenderbuffers)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.ExtGetRenderbuffersQCOM))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.ExtGetRenderbuffersQCOM (renderbuffers, maxRenderbuffers,
				      numRenderbuffers);

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glExtGetFramebuffersQCOM (GLuint *framebuffers, GLint maxFramebuffers,
			  GLint *numFramebuffers)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.ExtGetFramebuffersQCOM))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.ExtGetFramebuffersQCOM (framebuffers, maxFramebuffers,
				     numFramebuffers);

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glExtGetTexLevelParameterivQCOM (GLuint texture, GLenum face, GLint level,
				 GLenum pname, GLint *params)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.ExtGetTexLevelParameterivQCOM))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.ExtGetTexLevelParameterivQCOM (texture, face, level,
					    pname, params);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glExtTexObjectStateOverrideiQCOM (GLenum target, GLenum pname, GLint param)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.ExtTexObjectStateOverrideiQCOM))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.ExtTexObjectStateOverrideiQCOM (target, pname, param);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glExtGetTexSubImageQCOM (GLenum target, GLint level,
			 GLint xoffset, GLint yoffset, GLint zoffset,
			 GLsizei width, GLsizei height, GLsizei depth,
			 GLenum format, GLenum type, GLvoid *texels)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.ExtGetTexSubImageQCOM))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.ExtGetTexSubImageQCOM (target, level,
				    xoffset, yoffset, zoffset,
				    width, height, depth,
				    format, type, texels);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glExtGetBufferPointervQCOM (GLenum target, GLvoid **params)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.ExtGetBufferPointervQCOM))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.ExtGetBufferPointervQCOM (target, params);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}
#endif

#ifdef GL_QCOM_extended_get2
GL_APICALL void GL_APIENTRY
glExtGetShadersQCOM (GLuint *shaders, GLint maxShaders, GLint *numShaders)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.ExtGetShadersQCOM))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.ExtGetShadersQCOM (shaders, maxShaders, numShaders);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glExtGetProgramsQCOM (GLuint *programs, GLint maxPrograms,
		      GLint *numPrograms)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.ExtGetProgramsQCOM))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.ExtGetProgramsQCOM (programs, maxPrograms, numPrograms);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL GLboolean GL_APIENTRY
glExtIsProgramBinaryQCOM (GLuint program)
{
    egl_state_t *egl_state;
    v_bool_t result = FALSE;

    if(! _is_error_state_or_func (active_state,
				  dispatch.ExtIsProgramBinaryQCOM))
	return result;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    result = dispatch.ExtIsProgramBinaryQCOM (program);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
    return result;
}

GL_APICALL void GL_APIENTRY
glExtGetProgramBinarySourceQCOM (GLuint program, GLenum shadertype,
				 GLchar *source, GLint *length)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.ExtGetProgramBinarySourceQCOM))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.ExtGetProgramBinarySourceQCOM (program, shadertype,
					    source, length);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}
#endif

#ifdef GL_QCOM_tiled_rendering
GL_APICALL void GL_APIENTRY
glStartTilingQCOM (GLuint x, GLuint y, GLuint width, GLuint height,
		   GLbitfield preserveMask)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.StartTilingQCOM))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.StartTilingQCOM (x, y, width, height, preserveMask);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}

GL_APICALL void GL_APIENTRY
glEndTilingQCOM (GLbitfield preserveMask)
{
    egl_state_t *egl_state;

    if(! _is_error_state_or_func (active_state,
				  dispatch.EndTilingQCOM))
	return;

    egl_state = (egl_state_t *) active_state->data;
    gpu_mutex_lock (egl_state->mutex);

    /* XXX: create command buffer, no wait */
    dispatch.EndTilingQCOM (preserveMask);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (egl_state->mutex);
}
#endif
