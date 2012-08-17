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
    v_vertex_attrib_list_t *attrib_list = &cli_states.vertex_attribs;
    v_vertex_attrib_t *attribs = attrib_list->attribs;
    int count = attrib_list->count;
    int i;
    
    gpu_mutex_lock (mutex);

    if (target == GL_ARRAY_BUFFER && 
	buffer == egl_state->state.array_buffer_binding)
	goto FINISH;
    else if (target == GL_ELEMENT_ARRAY_BUFFER &&
	     buffer == egl_state->state.element_array_buffer_binding)
	goto FINISH; 

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

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.BindFramebuffer))
	goto FINISH;

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
	egl_state = (egl_state_t *) active_state->data;
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

    //egl_state->state.need_get_error = TRUE;

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
    
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.DisableVertexAttribArray))
	return;
    

    egl_state = (egl_state_t *) active_state->data;
    /* gles2 spec says at least 8 */
    /* XXX: command buffer */
    if (_gl_index_is_too_large (&egl_state->state, index)) {
	gpu_mutex_unlock (mutex);
	return;
    }

    /* XXX: command buffer, error code generated */
    dispatch.DisableVertexAttribArray (index);

    bound_buffer = egl_state->state.array_buffer_binding;
    gpu_mutex_unlock (mutex);

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

    gpu_mutex_lock (mutex);
    if(! _is_error_state_or_func (active_state, 
				  dispatch.EnableVertexAttribArray))
	return;

    egl_state = (egl_state_t *) active_state->data;
    /* gles2 spec says at least 8 */
    /* XXX: command buffer */
    if (_gl_index_is_too_large (&egl_state->state, index)) {
	gpu_mutex_unlock (mutex);
	return;
    }

    /* XXX: command buffer, error code generated */
    dispatch.EnableVertexAttribArray (index);

    bound_buffer = egl_state->state.array_buffer_binding;

    gpu_mutex_unlock (mutex);

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
    
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.DrawArrays)) {
	gpu_mutex_unlock (mutex);
	return;
    }
    
    egl_state = (egl_state_t *)active_state->data;

    if (mode != GL_POINTS && mode != GL_LINE_STRIP &&
	mode != GL_LINE_LOOP && mode != GL_LINES &&
	mode != GL_TRIANGLE_STRIP && mode != GL_TRIANGLE_FAN &&
	mode != GL_TRIANGLES) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	gpu_mutex_unlock (mutex);
	return;
    }
    else if (count < 0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_VALUE;
	gpu_mutex_unlock (mutex);
	return;
    }

#ifdef GL_OES_vertex_array_object
    if (egl_state->state.vertex_array_binding) {
	dispatch.DrawArrays (mode, first, count);
	egl_state->state.need_get_error = TRUE;
	gpu_mutex_unlock (mutex);
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
    
    gpu_mutex_lock (mutex);
    
    if(! _is_error_state_or_func (active_state, 
				  dispatch.DrawElements)) {
	gpu_mutex_unlock (mutex);
	return;
    }
    egl_state = (egl_state_t *)active_state->data;

#ifdef GL_OES_vertex_array_object
    if (egl_state->state.vertex_array_binding) {
	dispatch.DrawElements (mode, count, type, indices);
	egl_state->state.need_get_error = TRUE;
	gpu_mutex_unlock (mutex);
	return;
    }
#endif

    if (mode != GL_POINTS && mode != GL_LINE_STRIP &&
	mode != GL_LINE_LOOP && mode != GL_LINES &&
	mode != GL_TRIANGLE_STRIP && mode != GL_TRIANGLE_FAN &&
	mode != GL_TRIANGLES) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	gpu_mutex_unlock (mutex);
	return;
    }
    else if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	gpu_mutex_unlock (mutex);
	return;
    }
    else if (count < 0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_VALUE;
	gpu_mutex_unlock (mutex);
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
    v_vertex_attrib_list_t *attrib_list = &cli_states.vertex_attribs;
    v_vertex_attrib_t *attribs = attrib_list->attribs;
    int count = attrib_list->count;
    int i;

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
    if (_gl_index_is_too_large (&egl_state->state, index)) {
	gpu_mutex_unlock (mutex);
	return;
    }
    
#ifdef GL_OES_vertex_array_object
    /* we cannot use client state */
    if (egl_state->state.vertex_array_binding) {
	dispatch.GetVertexAttribfv (index, pname, params);
	gpu_mutex_unlock (mutex);
	return;
    }
#endif

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
    if (_gl_index_is_too_large (&egl_state->state, index)) {
	gpu_mutex_unlock (mutex);
	return;
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
    
    /* XXX: create command buffer, no wait */
    dispatch.TexSubImage2D (target, level, xoffset, yoffset, 
			    width, height, format, type, data);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glUniform1f (GLint location, GLfloat v0)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.Uniform1f))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, no wait */
    dispatch.Uniform1f (location, v0);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glUniform2f (GLint location, GLfloat v0, GLfloat v1)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.Uniform2f))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, no wait */
    dispatch.Uniform2f (location, v0, v1);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glUniform3f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.Uniform3f))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, no wait */
    dispatch.Uniform3f (location, v0, v1, v2);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glUniform4f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2,
		  GLfloat v3)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.Uniform4f))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, no wait */
    dispatch.Uniform4f (location, v0, v1, v2, v3);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glUniform1i (GLint location, GLint v0)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.Uniform1i))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, not wait */
    dispatch.Uniform1i (location, v0);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glUniform2i (GLint location, GLint v0, GLint v1)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.Uniform2i))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, not wait */
    dispatch.Uniform2i (location, v0, v1);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glUniform3i (GLint location, GLint v0, GLint v1, GLint v2)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.Uniform3i))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, no wait */
    dispatch.Uniform3i (location, v0, v1, v2);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glUniform4i (GLint location, GLint v0, GLint v1, GLint v2,
		  GLint v3)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.Uniform4i))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, not wait */
    dispatch.Uniform4i (location, v0, v1, v2, v3);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

static void 
_gl_uniform_fv (int i, GLint location, 
		GLsizei count, const GLfloat *value)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if (i == 1) {
	if(! _is_error_state_or_func (active_state, 
				      dispatch.Uniform1fv))
	goto FINISH;
    }
    else if (i == 2) {
	if(! _is_error_state_or_func (active_state, 
				      dispatch.Uniform2fv))
	goto FINISH;
    }
    else if (i == 3) {
	if(! _is_error_state_or_func (active_state, 
				      dispatch.Uniform3fv))
	goto FINISH;
    }
    else {
	if(! _is_error_state_or_func (active_state, 
				      dispatch.Uniform4fv))
	goto FINISH;
    }

    egl_state = (egl_state_t *) active_state->data;
    
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
    gpu_mutex_unlock (mutex);
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

    gpu_mutex_lock (mutex);

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
    gpu_mutex_unlock (mutex);
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

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.UseProgram))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    if (egl_state->state.current_program == program)
	goto FINISH;
    
    /* XXX: create command buffer, no wait signal */
    dispatch.UseProgram (program);
    /* FIXME: this maybe not right because this texture may be invalid
     * object, we save here to save time in glGetError() */
    egl_state->state.current_program = program;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glValidateProgram (GLuint program)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.ValidateProgram))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    if (egl_state->state.current_program == program)
	goto FINISH;
    
    /* XXX: create command buffer, no wait signal */
    dispatch.ValidateProgram (program);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

void glVertexAttrib1f (GLuint index, GLfloat v0)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.VertexAttrib1f))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

    if (_gl_index_is_too_large (&egl_state->state, index)) {
	gpu_mutex_unlock (mutex);
	return;
    }
    
    /* XXX: create command buffer, no wait signal */
    dispatch.VertexAttrib1f (index, v0);

FINISH:
    gpu_mutex_unlock (mutex);
}

void glVertexAttrib2f (GLuint index, GLfloat v0, GLfloat v1)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.VertexAttrib2f))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    if (_gl_index_is_too_large (&egl_state->state, index)) {
	gpu_mutex_unlock (mutex);
	return;
    }
    
    /* XXX: create command buffer, no wait signal */
    dispatch.VertexAttrib2f (index, v0, v1);

FINISH:
    gpu_mutex_unlock (mutex);
}

void glVertexAttrib3f (GLuint index, GLfloat v0, GLfloat v1, GLfloat v2)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.VertexAttrib3f))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    if (_gl_index_is_too_large (&egl_state->state, index)) {
	gpu_mutex_unlock (mutex);
	return;
    }
    
    /* XXX: create command buffer, no wait signal */
    dispatch.VertexAttrib3f (index, v0, v1, v2);

FINISH:
    gpu_mutex_unlock (mutex);
}

void glVertexAttrib4f (GLuint index, GLfloat v0, GLfloat v1, GLfloat v2,
		       GLfloat v3)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.VertexAttrib4f))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    if (_gl_index_is_too_large (&egl_state->state, index)) {
	gpu_mutex_unlock (mutex);
	return;
    }
    
    /* XXX: create command buffer, no wait signal */
    dispatch.VertexAttrib4f (index, v0, v1, v2, v3);

FINISH:
    gpu_mutex_unlock (mutex);
}

static void 
_gl_vertex_attrib_fv (int i, GLuint index, const GLfloat *v)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if (i == 1) {
	if(! _is_error_state_or_func (active_state, 
				      dispatch.VertexAttrib1fv))
	goto FINISH;
    }
    else if (i == 2) {
	if(! _is_error_state_or_func (active_state, 
				      dispatch.VertexAttrib2fv))
	goto FINISH;
    }
    else if (i == 3) {
	if(! _is_error_state_or_func (active_state, 
				      dispatch.VertexAttrib3fv))
	goto FINISH;
    }
    else {
	if(! _is_error_state_or_func (active_state, 
				      dispatch.VertexAttrib4fv))
	goto FINISH;
    }

    egl_state = (egl_state_t *) active_state->data;

    if (_gl_index_is_too_large (&egl_state->state, index)) {
	gpu_mutex_unlock (mutex);
	return;
    }
    
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
    gpu_mutex_unlock (mutex);
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

#ifdef GL_OES_vertex_array_object
    gpu_mutex_lock (mutex);
    if (! active_state && ! active_state->data) {
	gpu_mutex_unlock (mutex);
	return;
    }

    egl_state = (egl_state_t *) active_state->data;
    if (egl_state->state.vertex_array_binding) {
	dispatch.VertexAttribPointer (index, size, type, normalized,
				      stride, pointer);
	egl_state->state.need_get_error = TRUE;
	gpu_mutex_unlock (mutex);
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
	gpu_mutex_unlock (mutex);
	return;
    }
    else if (size > 4 || size < 1 || stride < 0) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_VALUE;
	gpu_mutex_unlock (mutex);
	return;
    }

    /* check max_vertex_attribs */
    if (_gl_index_is_too_large (&egl_state->state, index)) {
	gpu_mutex_unlock (mutex);
	return;
    }

    bound_buffer = egl_state->state.array_buffer_binding;

    gpu_mutex_unlock (mutex);

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

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.Viewport))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

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
    gpu_mutex_unlock (mutex);
}
/* end of GLES2 core profile */

#ifdef GL_OES_EGL_image
GL_APICALL void GL_APIENTRY
glEGLImageTargetTexture2DOES (GLenum target, GLeglImageOES image)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.EGLImageTargetTexture2DOES))
	goto FINISH;

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
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY 
glEGLImageTargetRenderbufferStorageOES (GLenum target, GLeglImageOES image)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.EGLImageTargetRenderbufferStorageOES))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

    if (target != GL_RENDERBUFFER_OES) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    
    /* XXX: create command buffer, no wait signal */
    dispatch.EGLImageTargetRenderbufferStorageOES (target, image);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}
#endif

#ifdef GL_OES_get_program_binary
GL_APICALL void GL_APIENTRY
glGetProgramBinaryOES (GLuint program, GLsizei bufSize, GLsizei *length,
		       GLenum *binaryFormat, GLvoid *binary)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetProgramBinaryOES))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

    /* XXX: create command buffer, no wait signal */
    dispatch.GetProgramBinaryOES (program, bufSize, length, binaryFormat,
				  binary);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glProgramBinaryOES (GLuint program, GLenum binaryFormat, 
		    const GLvoid *binary, GLint length)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.ProgramBinaryOES))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

    /* XXX: create command buffer, no wait signal */
    dispatch.ProgramBinaryOES (program, binaryFormat, binary, length);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}
#endif

#ifdef GL_OES_mapbuffer
GL_APICALL void* GL_APIENTRY glMapBufferOES (GLenum target, GLenum access)
{
    egl_state_t *egl_state;
    void *result = NULL;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.MapBufferOES))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

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
    gpu_mutex_unlock (mutex);
    return result;
}

GL_APICALL GLboolean GL_APIENTRY 
glUnmapBufferOES (GLenum target)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.UnmapBufferOES))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

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
    gpu_mutex_unlock (mutex);
    return result;
}

GL_APICALL void GL_APIENTRY
glGetBufferPointervOES (GLenum target, GLenum pname, GLvoid **params)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetBufferPointervOES)) {
	*params = NULL;
	goto FINISH;
    }

    egl_state = (egl_state_t *) active_state->data;

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
    gpu_mutex_unlock (mutex);
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

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.TexImage3DOES))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, wait for signal */
    dispatch.TexImage3DOES (target, level, internalformat, width, height, 
			    depth, border, format, type, pixels);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glTexSubImage3DOES (GLenum target, GLint level, 
		    GLint xoffset, GLint yoffset, GLint zoffset,
		    GLsizei width, GLsizei height, GLsizei depth, 
		    GLenum format, GLenum type, const GLvoid *data)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.TexSubImage3DOES))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, no wait */
    dispatch.TexSubImage3DOES (target, level, xoffset, yoffset, zoffset, 
			       width, height, depth, format, type, data);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY 
glCopyTexSubImage3DOES (GLenum target, GLint level, 
			GLint xoffset, GLint yoffset, GLint zoffset,
			GLint x, GLint y, 
			GLint width, GLint height)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.CopyTexSubImage3DOES))
	goto FINISH;

    /* XXX: command buffer */
    dispatch.CopyTexSubImage3DOES (target, level, 
				   xoffset, yoffset, zoffset,
				   x, y, width, height);
    egl_state = (egl_state_t *) active_state->data;
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY 
glCompressedTexImage3DOES (GLenum target, GLint level, 
			   GLenum internalformat, 
			   GLsizei width, GLsizei height, GLsizei depth,
			   GLint border, GLsizei imageSize,
			   const GLvoid *data)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.CompressedTexImage3DOES))
	goto FINISH;

    /* XXX: command buffer */
    dispatch.CompressedTexImage3DOES (target, level, internalformat,
				      width, height, depth, 
				      border, imageSize, data);
    egl_state = (egl_state_t *) active_state->data;
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glCompressedTexSubImage3DOES (GLenum target, GLint level,
			      GLint xoffset, GLint yoffset, GLint zoffset,
			      GLsizei width, GLsizei height, GLsizei depth,
			      GLenum format, GLsizei imageSize,
			      const GLvoid *data)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.CompressedTexSubImage3DOES))
	goto FINISH;

    /* XXX: command buffer */
    dispatch.CompressedTexSubImage3DOES (target, level, 
					 xoffset, yoffset, zoffset,
					 width, height, depth, 
					 format, imageSize, data);
    egl_state = (egl_state_t *) active_state->data;
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}
GL_APICALL void GL_APIENTRY
glFramebufferTexture3DOES (GLenum target, GLenum attachment,
			   GLenum textarget, GLuint texture, 
			   GLint level, GLint zoffset)
{
    egl_state_t *egl_state;
    
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.FramebufferTexture3DOES))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    if (target != GL_FRAMEBUFFER) {
	if (egl_state->state.error == GL_NO_ERROR)
	    egl_state->state.error = GL_INVALID_ENUM;
	goto FINISH;
    }

    dispatch.FramebufferTexture3DOES (target, attachment, textarget,
				      texture, level, zoffset);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
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
    
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.BindVertexArrayOES))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    dispatch.BindVertexArrayOES (array);
    egl_state->state.need_get_error = TRUE;
    /* should we save this ? */
    egl_state->state.vertex_array_binding = array;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glDeleteVertexArraysOES (GLsizei n, const GLuint *arrays)
{
    egl_state_t *egl_state;
    int i;

    /* XXX: any error generated ? */
    if (n <= 0)
	return;
    
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.DeleteVertexArraysOES))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    dispatch.DeleteVertexArraysOES (n, arrays);
    
    /* matching vertex_array_binding ? */
    for (i = 0; i < n; i++) {
	if (arrays[i] == egl_state->state.vertex_array_binding) {
	    egl_state->state.vertex_array_binding = 0;
	    break;
	}
    }
    
FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glGenVertexArraysOES (GLsizei n, GLuint *arrays)
{
    egl_state_t *egl_state;

    /* XXX: any error generated ? */
    if (n <= 0)
	return;
    
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GenVertexArraysOES))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    dispatch.GenVertexArraysOES (n, arrays);
    
    
FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL GLboolean GL_APIENTRY
glIsVertexArrayOES (GLuint array)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.IsVertexArrayOES))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    result = dispatch.IsVertexArrayOES (array);

    if (result == GL_FALSE && egl_state->state.vertex_array_binding == array)
	egl_state->state.vertex_array_binding = 0;
    
FINISH:
    gpu_mutex_unlock (mutex);
}
#endif

#ifdef GL_AMD_performance_monitor
GL_APICALL void GL_APIENTRY
glGetPerfMonitorGroupsAMD (GLint *numGroups, GLsizei groupSize, 
			   GLuint *groups)
{
    egl_state_t *egl_state;

    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetPerfMonitorGroupsAMD))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

    dispatch.GetPerfMonitorGroupsAMD (numGroups, groupSize, groups);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glGetPerfMonitorCountersAMD (GLuint group, GLint *numCounters, 
			     GLint *maxActiveCounters, GLsizei counterSize,
			     GLuint *counters)
{
    egl_state_t *egl_state;

    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetPerfMonitorCountersAMD))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    dispatch.GetPerfMonitorCountersAMD (group, numCounters,
					maxActiveCounters,
					counterSize, counters);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glGetPerfMonitorGroupStringAMD (GLuint group, GLsizei bufSize, 
				GLsizei *length, GLchar *groupString)
{
    egl_state_t *egl_state;

    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetPerfMonitorGroupStringAMD))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    dispatch.GetPerfMonitorGroupStringAMD (group, bufSize, length,
					   groupString);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glGetPerfMonitorCounterStringAMD (GLuint group, GLuint counter, 
				  GLsizei bufSize, 
				  GLsizei *length, GLchar *counterString)
{
    egl_state_t *egl_state;

    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetPerfMonitorCounterStringAMD))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    dispatch.GetPerfMonitorCounterStringAMD (group, counter, bufSize, 
					     length, counterString);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glGetPerfMonitorCounterInfoAMD (GLuint group, GLuint counter, 
				GLenum pname, GLvoid *data)
{
    egl_state_t *egl_state;

    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetPerfMonitorCounterInfoAMD))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    dispatch.GetPerfMonitorCounterInfoAMD (group, counter, pname, data); 
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glGenPerfMonitorsAMD (GLsizei n, GLuint *monitors) 
{
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GenPerfMonitorsAMD))
	goto FINISH;

    dispatch.GenPerfMonitorsAMD (n, monitors); 

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glDeletePerfMonitorsAMD (GLsizei n, GLuint *monitors) 
{
    egl_state_t *egl_state;

    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.DeletePerfMonitorsAMD))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    dispatch.DeletePerfMonitorsAMD (n, monitors); 
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glSelectPerfMonitorCountersAMD (GLuint monitor, GLboolean enable,
				GLuint group, GLint numCounters,
				GLuint *countersList) 
{
    egl_state_t *egl_state;

    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.SelectPerfMonitorCountersAMD))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    dispatch.SelectPerfMonitorCountersAMD (monitor, enable, group,
					   numCounters, countersList); 
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glBeginPerfMonitorAMD (GLuint monitor)
{
    egl_state_t *egl_state;

    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.BeginPerfMonitorAMD))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    dispatch.BeginPerfMonitorAMD (monitor); 
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glEndPerfMonitorAMD (GLuint monitor)
{
    egl_state_t *egl_state;

    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.EndPerfMonitorAMD))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    dispatch.EndPerfMonitorAMD (monitor); 
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glGetPerfMonitorCounterDataAMD (GLuint monitor, GLenum pname,
				GLsizei dataSize, GLuint *data,
				GLint *bytesWritten)
{
    egl_state_t *egl_state;

    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetPerfMonitorCounterDataAMD))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    dispatch.GetPerfMonitorCounterDataAMD (monitor, pname, dataSize,
					   data, bytesWritten); 
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}
#endif

#ifdef GL_ANGLE_framebuffer_blit
GL_APICALL void GL_APIENTRY
glBlitFramebufferANGLE (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
			GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
			GLbitfield mask, GLenum filter)
{
    egl_state_t *egl_state;

    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.BlitFramebufferANGLE))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    dispatch.BlitFramebufferANGLE (srcX0, srcY0, srcX1, srcY1,
				   dstX0, dstY0, dstX1, dstY1,
				   mask, filter);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}
#endif

#ifdef GL_ANGLE_framebuffer_multisample
GL_APICALL void GL_APIENTRY
glRenderbufferStorageMultisampleANGLE (GLenum target, GLsizei samples, 
				       GLenum internalformat, 
				       GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.RenderbufferStorageMultisampleANGLE))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    dispatch.RenderbufferStorageMultisampleANGLE (target, samples,
						  internalformat,
						  width, height);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}
#endif

#ifdef GL_APPLE_framebuffer_multisample
GL_APICALL void GL_APIENTRY
glRenderbufferStorageMultisampleAPPLE (GLenum target, GLsizei samples, 
				       GLenum internalformat, 
				       GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.RenderbufferStorageMultisampleAPPLE))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    dispatch.RenderbufferStorageMultisampleAPPLE (target, samples,
						  internalformat,
						  width, height);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glResolveMultisampleFramebufferAPPLE (void) 
{
    egl_state_t *egl_state;

    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.ResolveMultisampleFramebufferAPPLE))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

    dispatch.ResolveMultisampleFramebufferAPPLE ();
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}
#endif

#ifdef GL_EXT_discard_framebuffer
GL_APICALL void GL_APIENTRY
glDiscardFramebufferEXT (GLenum target, GLsizei numAttachments,
			 const GLenum *attachments)
{
    egl_state_t *egl_state;
    int i;

    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.DiscardFramebufferEXT))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

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
    gpu_mutex_unlock (mutex);
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

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.RenderbufferStorageMultisampleEXT))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, no wait */
    dispatch.RenderbufferStorageMultisampleEXT (target, samples, 
						internalformat, 
						width, height);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glFramebufferTexture2DMultisampleEXT (GLenum target, GLenum attachment,
				      GLenum textarget, GLuint texture,
				      GLint level, GLsizei samples)
{
    egl_state_t *egl_state;
    
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.FramebufferTexture2DMultisampleEXT))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

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
    gpu_mutex_unlock (mutex);
}
#endif
				     
#ifdef GL_IMG_multisampled_render_to_texture
GL_APICALL void GL_APIENTRY
glRenderbufferStorageMultisampleIMG (GLenum target, GLsizei samples,
				     GLenum internalformat,
				     GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.RenderbufferStorageMultisampleIMG))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, no wait */
    dispatch.RenderbufferStorageMultisampleIMG (target, samples, 
						internalformat, 
						width, height);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glFramebufferTexture2DMultisampleIMG (GLenum target, GLenum attachment,
				      GLenum textarget, GLuint texture,
				      GLint level, GLsizei samples)
{
    egl_state_t *egl_state;
    
    /* XXX: put to a command buffer, no wait for signal */
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.FramebufferTexture2DMultisampleIMG))
	goto FINISH;

    egl_state = (egl_state_t *)active_state->data;

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
    gpu_mutex_unlock (mutex);
}
#endif

#ifdef GL_NV_fence
GL_APICALL void GL_APIENTRY
glDeleteFencesNV (GLsizei n, const GLuint *fences)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.DeleteFencesNV))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, no wait */
    dispatch.DeleteFencesNV (n, fences); 
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glGenFencesNV (GLsizei n, GLuint *fences)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GenFencesNV))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, no wait */
    dispatch.GenFencesNV (n, fences); 
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL GLboolean GL_APIENTRY
glIsFenceNV (GLuint fence)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.IsFenceNV))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, no wait */
    result = dispatch.IsFenceNV (fence); 
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
    return result;
}

GL_APICALL GLboolean GL_APIENTRY
glTestFenceNV (GLuint fence)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.TestFenceNV))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, no wait */
    result = dispatch.TestFenceNV (fence); 
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
    return result;
}

GL_APICALL GLboolean GL_APIENTRY
glGetFenceivNV (GLuint fence, GLenum pname, int *params)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetFenceivNV))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, no wait */
    dispatch.GetFenceivNV (fence, pname, params); 
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL GLboolean GL_APIENTRY
glFinishFenceivNV (GLuint fence)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.FinishFenceNV))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, no wait */
    dispatch.FinishFenceNV (fence); 
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL GLboolean GL_APIENTRY
glSetFenceNV (GLuint fence, GLenum condition)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.SetFenceNV))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;
    
    /* XXX: create command buffer, no wait */
    dispatch.SetFenceNV (fence, condition); 
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}
#endif

#ifdef GL_NV_coverage_sample
GL_APICALL void GL_APIENTRY
glCoverageMaskNV (GLboolean mask)
{
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.CoverageMaskNV))
	goto FINISH;

    /* XXX: create command buffer, no wait */
    dispatch.CoverageMaskNV (mask); 

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glCoverageOperationNV (GLenum operation)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.CoverageOperationNV))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

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
    gpu_mutex_unlock (mutex);
}
#endif

#ifdef GL_QCOM_driver_control
GL_APICALL void GL_APIENTRY
glGetDriverControlsQCOM (GLint *num, GLsizei size, GLuint *driverControls)
{
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetDriverControlsQCOM))
	goto FINISH;

    /* XXX: create command buffer, no wait */
    dispatch.GetDriverControlsQCOM (num, size, driverControls); 

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glGetDriverControlStringQCOM (GLuint driverControl, GLsizei bufSize,
			      GLsizei *length, GLchar *driverControlString)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.GetDriverControlStringQCOM))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

    /* XXX: create command buffer, no wait */
    dispatch.GetDriverControlStringQCOM (driverControl, bufSize,
					 length, driverControlString); 
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glEnableDriverControlQCOM (GLuint driverControl)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.EnableDriverControlQCOM))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

    /* XXX: create command buffer, no wait */
    dispatch.EnableDriverControlQCOM (driverControl);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glDisableDriverControlQCOM (GLuint driverControl)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.DisableDriverControlQCOM))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

    /* XXX: create command buffer, no wait */
    dispatch.DisableDriverControlQCOM (driverControl);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}
#endif

#ifdef GL_QCOM_extended_get
GL_APICALL void GL_APIENTRY
glExtGetTexturesQCOM (GLuint *textures, GLint maxTextures, 
		      GLint *numTextures)
{
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.ExtGetTexturesQCOM))
	goto FINISH;

    /* XXX: create command buffer, no wait */
    dispatch.ExtGetTexturesQCOM (textures, maxTextures, numTextures);

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glExtGetBuffersQCOM (GLuint *buffers, GLint maxBuffers, GLint *numBuffers) 
{
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.ExtGetBuffersQCOM))
	goto FINISH;

    /* XXX: create command buffer, no wait */
    dispatch.ExtGetBuffersQCOM (buffers, maxBuffers, numBuffers);

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glExtGetRenderbuffersQCOM (GLuint *renderbuffers, GLint maxRenderbuffers, 
			   GLint *numRenderbuffers) 
{
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.ExtGetRenderbuffersQCOM))
	goto FINISH;

    /* XXX: create command buffer, no wait */
    dispatch.ExtGetRenderbuffersQCOM (renderbuffers, maxRenderbuffers, 
				      numRenderbuffers);

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glExtGetFramebuffersQCOM (GLuint *framebuffers, GLint maxFramebuffers, 
			  GLint *numFramebuffers) 
{
    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.ExtGetFramebuffersQCOM))
	goto FINISH;

    /* XXX: create command buffer, no wait */
    dispatch.ExtGetFramebuffersQCOM (framebuffers, maxFramebuffers, 
				     numFramebuffers);

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glExtGetTexLevelParameterivQCOM (GLuint texture, GLenum face, GLint level,
				 GLenum pname, GLint *params)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.ExtGetTexLevelParameterivQCOM))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

    /* XXX: create command buffer, no wait */
    dispatch.ExtGetTexLevelParameterivQCOM (texture, face, level,
					    pname, params);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glExtTexObjectStateOverrideiQCOM (GLenum target, GLenum pname, GLint param)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.ExtTexObjectStateOverrideiQCOM))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

    /* XXX: create command buffer, no wait */
    dispatch.ExtTexObjectStateOverrideiQCOM (target, pname, param);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glExtGetTexSubImageQCOM (GLenum target, GLint level, 
			 GLint xoffset, GLint yoffset, GLint zoffset,
			 GLsizei width, GLsizei height, GLsizei depth,
			 GLenum format, GLenum type, GLvoid *texels)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.ExtGetTexSubImageQCOM))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

    /* XXX: create command buffer, no wait */
    dispatch.ExtGetTexSubImageQCOM (target, level, 
				    xoffset, yoffset, zoffset,
				    width, height, depth,
				    format, type, texels);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glExtGetBufferPointervQCOM (GLenum target, GLvoid **params) 
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.ExtGetBufferPointervQCOM))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

    /* XXX: create command buffer, no wait */
    dispatch.ExtGetBufferPointervQCOM (target, params); 
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}
#endif

#ifdef GL_QCOM_extended_get2
GL_APICALL void GL_APIENTRY
glExtGetShadersQCOM (GLuint *shaders, GLint maxShaders, GLint *numShaders)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.ExtGetShadersQCOM))
	goto FINISH;

    /* XXX: create command buffer, no wait */
    dispatch.ExtGetShadersQCOM (shaders, maxShaders, numShaders); 
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glExtGetProgramsQCOM (GLuint *programs, GLint maxPrograms, 
		      GLint *numPrograms)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.ExtGetProgramsQCOM))
	goto FINISH;

    /* XXX: create command buffer, no wait */
    dispatch.ExtGetProgramsQCOM (programs, maxPrograms, numPrograms); 
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL GLboolean GL_APIENTRY
glExtIsProgramBinaryQCOM (GLuint program) 
{
    egl_state_t *egl_state;
    v_bool_t result = FALSE;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.ExtIsProgramBinaryQCOM))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

    /* XXX: create command buffer, no wait */
    result = dispatch.ExtIsProgramBinaryQCOM (program); 
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
    return result;
}

GL_APICALL void GL_APIENTRY
glExtGetProgramBinarySourceQCOM (GLuint program, GLenum shadertype,
				 GLchar *source, GLint *length) 
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.ExtGetProgramBinarySourceQCOM))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

    /* XXX: create command buffer, no wait */
    dispatch.ExtGetProgramBinarySourceQCOM (program, shadertype, 
					    source, length); 
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}
#endif

#ifdef GL_QCOM_tiled_rendering
GL_APICALL void GL_APIENTRY
glStartTilingQCOM (GLuint x, GLuint y, GLuint width, GLuint height, 
		   GLbitfield preserveMask)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.StartTilingQCOM))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

    /* XXX: create command buffer, no wait */
    dispatch.StartTilingQCOM (x, y, width, height, preserveMask);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}

GL_APICALL void GL_APIENTRY
glEndTilingQCOM (GLbitfield preserveMask)
{
    egl_state_t *egl_state;

    gpu_mutex_lock (mutex);

    if(! _is_error_state_or_func (active_state, 
				  dispatch.EndTilingQCOM))
	goto FINISH;

    egl_state = (egl_state_t *) active_state->data;

    /* XXX: create command buffer, no wait */
    dispatch.EndTilingQCOM (preserveMask);
    egl_state->state.need_get_error = TRUE;

FINISH:
    gpu_mutex_unlock (mutex);
}
#endif
