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
extern gpu_mutex_t	mutex;

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

    gpu_mutex_lock (mutex);

    if (_is_error_state_or_func (active_state, dispatch.ActiveTexture))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

    if (texture > GL_TEXTURE31 || texture < GL_TEXTURE0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }
   
    if (texture == egl_state->state.active_texture)
	goto FINISH;

    /* XXX: create a command buffer */
    dispatch.ActiveTexture (texture);
    egl_state->state.active_texture = texture;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glAttachShader (GLuint program, GLuint shader)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if (_is_error_state_or_func (active_state, dispatch.AttachShader))
	goto FINISH;

    /* XXX: create a command buffer */
    dispatch.AttachShader (program, shader);
    egl_state = (egl_state_t *) active_state->data;
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glBindAttribLication (GLuint program, GLuint index, const GLchar *name)
{
    gles2_state_t *state;
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if (_is_error_state_or_func (active_state,
				 dispatch.BindAttribLocation))
	goto FINISH;

    /* XXX: create a command buffer */
    dispatch.BindAttribLocation (program, index, name);
    egl_state = (egl_state_t *) active_state->data;
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glBindBuffer (GLenum target, GLuint buffer)
{
    egl_state_t *egl_state;
    gpu_mutex_lock (mutex);

    if (!(target == GL_ARRAY_BUFFER || target == GL_ELEMENT_ARRAY_BUFFER) ||
	! _is_error_state_or_func (active_state, dispatch.BindBuffer))
	goto FINISH;

    /* XXX: create a command buffer */
    dispatch.BindBuffer (target, buffer);
    egl_state = (egl_state_t *) active_state->data;
    egl_state->state.need_get_error = TRUE;

    /* FIXME: we don't know whether it succeeds or not */
    if (target == GL_ARRAY_BUFFER)
	egl_state->state.array_buffer_binding = buffer;
    else
	egl_state->state.element_array_buffer_binding = buffer;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glBindFramebuffer (GLenum target, GLuint framebuffer)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.BindFramebuffer))
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
	egl_state = (egl_state_t *) active_state->data;
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }
    /* XXX: create a command buffer, according to spec, no error generated */
    dispatch.BindFramebuffer (target, framebuffer);

    //egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glBindRenderbuffer (GLenum target, GLuint renderbuffer)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.BindRenderbuffer))
	goto FINISH;

    if (target != GL_RENDERBUFFER) {
	egl_state = (egl_state_t *) active_state->data;
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }
    /* XXX: create a command buffer, according to spec, no error generated */
    dispatch.BindRenderbuffer (target, renderbuffer);
    //egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glBindTexture (GLenum target, GLuint texture)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.BindTexture))
	goto FINISH;

    if (! (target == GL_TEXTURE_2D || target == GL_TEXTURE_CUBE_MAP
#ifdef GL_OES_texture_3D
	   || target == GL_TEXTURE_3D_OES
#endif
	  )) {
	egl_state = (egl_state_t *) active_state->data;
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

FINISH:
    gpu_mutex_unlock (mutex);
}

void glBlendColor (GLclampf red, GLclampf green, 
		   GLclampf blue, GLclampf alpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.BlendColor))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
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
    gpu_mutex_unlock (mutex);
}

void glBlendEquation (GLenum mode)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.BlendEquation))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
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
    gpu_mutex_unlock (mutex);
}
	   
void glBlendEquationSeparate (GLenum modeRGB, GLenum modeAlpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.BlendEquationSeparate))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
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
    gpu_mutex_unlock (mutex);
}
	
void glBlendFunc (GLenum sfactor, GLenum dfactor)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.BlendFunc))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
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
    gpu_mutex_unlock (mutex);
}

void glBlendFuncSeparate (GLenum srcRGB, GLenum dstRGB, 
			  GLenum srcAlpha, GLenum dstAlpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.BlendFuncSeparate))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
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
    gpu_mutex_unlock (mutex);
}

void glBufferData (GLenum target, GLsizeiptr size, 
		   const GLvoid *data, GLenum usage)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if (! _is_error_state_or_func (active_state, dispatch.BufferData))
	goto FINISH;

    /* XXX: create a command buffer, we skip rest of check, because driver
     * can generate GL_OUT_OF_MEMORY, which cannot check
     */
    dispatch.BufferData (target, size, data, usage);
    egl_state = (egl_state_t *) active_state->data;
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glBufferSubData (GLenum target, GLintptr offset, 
		      GLsizeiptr size, const GLvoid *data)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if (! _is_error_state_or_func (active_state, dispatch.BufferSubData))
	goto FINISH;

    /* XXX: create a command buffer, we skip rest of check, because driver
     * can generate GL_INVALID_VALUE, when offset + data can be out of
     * bound
     */
    dispatch.BufferSubData (target, offset, size, data);
    egl_state = (egl_state_t *) active_state->data;
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GLenum glCheckFramebufferStatus (GLenum target)
{
    GLenum result = GL_INVALID_ENUM;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.CheckFramebufferStatus))
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
        egl_state_t *egl_state = (egl_state_t *) active_state->data;
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }
    
    /* XXX: create a command buffer, no need to set error code */
    dispatch.CheckFramebufferStatus (target);

FINISH:
    gpu_mutex_unlock (mutex);
    return result;
}

void glClear (GLbitfield mask)
{
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.Clear))
	goto FINISH;

    if (! (mask && GL_COLOR_BUFFER_BIT ||
	   mask && GL_DEPTH_BUFFER_BIT ||
	   mask && GL_STENCIL_BUFFER_BIT)) {
        egl_state_t *egl_state = (egl_state_t *) active_state->data;
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }
    
    /* XXX: create a command buffer, no need to set error code */
    dispatch.Clear (mask);

FINISH:
    gpu_mutex_unlock (mutex);
}

void glClearColor (GLclampf red, GLclampf green, 
		   GLclampf blue, GLclampf alpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.ClearColor))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
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
    gpu_mutex_unlock (mutex);
}

void glClearDepthf (GLclampf depth)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.ClearDepthf))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    state = &egl_state->state; 
    
    if (state->depth_clear_value == depth)
	goto FINISH;

    state->depth_clear_value = depth;

    /* XXX: command buffer */
    dispatch.ClearDepthf (depth);

FINISH:
    gpu_mutex_unlock (mutex);
}

void glClearStencil (GLint s)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.ClearStencil))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    state = &egl_state->state; 
    
    if (state->stencil_clear_value == s)
	goto FINISH;

    state->stencil_clear_value = s;

    /* XXX: command buffer */
    dispatch.ClearStencil (s);

FINISH:
    gpu_mutex_unlock (mutex);
}

void glColorMask (GLboolean red, GLboolean green, 
		  GLboolean blue, GLboolean alpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.ColorMask))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
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
    gpu_mutex_unlock (mutex);
}

void glCompressedTexImage2D (GLenum target, GLint level, 
			     GLenum internalformat, 
			     GLsizei width, GLsizei height,
			     GLint border, GLsizei imageSize,
			     const GLvoid *data)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.CompressedTexImage2D))
	goto FINISH;

    /* XXX: command buffer */
    dispatch.CompressedTexImage2D (target, level, internalformat,
				   width, height, border, imageSize,
				   data);
    egl_state = (egl_state_t *) active_state->data;
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glCompressedTexSubImage2D (GLenum target, GLint level,
				GLint xoffset, GLint yoffset,
				GLsizei width, GLsizei height,
				GLenum format, GLsizei imageSize,
				const GLvoid *data)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.CompressedTexSubImage2D))
	goto FINISH;

    /* XXX: command buffer */
    dispatch.CompressedTexSubImage2D (target, level, xoffset, yoffset,
				      width, height, format, imageSize, 
				      data);
    egl_state = (egl_state_t *) active_state->data;
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glCopyTexImage2D (GLenum target, GLint level,
		       GLenum internalformat, 
		       GLint x, GLint y,
		       GLsizei width, GLsizei height,
		       GLint border)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.CopyTexImage2D))
	goto FINISH;

    /* XXX: command buffer */
    dispatch.CopyTexImage2D (target, level, internalformat,
			     x, y, width, height, border);
    egl_state = (egl_state_t *) active_state->data;
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glCopyTexSubImage2D (GLenum target, GLint level, 
			  GLint xoffset, GLint yoffset,
			  GLint x, GLint y,
			  GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.CopyTexSubImage2D))
	goto FINISH;

    /* XXX: command buffer */
    dispatch.CopyTexSubImage2D (target, level, xoffset, yoffset,
			     x, y, width, height);
    egl_state = (egl_state_t *) active_state->data;
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

/* This is a sync call */
GLuint glCreateProgram (void)
{
    GLuint result = 0;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.CreateProgram))
	goto FINISH;

    /* XXX: command buffer and wait for signal, no error code generated */
    result = dispatch.CreateProgram ();

FINISH:
    gpu_mutex_unlock (mutex);
    
    return result;
}

GLuint glCreateShader (GLenum shaderType)
{
    egl_state_t *egl_state;

    GLuint result = 0;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.CreateShader))
	goto FINISH;

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
    gpu_mutex_unlock (mutex);
    
    return result;
}

void glCullFace (GLenum mode)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.CullFace))
	goto FINISH;
    
    egl_state = (egl_state_t *) active_state->data;
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
    gpu_mutex_unlock (mutex);
}

void glDeleteBuffers (GLsizei n, const GLuint *buffers)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.DeleteBuffers))
	goto FINISH;
    
    if (n < 0) {
	egl_state = (egl_state_t *) active_state->data;
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error == GL_INVALID_VALUE;
	goto FINISH;
    }

    /* XXX: command buffer, no error code generated */
    dispatch.DeleteBuffers (n, buffers);

FINISH:
    gpu_mutex_unlock (mutex);
}

void glDeleteFramebuffers (GLsizei n, const GLuint *framebuffers)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.DeleteFramebuffers))
	goto FINISH;
    
    if (n < 0) {
	egl_state = (egl_state_t *) active_state->data;
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error == GL_INVALID_VALUE;
	goto FINISH;
    }

    /* XXX: command buffer, no error code generated */
    dispatch.DeleteFramebuffers (n, framebuffers);

FINISH:
    gpu_mutex_unlock (mutex);
}

void glDeleteProgram (GLuint program)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.DeleteProgram))
	goto FINISH;
    
    /* XXX: command buffer */
    dispatch.DeleteProgram (program);
    egl_state = (egl_state_t *) active_state->data;
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glDeleteRenderbuffers (GLsizei n, const GLuint *renderbuffers)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.DeleteRenderbuffers))
	goto FINISH;
    
    if (n < 0) {
	egl_state = (egl_state_t *) active_state->data;
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error == GL_INVALID_VALUE;
	goto FINISH;
    }

    /* XXX: command buffer, no error code generated */
    dispatch.DeleteRenderbuffers (n, renderbuffers);

FINISH:
    gpu_mutex_unlock (mutex);
}

void glDeleteShader (GLuint shader)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.DeleteShader))
	goto FINISH;
    
    /* XXX: command buffer */
    dispatch.DeleteShader (shader);
    egl_state = (egl_state_t *) active_state->data;
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glDeleteTextures (GLsizei n, const GLuint *textures)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.DeleteTextures))
	goto FINISH;
    
    if (n < 0) {
	egl_state = (egl_state_t *) active_state->data;
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error == GL_INVALID_VALUE;
	goto FINISH;
    }

    /* XXX: command buffer, no error code generated */
    dispatch.DeleteTextures (n, textures);

FINISH:
    gpu_mutex_unlock (mutex);
}

void glDepthFunc (GLenum func)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.DepthFunc))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
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
    gpu_mutex_unlock (mutex);
}

void glDepthMask (GLboolean flag)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.DepthMask))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    if (egl_state->state.depth_writemask == flag)
	goto FINISH;
    
    egl_state->state.depth_writemask = flag;
    
    /* XXX: command buffer, no error code generated */
    dispatch.DepthMask (flag);

FINISH:
    gpu_mutex_unlock (mutex);
}

void glDepthRangef (GLclampf nearVal, GLclampf farVal)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.DepthRangef))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    if (egl_state->state.depth_range[0] == nearVal &&
	egl_state->state.depth_range[1] == farVal)
	goto FINISH;
    
    egl_state->state.depth_range[0] = nearVal;
    egl_state->state.depth_range[1] = farVal;
    
    /* XXX: command buffer, no error code generated */
    dispatch.DepthRangef (nearVal, farVal);

FINISH:
    gpu_mutex_unlock (mutex);
}

void glDetachShader (GLuint program, GLuint shader)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.DetachShader))
	goto FINISH;

    /* XXX: command buffer, error code generated */
    dispatch.DetachShader (program, shader);
    egl_state = (egl_state_t *) active_state->data;
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

static void
_gl_set_cap (GLenum cap, GLboolean enable)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    v_bool_t needs_call = FALSE;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.Disable))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
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
	if (state->error == GL_NO_ERROR)
	    state->error = GL_INVALID_ENUM;
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
    gpu_mutex_unlock (mutex);
}

void glDisable (GLenum cap)
{
    _gl_set_cap (cap, GL_FALSE);
}

void glEnable (GLenum cap)
{
    _gl_set_cap (cap, GL_TRUE);
}

void glDisableVertexAttribArray (GLuint index) 
{
    egl_state_t *egl_state;

    v_vertex_attrib_list_t *attrib_list = &cli_states.vertex_attribs;
    v_vertex_attrib_t *attribs = attrib_list->attribs;
    int count = attrib_list->count;
    int i, found_index = -1;

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
    
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.DisableVertexAttribArray))
	return;
    

    egl_state = (egl_state_t *) active_state->data;
    /* gles2 spec says at least 8 */
    /* XXX: command buffer */
    if (index >= egl_state->state.max_vertex_attribs) {
	if (! egl_state->state.max_vertex_attribs_queried) {
	/* XXX: command buffer */
	    dispatch.GetIntegerv (GL_MAX_VERTEX_ATTRIBS,
				  &(egl_state->state.max_vertex_attribs));
	}
	if (index >= egl_state->state.max_vertex_attribs) {
	    if (egl_state->state.error == GL_NO_ERROR)
		egl_state->state.error = GL_INVALID_VALUE;
	    gpu_mutex_unlock (mutex);
	    return;
	}
    }

    gpu_mutex_unlock (mutex);

    /* XXX: command buffer, error code generated */
    dispatch.DisableVertexAttribArray (index);

    /* update client state */
    if (found_index != -1)
	return;

    if (i < NUM_EMBEDDED) {
	attribs[i].array_enabled = FALSE;
	attribs[i].index = index;
    }
    else {
	v_vertex_attrib_t *new_attribs = (v_vertex_attrib_t *)malloc (sizeof (v_vertex_attrib_t) * (count + 1));

	memcpy (new_attribs, attribs, (count+1) * sizeof (v_vertex_attrib_t));
	if (attribs != attrib_list->embedded_attribs)
	    free (attribs);

	new_attribs[count].index = index;
	new_attribs[count].array_enabled = FALSE;
	new_attribs[count].data = NULL;
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

    gpu_mutex_lock (mutex);
    if(! _is_error_state_or_func (active_state, 
				  dispatch.EnableVertexAttribArray))
	return;

    egl_state = (egl_state_t *) active_state->data;
    /* gles2 spec says at least 8 */
    /* XXX: command buffer */
    if (index >= egl_state->state.max_vertex_attribs) {
	if (! egl_state->state.max_vertex_attribs_queried) {
	/* XXX: command buffer */
	    dispatch.GetIntegerv (GL_MAX_VERTEX_ATTRIBS,
				  &(egl_state->state.max_vertex_attribs));
	}
	if (index >= egl_state->state.max_vertex_attribs) {
	    if (egl_state->state.error == GL_NO_ERROR)
		egl_state->state.error == GL_INVALID_VALUE;
	    gpu_mutex_unlock (mutex);
	    return;
	}
    }

    gpu_mutex_unlock (mutex);

    /* XXX: command buffer, error code generated */
    dispatch.EnableVertexAttribArray (index);
    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (mutex);

    /* update client state */
    if (found_index != -1)
	return;

    if (i < NUM_EMBEDDED) {
	attribs[i].array_enabled = TRUE;
	attribs[i].index = index;
    }
    else {
	v_vertex_attrib_t *new_attribs = (v_vertex_attrib_t *)malloc (sizeof (v_vertex_attrib_t) * (count + 1));

	memcpy (new_attribs, attribs, (count+1) * sizeof (v_vertex_attrib_t));
	if (attribs != attrib_list->embedded_attribs)
	    free (attribs);

	new_attribs[count].index = index;
	new_attribs[count].array_enabled = TRUE;
	new_attribs[count].data = NULL;
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

    if (mode != GL_POINTS && mode != GL_LINE_STRIP &&
	mode != GL_LINE_LOOP && mode != GL_LINES &&
	mode != GL_TRIANGLE_STRIP && mode != GL_TRIANGLE_FAN &&
	mode != GL_TRIANGLES) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	return;
    }
    else if (count < 0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_VALUE;
	return;
    }

    /* check buffer binding, if there is a buffer, we don't need to 
     * copy data
     */
    gpu_mutex_lock (mutex);

    for (i = 0; i < attrib_list->count; i++) {
	if (! attribs[i].array_enabled)
	    continue;
	else if (attribs[i].array_buffer_binding) {
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


    gpu_mutex_unlock (mutex);
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
				  dispatch.DrawArrays))
	return;

    egl_state = (egl_state_t *)active_state->data;

    if (mode != GL_POINTS && mode != GL_LINE_STRIP &&
	mode != GL_LINE_LOOP && mode != GL_LINES &&
	mode != GL_TRIANGLE_STRIP && mode != GL_TRIANGLE_FAN &&
	mode != GL_TRIANGLES) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	return;
    }
    else if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	return;
    }
    else if (count < 0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_VALUE;
	return;
    }

    /* check buffer binding, if there is a buffer, we don't need to 
     * copy data
     */
    gpu_mutex_lock (mutex);

    for (i = 0; i < attrib_list->count; i++) {
	if (! attribs[i].array_enabled)
	    continue;
	else if ( attribs[i].array_buffer_binding) {
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

    /* create indices */
    data = _gl_create_indices_array (mode, type, count, (char *)indices);

    // we need put DrawArrays
    dispatch.DrawElements (mode, type, count, (const GLvoid *)data);

    array = array_data;
    while (array != NULL) {
	new_array_data = array;
	array = array->next;
	free (new_array_data->data);
	free (new_array_data);
    }
    free (data);

    egl_state->state.need_get_error = TRUE;

    gpu_mutex_unlock (mutex);
}

void glFinish (void)
{
    /* XXX: put to a command buffer, wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.Finish))
	goto FINISH;

    dispatch.Finish ();
 
FINISH:
    gpu_mutex_unlock (mutex);
}

void glFlush (void)
{
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.Flush))
	goto FINISH;

    dispatch.Flush ();
 
FINISH:
    gpu_mutex_unlock (mutex);
}

void glFramebufferRenderbuffer (GLenum target, GLenum attachment,
				GLenum renderbuffertarget,
				GLenum renderbuffer)
{
    egl_state_t *egl_state;
    
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.FramebufferRenderbuffer))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

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
    gpu_mutex_unlock (mutex);
}

void glFramebufferTexture2D (GLenum target, GLenum attachment,
			     GLenum textarget, GLuint texture, GLint level)
{
    egl_state_t *egl_state;
    
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.FramebufferTexture2D))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    if (target != GL_FRAMEBUFFER) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    dispatch.FramebufferTexture2D (target, attachment, textarget,
				   texture, level);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glFrontFace (GLenum mode)
{
    egl_state_t *egl_state;

    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.FrontFace))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

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
    gpu_mutex_unlock (mutex);
}

void glGenBuffers (GLsizei n, GLuint *buffers)
{
    egl_state_t *egl_state;

    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GenBuffers))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    if (n < 0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_VALUE;
	goto FINISH;
    }

    /* XXX: create a command buffer and wait for signal */
    dispatch.GenBuffers (n, buffers);

FINISH:
    gpu_mutex_unlock (mutex);
}

void glGenFramebuffers (GLsizei n, GLuint *framebuffers)
{
    egl_state_t *egl_state;

    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GenFramebuffers))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    if (n < 0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_VALUE;
	goto FINISH;
    }

    /* XXX: create a command buffer and wait for signal */
    dispatch.GenFramebuffers (n, framebuffers);

FINISH:
    gpu_mutex_unlock (mutex);
}

void glGenRenderbuffers (GLsizei n, GLuint *renderbuffers)
{
    egl_state_t *egl_state;

    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GenRenderbuffers))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    if (n < 0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_VALUE;
	goto FINISH;
    }

    /* XXX: create a command buffer and wait for signal */
    dispatch.GenRenderbuffers (n, renderbuffers);

FINISH:
    gpu_mutex_unlock (mutex);
}

void glGenTextures (GLsizei n, GLuint *textures)
{
    egl_state_t *egl_state;

    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GenTextures))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    if (n < 0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_VALUE;
	goto FINISH;
    }

    /* XXX: create a command buffer and wait for signal */
    dispatch.GenTextures (n, textures);

FINISH:
    gpu_mutex_unlock (mutex);
}

void glGenerateMipmap (GLenum target)
{
    egl_state_t *egl_state;

    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GenerateMipmap))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

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
    gpu_mutex_unlock (mutex);
}

void glGetBooleanv (GLenum pname, GLboolean *params)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetBooleanv))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

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

FINISH:
    gpu_mutex_unlock (mutex);
}

void glGetFloatv (GLenum pname, GLfloat *params)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetFloatv))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

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
    gpu_mutex_unlock (mutex);
}

void glGetIntegerv (GLenum pname, GLint *params)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetIntegerv))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

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
    default:
	/* XXX: command buffer, and wait for signal */
	dispatch.GetIntegerv (pname, params);
	break;
    }
  
FINISH:
    gpu_mutex_unlock (mutex);
}

void glGetActiveAttrib (GLuint program, GLuint index, 
			GLsizei bufsize, GLsizei *length,
			GLint *size, GLenum *type, GLchar *name)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetActiveAttrib))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetActiveAttrib (program, index, bufsize, length,
			      size, type, name);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glGetActiveUniform (GLuint program, GLuint index, GLsizei bufsize,
			 GLsizei *length, GLint *size, GLenum *type,
			 GLchar *name)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetActiveUniform))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetActiveUniform (program, index, bufsize, length,
			       size, type, name);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glGetAttachedShaders (GLuint program, GLsizei maxCount, 
			   GLsizei *count, GLuint *shaders)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetAttachedShaders))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetAttachedShaders (program, maxCount, count, shaders);
    
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GLint glGetAttribLocation (GLuint program, const GLchar *name)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetAttribLocation))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetAttribLocation (program, name);
    
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glGetBufferParameteriv (GLenum target, GLenum value, GLint *data)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetBufferParameteriv))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetBufferParameteriv (target, value, data);
    
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GLenum glGetError (void)
{
    egl_state_t *egl_state;

    GLenum error = GL_INVALID_OPERATION;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetError))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;
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
    gpu_mutex_unlock (mutex);

    return error;
}
   
void glGetFramebufferAttachmentParameteriv (GLenum target, 
					    GLenum attachment,
					    GLenum pname,
					    GLint *params)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetFramebufferAttachmentParameteriv))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

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
    gpu_mutex_unlock (mutex);
}

void glGetProgramInfoLog (GLuint program, GLsizei maxLength,
			  GLsizei *length, GLchar *infoLog)
{ 
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetProgramInfoLog))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetProgramInfoLog (program, maxLength, length, infoLog);

    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glGetProgramiv (GLuint program, GLenum pname, GLint *params)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetProgramiv))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetProgramiv (program, pname, params);

    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glGetRenderbufferParameteriv (GLenum target, 
				   GLenum pname,
				   GLint *params)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetRenderbufferParameteriv))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    if (target != GL_RENDERBUFFER) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetRenderbufferParameteriv (target, pname, params);

    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glGetShaderInfoLog (GLuint program, GLsizei maxLength,
			 GLsizei *length, GLchar *infoLog)
{ 
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetShaderInfoLog))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetShaderInfoLog (program, maxLength, length, infoLog);

    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glGetShaderPrecisionFormat (GLenum shaderType, GLenum precisionType,
				 GLint *range, GLint *precision)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetShaderPrecisionFormat))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetShaderPrecisionFormat (shaderType, precisionType,
				       range, precision);

    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glGetShaderSource (GLuint shader, GLsizei bufSize, GLsizei *length,
			GLchar *source)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetShaderSource))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetShaderSource (shader, bufSize, length, source);

    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glGetShaderiv (GLuint shader, GLenum pname, GLint *params)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetShaderiv))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetShaderiv (shader, pname, params);

    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

const GLubyte *glGetString (GLenum name)
{
    GLubyte *result = NULL;

    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetString))
	goto FINISH;
	
    egl_state = (egl_state_t *)active_state->data;

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
    gpu_mutex_unlock (mutex);
    return (const GLubyte *)result;
}

void glGetTexParameteriv (GLenum target, GLenum pname, GLint *params)
{
    egl_state_t *egl_state;
    int active_texture_index;
    int target_index;

    gpu_mutex_lock (mutex);
    
    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetTexParameteriv))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

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
    gpu_mutex_unlock (mutex);
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

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetUniformiv))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetUniformiv (program, location, params);

    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glGetUniformfv (GLuint program, GLint location, GLfloat *params)
{ 
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetUniformfv))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    /* XXX: create a command buffer and wait for signal */
    dispatch.GetUniformfv (program, location, params);

    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GLint glGetUniformLocation (GLuint program, const GLchar *name)
{
    GLint result = -1;
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetUniformLocation))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    /* XXX: create a command buffer and wait for signal */
    result = dispatch.GetUniformLocation (program, name);

    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);

    return result;
}

void glGetVertexAttribfv (GLuint index, GLenum pname, GLfloat *params)
{
    egl_state_t *egl_state;

    v_vertex_attrib_list_t *attrib_list = &cli_states.vertex_attribs;
    v_vertex_attrib_t *attribs = attrib_list->attribs;
    int count = attrib_list->count;
    int i, found_index = -1;
    
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetVertexAttribfv))
	return;

    egl_state = (egl_state_t *) active_state->data;

    if (! (pname == GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING ||
	   pname == GL_VERTEX_ATTRIB_ARRAY_ENABLED	 ||
	   pname == GL_VERTEX_ATTRIB_ARRAY_SIZE		 ||
	   pname == GL_VERTEX_ATTRIB_ARRAY_STRIDE	 ||
	   pname == GL_VERTEX_ATTRIB_ARRAY_TYPE		 ||
	   pname == GL_VERTEX_ATTRIB_ARRAY_NORMALIZED	 ||
	   pname == GL_CURRENT_VERTEX_ATTRIB)) {
	if (egl_state->state.error != GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	gpu_mutex_unlock (mutex);
	return;
    }
    
    /* gles2 spec says at least 8 */
    /* XXX: command buffer */
    if (index >= egl_state->state.max_vertex_attribs) {
	if (! egl_state->state.max_vertex_attribs_queried) {
	/* XXX: command buffer */
	    dispatch.GetIntegerv (GL_MAX_VERTEX_ATTRIBS,
				  &(egl_state->state.max_vertex_attribs));
	}
	if (index >= egl_state->state.max_vertex_attribs) {
	    if (egl_state->state.error = GL_NO_ERROR)
		egl_state->state.error = GL_INVALID_VALUE;
	    gpu_mutex_unlock (mutex);
	    return;
	}
    }

    gpu_mutex_unlock (mutex);

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
	memcpy (params, 0, sizeof (GLfloat) * 4);
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
    
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetVertexAttribPointerv))
	return;

    egl_state = (egl_state_t *) active_state->data;

    if (pname != GL_VERTEX_ATTRIB_ARRAY_POINTER) {
	if (egl_state->state.error != GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	gpu_mutex_unlock (mutex);
	return;
    }
    
    /* gles2 spec says at least 8 */
    /* XXX: command buffer */
    if (index >= egl_state->state.max_vertex_attribs) {
	if (! egl_state->state.max_vertex_attribs_queried) {
	/* XXX: command buffer */
	    dispatch.GetIntegerv (GL_MAX_VERTEX_ATTRIBS,
				  &(egl_state->state.max_vertex_attribs));
	}
	if (index >= egl_state->state.max_vertex_attribs) {
	    if (egl_state->state.error = GL_NO_ERROR)
		egl_state->state.error = GL_INVALID_VALUE;
	    gpu_mutex_unlock (mutex);
	    return;
	}
    }

    gpu_mutex_unlock (mutex);

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

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.Hint))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

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
    gpu_mutex_unlock (mutex);
}

GLboolean glIsBuffer (GLuint buffer)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.IsBuffer))
	goto FINISH;

    /* XXX: create a command buffer, wait for signal */
    result = dispatch.IsBuffer (buffer);

FINISH:
    gpu_mutex_unlock (mutex);
    return result;
}

GLboolean glIsEnabled (GLenum cap)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.IsEnabled))
	goto FINISH;

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
    gpu_mutex_unlock (mutex);
    return result;
}

GLboolean glIsFramebuffer (GLuint framebuffer)
{
    GLboolean result = GL_FALSE;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.IsFramebuffer))
	goto FINISH;

    /* XXX: create command buffer, wait for signal */
    result = dispatch.IsFramebuffer (framebuffer);
FINISH:
    gpu_mutex_unlock (mutex);
    return result;
}

GLboolean glIsProgram (GLuint program)
{
    GLboolean result = GL_FALSE;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.IsProgram))
	goto FINISH;

    /* XXX: create command buffer, wait for signal */
    result = dispatch.IsProgram (program);
FINISH:
    gpu_mutex_unlock (mutex);
    return result;
}

GLboolean glIsRenderbuffer (GLuint renderbuffer)
{
    GLboolean result = GL_FALSE;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.IsRenderbuffer))
	goto FINISH;

    /* XXX: create command buffer, wait for signal */
    result = dispatch.IsRenderbuffer (renderbuffer);
FINISH:
    gpu_mutex_unlock (mutex);
    return result;
}

GLboolean glIsShader (GLuint shader)
{
    GLboolean result = GL_FALSE;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.IsShader))
	goto FINISH;

    /* XXX: create command buffer, wait for signal */
    result = dispatch.IsShader (shader);
FINISH:
    gpu_mutex_unlock (mutex);
    return result;
}

GLboolean glIsTexture (GLuint texture)
{
    GLboolean result = GL_FALSE;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.IsTexture))
	goto FINISH;

    /* XXX: create command buffer, wait for signal */
    result = dispatch.IsTexture (texture);
FINISH:
    gpu_mutex_unlock (mutex);
    return result;
}

void glLineWidth (GLfloat width)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.LineWidth))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

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
    gpu_mutex_unlock (mutex);
}

void glLinkProgram (GLuint program)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.LinkProgram))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

    /* XXX: create command buffer, wait for signal */
    dispatch.LinkProgram (program);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glPixelStorei (GLenum pname, GLint param)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.PixelStorei))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
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
    gpu_mutex_unlock (mutex);
}

void glPolygonOffset (GLfloat factor, GLfloat units)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.PolygonOffset))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    if (egl_state->state.polygon_offset_factor == factor &&
	egl_state->state.polygon_offset_units == units)    
	goto FINISH;

    egl_state->state.polygon_offset_factor = factor;
    egl_state->state.polygon_offset_units = units;

    /* XXX: create command buffer, no wait */
    dispatch.PolygonOffset (factor, units);

FINISH:
    gpu_mutex_unlock (mutex);
}

void glReadPixels (GLint x, GLint y,
		   GLsizei width, GLsizei height,
		   GLenum format, GLenum type, GLvoid *data)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.ReadPixels))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, wait for signal */
    dispatch.ReadPixels (x, y, width, height, format, type, data);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glReleaseShaderCompiler (void)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.ReleaseShaderCompiler))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, no wait */
    dispatch.ReleaseShaderCompiler ();
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glRenderbufferStorage (GLenum target, GLenum internalformat,
			   GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.RenderbufferStorage))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, no wait */
    dispatch.RenderbufferStorage (target, internalformat, width, height);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glSampleCoverage (GLclampf value, GLboolean invert)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.SampleCoverage))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    if (value == egl_state->state.sample_coverage_value &&
	invert == egl_state->state.sample_coverage_invert) 
	goto FINISH;

    egl_state->state.sample_coverage_invert = invert;
    egl_state->state.sample_coverage_value = value;
    
    /* XXX: create command buffer, no wait */
    dispatch.SampleCoverage (value, invert);

FINISH:
    gpu_mutex_unlock (mutex);
}

void glScissor (GLint x, GLint y, GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.Scissor))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

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
    gpu_mutex_unlock (mutex);
}

void glShaderBinary (GLsizei n, const GLuint *shaders, 
		     GLenum binaryformat, const void *binary,
		     GLsizei length)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.ShaderBinary))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, wait for signal */
    dispatch.ShaderBinary (n, shaders, binaryformat, binary, length);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glShaderSource (GLuint shader, GLsizei count,
		     const GLchar **string, const GLint *length) 
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.ShaderSource))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, wait for signal */
    dispatch.ShaderSource (shader, count, string, length);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
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

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.StencilFuncSeparate))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

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
    gpu_mutex_unlock (mutex);
}

void glStencilMaskSeparate (GLenum face, GLuint mask)
{
    egl_state_t *egl_state;
    v_bool_t needs_call = FALSE;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.StencilMaskSeparate))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

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
    gpu_mutex_unlock (mutex);
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

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.StencilOpSeparate))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

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
    gpu_mutex_unlock (mutex);
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

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.TexImage2D))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, wait for signal */
    dispatch.TexImage2D (target, level, internalformat, width, height, 
			 border, format, type, data);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glTexParameteri (GLenum target, GLenum pname, GLint param)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    int active_texture_index;
    int target_index;

    gpu_mutex_lock (mutex);
    
    if(! _is_error_state_or_func (active_state, 
				  dispatch.TexParameteri))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
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
    gpu_mutex_unlock (mutex);
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

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.TexSubImage2D))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, wait for signal */
    dispatch.TexSubImage2D (target, level, xoffset, yoffset, 
			    width, height, format, type, data);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}
