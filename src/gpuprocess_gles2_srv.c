#include "gpuprocess_gles2_srv_private.h"
#include "gpuprocess_dispatch_private.h"
#include <stdlib.h>
#include <string.h>

/* global state variable */
gl_srv_states_t	srv_states;
extern gpuprocess_dispatch_t dispatch;

static void
_gpuprocess_srv_copy_egl_state (egl_state_t *dst, egl_state_t *src)
{
    memcpy (dst, src, sizeof (egl_state_t));
}

void 
_gpuprocess_srv_init ()
{
    srv_states.num_contexts = 0;

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
	    egl_state->destroy_dpy = TRUE;
	    if (egl_state->active == FALSE) { 
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

    egl_state->ref_count = 0;

    egl_state->active = FALSE;

    egl_state->destroy_dpy = FALSE;
    egl_state->destroy_ctx = FALSE;
    egl_state->destroy_draw = FALSE;
    egl_state->destroy_read = FALSE;

    state->max_combined_texture_image_units = 8;
    state->max_vertex_attribs_queried = FALSE;
    state->max_vertex_attribs = 8;
    state->max_cube_map_texture_size = 16;
    state->max_fragment_uniform_vectors = 16;
    state->max_renderbuffer_size = 1;
    state->max_texture_image_units = 8;
    state->max_texture_size = 64;
    state->max_varying_vectors = 8;
    state->max_vertex_uniform_vectors = 128;
    state->max_vertex_texture_image_units = 0;


    state->error = GL_NO_ERROR;
    state->need_get_error = FALSE;
    state->programs = NULL;
   
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

    /* XXX: should we set this */
    state->shader_compiler = GL_TRUE; 

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
    memset (&state->stencil_back_writemask, 0xff, sizeof (GLint));

    memset (state->texture_binding, 0, sizeof (GLint) * 2);

    memset (state->viewport, 0, sizeof (GLint) * 4);

    for (i = 0; i < 32; i++) {
	for (j = 0; i < 3; j++) {
	    state->texture_mag_filter[i][j] = GL_LINEAR;
	    state->texture_mag_filter[i][j] = GL_NEAREST_MIPMAP_LINEAR;
	    state->texture_wrap_s[i][j] = GL_REPEAT;
	    state->texture_wrap_t[i][j] = GL_REPEAT;
	}
#ifdef GL_OES_texture_3D
	state->texture_3d_wrap_r[i] = GL_REPEAT;
#endif
    }


    state->buffer_size[0] = state->buffer_size[1] = 0;
    state->buffer_usage[0] = state->buffer_usage[1] = GL_STATIC_DRAW;

    /* XXX: initialize a thread */
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
v_bool_t 
_gpuprocess_srv_make_current (EGLDisplay display,
			      EGLSurface drawable,
			      EGLSurface readable,
			      EGLContext context,
			      EGLDisplay prev_dpy,
			      EGLSurface prev_draw,
			      EGLSurface prev_read,
			      EGLSurface prev_ctx, 
			      v_link_list_t **active_state)
{
    egl_state_t *new_state;
    v_link_list_t *list = srv_states.states;
    v_link_list_t *new_list;
    v_link_list_t *match_state = NULL;


    if (context == EGL_NO_CONTEXT || display == EGL_NO_DISPLAY) {
    /* look for matching context in existing states */
	while (list) {
	    egl_state_t *state = (egl_state_t *)list->data;

	    if (state->context == prev_ctx &&
		state->display == prev_dpy) {
		if (state->ref_count > 0)
		    state->ref_count --;
		if (state->ref_count == 0)
		    state->active = FALSE;
	    }
	}
	_gpuprocess_terminate (prev_dpy);
	_gpuprocess_destroy_context (prev_dpy, prev_ctx);
	_gpuprocess_destroy_surface (prev_dpy, prev_draw);
	_gpuprocess_destroy_surface (prev_dpy, prev_read);
	
	*active_state = NULL;

	/* XXX: tell affected thread to terminate */
	return TRUE;
    }

    if (srv_states.num_contexts == 0 || ! srv_states.states) {
	srv_states.num_contexts = 1;
	srv_states.states = (v_link_list_t *)malloc (sizeof (v_link_list_t));
	srv_states.states->prev = NULL;
	srv_states.states->next = NULL;

	new_state = (egl_state_t *)malloc (sizeof (egl_state_t));

	_gpuprocess_srv_set_egl_states (new_state, display, drawable, 
					readable, context);
	_gpuprocess_srv_init_gles2_states (new_state);
	srv_states.states->data = new_state;
	new_state->active = TRUE;
	new_state->ref_count = 1;
	*active_state = srv_states.states;
	/* XXX: create a thread */
	return TRUE; 	/* create new context, need to call eglMakeCurrent */
    }

    /* look for matching context in existing states */
    while (list) {
	egl_state_t *state = (egl_state_t *)list->data;

	if (state->context == prev_ctx &&
	    state->display == prev_dpy) {
	    if (state->ref_count > 0)
		state->ref_count --;
	    if (state->ref_count == 0)
		state->active = FALSE;
	}
	
	if (state->context == context &&
	    state->display == display) {	
	    _gpuprocess_srv_set_egl_states (state,
					    display, drawable, 
					    readable, context);
	    state->active = TRUE;
	    state->ref_count ++;
	    match_state = list;
	    break;
	}
    }

    /* destroy pending eglTerminate (display) */
    if (match_state) {
	_gpuprocess_terminate (prev_dpy);
	_gpuprocess_destroy_context (prev_dpy, prev_ctx);
	_gpuprocess_destroy_surface (prev_dpy, prev_draw);
	_gpuprocess_destroy_surface (prev_dpy, prev_read);

	*active_state = match_state;

	/* XXX: terminate affected threads */
	return TRUE;
    }
    else {
	/* we have not found a context match */
	new_state = (egl_state_t *) malloc (sizeof (egl_state_t));

	_gpuprocess_srv_set_egl_states (new_state, display, 
				    drawable, readable, context);
	_gpuprocess_srv_init_gles2_states (new_state);
	srv_states.num_contexts ++;

	list = srv_states.states;
	while (list->next != NULL)
	    list = list->next;

	new_list = (v_link_list_t *)malloc (sizeof (v_link_list_t));
	new_list->prev = list;
	new_list->next = NULL;
	new_list->data = new_state;
	new_state->active = TRUE;
	list->next = new_list;
	*active_state = new_list;

	/* XXX: start a new thread */
	return TRUE;
    }
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
_gpuprocess_srv_destroy_context (EGLDisplay display, 
				 EGLContext context)
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
	    state->destroy_ctx = TRUE;
	    if (state->active == FALSE) {
		if (srv_states.states == current)
		    srv_states.states = current->next;
	
		if (current->prev)
		    current->prev->next = current->next;
		if (current->next)
		    current->next->prev = current->prev;

		free (current);
		free (state);
		srv_states.num_contexts --;
		/* XXX: terminate affected thread */
	    }
	}
    }
}

void
_gpuprocess_srv_destroy_surface (EGLDisplay display, 
				 EGLSurface surface)
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

	if (state->display == display) {
	    if (state->readable == surface)
		state->destroy_read = TRUE;
	    else if (state->drawable == surface)
		state->destroy_draw = TRUE;

	    if (state->active == FALSE) {
		if (state->readable == surface && 
		    state->destroy_read == TRUE)
		    state->readable = EGL_NO_SURFACE;
		if (state->drawable == surface && 
		    state->destroy_draw == TRUE)
		    state->drawable = EGL_NO_SURFACE;
	    }
	}
    }
}

void
_gpuprocess_srv_remove_context (EGLDisplay display, 
				EGLSurface draw,
				EGLSurface read,
				EGLContext context)
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

	if (state->display == display && state->context == context &&
	    state->drawable == draw && state->readable == read) {
	    if (current->prev)
		current->prev->next = current->next;
	    if (current->next)
		current->next->prev = current->prev;

	    free (current);
	    free (state);
	    srv_states.num_contexts --;
	    /* XXX: terminate affected thread */
	}
    }
}
/* we should call this on srv thread */
//EGLBoolean 
//_egl_make_pending_current () 
//{
//    EGLBoolean make_current_result = GL_FALSE;

    /* check pending make current call */
//    if (srv_states.make_current_called && dispatch.eglMakeCurrent) {
	/* we need to make current call */
//	make_current_result = 
//	    dispatch.eglMakeCurrent (srv_states.pending_display,
//				     srv_states.pending_drawable,
//				     srv_states.pending_readable,
//				     srv_states.pending_context);
//	    srv_states.make_current_called = FALSE;
 
	/* we make current on pending eglMakeCurrent() and it succeeds */
//	if (make_current_result == EGL_TRUE) {
//	    _gpuprocess_srv_make_current (srv_states.pending_display,
//					  srv_states.pending_drawable,
//					  srv_states.pending_readable,
//					  srv_states.pending_context);
//	}
//    }
   
//    return make_current_result;
//}
