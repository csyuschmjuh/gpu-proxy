#include "gpuprocess_gles2_srv_private.h"
#include <stdlib.h>
#include <string.h>

/* global state variable */
gl_srv_states_t	srv_states;

static void
_gpuprocess_srv_copy_egl_state (egl_state_t *dst, egl_state_t *src)
{
    memcpy (dst, src, sizeof (egl_state_t));
}

void 
_gpuprocess_srv_init ()
{
    srv_states.num_contexts = 0;

    srv_states.active_state = NULL;
    
    srv_states.pending_context = EGL_NO_CONTEXT;
    srv_states.pending_display = EGL_NO_DISPLAY;
    srv_states.pending_drawable = EGL_NO_SURFACE;
    srv_states.pending_readable = EGL_NO_SURFACE;
    srv_states.make_current_called = FALSE;

    srv_states.states = NULL;
}

/* called within eglTerminate */
void 
_gpuprocess_srv_terminate (EGLDisplay display)
{
   v_link_list_t *head = srv_states.states;
   v_link_list_t *list = head;
   v_link_list_t *current;

   egl_state_t *egl_state;

    if (srv_states.num_contexts == 0 || ! srv_states.states)
	return;

    
    while (list != NULL) {
	egl_state = (egl_state_t *) list->data;
	current = list;
	list = list->next;

	if (egl_state->display == display) {
	    if (srv_states.active_state == egl_state)
		srv_states.active_state = NULL;

	    free (egl_state);

	    if (current->prev)
		current->prev->next = current->next;
	    if (current->next)
		current->next->prev = current->prev;
	    if (head == current)
		head = current->next;
	    free (current);
  	}
    }
}

static void
_gpuprocess_srv_init_gles2_states (egl_state_t *egl_state)
{
    int i, j;
    gles2_state_t *state = &egl_state->state;

    egl_state->context = EGL_NO_CONTEXT;
    egl_state->display = EGL_NO_DISPLAY;
    egl_state->drawable = EGL_NO_SURFACE;
    egl_state->readable = EGL_NO_SURFACE;

    state->error = GL_NO_ERROR;
   
    state->active_texture = GL_TEXTURE0;
    state->array_buffer_binding = 0;

    state->blend = GL_FALSE;
    memset (state->blend_color, 0, sizeof (GLfloat) * 4);
    state->blend_dst_alpha = GL_ZERO;
    state->blend_dst_rgb = GL_ZERO;
    state->blend_src_alpha = GL_ONE;
    state->blend_src_rgb = GL_ONE;
    state->blend_equation[0] = state->blend_equation[1] = GL_FUNC_ADD;

    memset (state->color_clear_value, 0, sizeof (GLfloat) * 4);
    for (i = 0; i < 4; i++)
	state->color_writemask[i] = GL_TRUE;    

    state->cull_face = GL_FALSE;
    state->cull_face_mode = GL_BACK;

    state->current_program = 0;

    state->depth_clear_value = 1;
    state->depth_func = GL_LESS;
    state->depth_range[0] = 0; state->depth_range[1] = 1;
    state->depth_test = GL_FALSE;
    state->depth_writemask = GL_TRUE;

    state->dither = GL_TRUE;

    state->element_array_buffer_binding = 0;
    state->framebuffer_binding = 0;
    state->renderbuffer_binding = 0;
    
    state->front_face = GL_CCW;
   
    state->generate_mipmap_hint = GL_DONT_CARE;

    state->line_width = 1;
    
    state->pack_alignment = 4; state->unpack_alignment = 4;

    state->polygon_offset_factor = 0;
    state->polygon_offset_fill = GL_FALSE;
    state->polygon_offset_units = 0;

    state->sample_alpha_to_coverage = 0;
    state->sample_coverage = GL_FALSE;
    
    memcpy (state->scissor_box, 0, sizeof (GLint) * 4);
    state->scissor_test = GL_FALSE;

    state->stencil_back_fail = GL_KEEP;
    state->stencil_back_func = GL_ALWAYS;
    state->stencil_back_pass_depth_fail = GL_KEEP;
    state->stencil_back_pass_depth_pass = GL_KEEP;
    state->stencil_back_ref = 0;
    memset (&state->stencil_back_value_mask, 0xff, sizeof (GLint));
    state->stencil_clear_value = 0;
    state->stencil_fail = GL_KEEP;
    state->stencil_func = GL_ALWAYS;
    state->stencil_pass_depth_fail = GL_KEEP;
    state->stencil_pass_depth_pass = GL_KEEP;
    state->stencil_ref = 0;
    state->stencil_test = GL_FALSE;
    memset (&state->stencil_value_mask, 0xff, sizeof (GLint));
    memset (&state->stencil_writemask, 0xff, sizeof (GLint));

    memset (state->texture_binding, 0, sizeof (GLint) * 2);

    memset (state->viewport, 0, sizeof (GLint) * 4);

    for (i = 0; i < 32; i++) {
	for (j = 0; i < 2; j++) {
	    state->texture_mag_filter[i][j] = GL_LINEAR;
	    state->texture_mag_filter[i][j] = GL_NEAREST_MIPMAP_LINEAR;
	    state->texture_wrap_s[i][j] = GL_REPEAT;
	    state->texture_wrap_t[i][j] = GL_REPEAT;
	}
    }

    state->buffer_size[0] = state->buffer_size[1] = 0;
    state->buffer_usage[0] = state->buffer_usage[1] = GL_STATIC_DRAW;
}

static void
_gpuprocess_srv_set_egl_states (egl_state_t *egl_state, 
				EGLDisplay  display,
				EGLSurface  drawable,
				EGLSurface  readable,
				EGLContext  context)
{
    egl_state->context = context;
    egl_state->display = display;
    egl_state->drawable = drawable;
    egl_state->readable = readable;
}

int
_gpuprocess_srv_is_equal (egl_state_t *state,
			  EGLDisplay  display,
			  EGLSurface  drawable,
			  EGLSurface  readable,
			  EGLContext  context)
{
   if (state->display == display && state->drawable == drawable &&
       state->readable == readable && state->context == context);
}

/* we should call real eglMakeCurrent() before, and wait for result
 * if eglMakeCurrent() returns EGL_TRUE, then we call this
 */
void
_gpuprocess_srv_make_current (EGLDisplay display,
			      EGLSurface drawable,
			      EGLSurface readable,
			      EGLContext context)
{
    egl_state_t *new_state;
    v_link_list_t *list = srv_states.states;
    v_link_list_t *new_list;

    srv_states.make_current_called = FALSE;

    if (srv_states.num_contexts == 0 || ! srv_states.states) {
	srv_states.num_contexts = 1;
	srv_states.states = (v_link_list_t *)malloc (sizeof (v_link_list_t));
	srv_states.states->prev = NULL;
	srv_states.states->next = NULL;

	new_state = (egl_state_t *)malloc (sizeof (egl_state_t));

	_gpuprocess_srv_set_egl_states (new_state, display, drawable, 
					readable, context);
	_gpuprocess_srv_init_gles2_states (new_state);
	srv_states.active_state = new_state;
	srv_states.states->data = new_state;
    }

    /* look for matching context in embedded_states */
    while (list) {
	egl_state_t *state = (egl_state_t *)list->data;
	
	if (state->context == context &&
	    state->display == display) {	
	    _gpuprocess_srv_set_egl_states (state,
					    display, drawable, 
					    readable, context);
	    srv_states.active_state = state;
	    return;
	}
    }

    /* we have not found a context match */
    new_state = (egl_state_t *) malloc (sizeof (egl_state_t));

    _gpuprocess_srv_set_egl_states (new_state, display, 
				    drawable, readable, context);
    _gpuprocess_srv_init_gles2_states (new_state);
    srv_states.active_state = new_state;
    srv_states.num_contexts ++;

    list = srv_states.states;
    while (list->next != NULL)
	list = list->next;

    new_list = (v_link_list_t *)malloc (sizeof (v_link_list_t));
    new_list->prev = list;
    new_list->next = NULL;
    new_list->data = new_state;
}

/* called by eglDestroyContext() */
static int
_gpuprocess_srv_has_context (egl_state_t *state,
			     EGLDisplay  display,
			     EGLContext  context)
{
    return (state->context == context && state->display == display);
}

/* called by eglDestroyContext() - once we know there is matching context
 * we call real eglDestroyContext(), and if returns EGL_TRUE, we call 
 * this function 
 */
void
_gpuprocess_srv_destroy_context (EGLDisplay display, EGLContext context)
{
    egl_state_t *state;
    v_link_list_t *list = srv_states.states;
    v_link_list_t *current;

    if (srv_states.num_contexts == 0 || ! srv_states.states)
	return;

    while (list != NULL) {
	current = list;
	list = list->next;
	state = (egl_state_t *)current->data;
    
	if (_gpuprocess_has_context (state, display, context)) {
	    if (srv_states.active_state == state)
		srv_states.active_state = NULL;
	    if (srv_states.states == current)
		srv_states.states = current->next;
	
	    if (current->prev)
		current->prev->next = current->next;
	    if (current->next)
		current->next->prev = current->prev;

	    free (current);
	    free (state);
	    srv_states.num_contexts --;
	    break;
	}
    }
}
