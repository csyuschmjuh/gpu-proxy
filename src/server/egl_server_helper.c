/* This file is a helper file for some of the private functions that
 * manipulate the server_states variable 
 */
#include "dispatch_private.h"
#include "command_buffer_server.h"
#include "egl_server_private.h"
#include "thread_private.h"
#include <stdlib.h>
#include <string.h>

/* global state variable for all server threads */
gl_server_states_t              server_states;

mutex_static_init (egl_mutex);

static void
_server_copy_egl_state (egl_state_t *dst, egl_state_t *src)
{
    memcpy (dst, src, sizeof (egl_state_t));
}

static bool
_server_has_context (egl_state_t *state,
                             EGLDisplay  display,
                             EGLContext  context)
{
    return (state->context == context && state->display == display);
}

static void
_server_init_gles2_states (egl_state_t *egl_state)
{
    int i, j;
    gles2_state_t *state = &egl_state->state;

    egl_state->context = EGL_NO_CONTEXT;
    egl_state->display = EGL_NO_DISPLAY;
    egl_state->drawable = EGL_NO_SURFACE;
    egl_state->readable = EGL_NO_SURFACE;

    egl_state->active = false;

    egl_state->destroy_dpy = false;
    egl_state->destroy_ctx = false;
    egl_state->destroy_draw = false;
    egl_state->destroy_read = false;

    state->vertex_attribs.count = 0;
    state->vertex_attribs.attribs = state->vertex_attribs.embedded_attribs;

    state->max_combined_texture_image_units = 8;
    state->max_vertex_attribs_queried = false;
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
    state->need_get_error = false;
    state->programs = NULL;
   
    state->active_texture = GL_TEXTURE0;
    state->array_buffer_binding = 0;

    state->blend = GL_FALSE;

    for (i = 0; i < 4; i++) {
        state->blend_color[i] = GL_ZERO;
        state->blend_dst[i] = GL_ZERO;
        state->blend_src[i] = GL_ONE;
    }
    
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
    
    state->pack_alignment = 4; 
    state->unpack_alignment = 4;

    state->polygon_offset_factor = 0;
    state->polygon_offset_fill = GL_FALSE;
    state->polygon_offset_units = 0;

    state->sample_alpha_to_coverage = 0;
    state->sample_coverage = GL_FALSE;
    
    memset (state->scissor_box, 0, sizeof (GLint) * 4);
    state->scissor_test = GL_FALSE;

    /* XXX: should we set this */
    state->shader_compiler = GL_TRUE; 

    state->stencil_back_fail = GL_KEEP;
    state->stencil_back_func = GL_ALWAYS;
    state->stencil_back_pass_depth_fail = GL_KEEP;
    state->stencil_back_pass_depth_pass = GL_KEEP;
    state->stencil_back_ref = 0;
    memset (&state->stencil_back_value_mask, 1, sizeof (GLint));
    state->stencil_clear_value = 0;
    state->stencil_fail = GL_KEEP;
    state->stencil_func = GL_ALWAYS;
    state->stencil_pass_depth_fail = GL_KEEP;
    state->stencil_pass_depth_pass = GL_KEEP;
    state->stencil_ref = 0;
    state->stencil_test = GL_FALSE;
    memset (&state->stencil_value_mask, 1, sizeof (GLint));
    memset (&state->stencil_writemask, 1, sizeof (GLint));
    memset (&state->stencil_back_writemask, 1, sizeof (GLint));

    memset (state->texture_binding, 0, sizeof (GLint) * 2);

    memset (state->viewport, 0, sizeof (GLint) * 4);

    for (i = 0; i < 32; i++) {
        for (j = 0; j < 3; j++) {
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
_server_set_egl_states (egl_state_t *egl_state, 
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

void 
_server_remove_state (link_list_t **state)
{
    egl_state_t *egl_state = (egl_state_t *) (*state)->data;

    if (egl_state->state.vertex_attribs.attribs != 
        egl_state->state.vertex_attribs.embedded_attribs)
        free (egl_state->state.vertex_attribs.attribs);

    if (*state == server_states.states) {
        server_states.states = server_states.states->next;
        if (server_states.states != NULL)
            server_states.states->prev = NULL;
    }

    if ((*state)->prev)
        (*state)->prev->next = (*state)->next;
    if ((*state)->next)
        (*state)->next->prev = (*state)->prev;

    free (egl_state);
    free (*state);
    *state = NULL;

    server_states.num_contexts --;
}

static void 
_server_remove_surface (link_list_t *state, bool read)
{
    egl_state_t *egl_state = (egl_state_t *) state->data;

    if (read == true)
        egl_state->readable = EGL_NO_SURFACE;
    else
        egl_state->drawable = EGL_NO_SURFACE;

}

static link_list_t *
_server_get_state (EGLDisplay dpy,
                              EGLSurface draw,
                              EGLSurface read,
                              EGLContext ctx)
{
    link_list_t *new_list;
    egl_state_t   *new_state;
    link_list_t *list = server_states.states;

    if (server_states.num_contexts == 0 || ! server_states.states) {
        server_states.num_contexts = 1;
        server_states.states = (link_list_t *)malloc (sizeof (link_list_t));
        server_states.states->prev = NULL;
        server_states.states->next = NULL;

        new_state = (egl_state_t *)malloc (sizeof (egl_state_t));

        _server_init_gles2_states (new_state);
        _server_set_egl_states (new_state, dpy, draw, read, ctx); 
        server_states.states->data = new_state;
        new_state->active = true;
	return server_states.states;
    }

    /* look for matching context in existing states */
    while (list) {
        egl_state_t *state = (egl_state_t *)list->data;

        if (state->context == ctx &&
            state->display == dpy) {
            _server_set_egl_states (state, dpy, draw, read, ctx);
            state->active = true;
            return list;
        }
        list = list->next;
    }

    /* we have not found a context match */
    new_state = (egl_state_t *) malloc (sizeof (egl_state_t));

    _server_init_gles2_states (new_state);
    _server_set_egl_states (new_state, dpy, draw, read, ctx); 
    server_states.num_contexts ++;

    list = server_states.states;
    while (list->next != NULL)
        list = list->next;

    new_list = (link_list_t *)malloc (sizeof (link_list_t));
    new_list->prev = list;
    new_list->next = NULL;
    new_list->data = new_state;
    new_state->active = true;
    list->next = new_list;

    return new_list;
}

void 
_server_init (command_buffer_server_t *server)
{
    mutex_lock (egl_mutex);
    if (server_states.initialized == false) {
        server_states.num_contexts = 0;
        server_states.states = NULL;

        dispatch_init (&server->dispatch);
        server_states.initialized = true;
    }
    mutex_unlock (egl_mutex);
}

/* the server first calls eglTerminate (display),
 * then look over the cached states
 */
void 
_server_terminate (EGLDisplay display, link_list_t *active_state) 
{
    link_list_t *head = server_states.states;
    link_list_t *list = head;
    link_list_t *current;

    egl_state_t *egl_state;

    mutex_lock (egl_mutex);

    if (server_states.initialized == false ||
        server_states.num_contexts == 0 || (! server_states.states)) {
        mutex_unlock (egl_mutex);
        return;
    }
    
    while (list != NULL) {
        egl_state = (egl_state_t *) list->data;
        current = list;
        list = list->next;

        if (egl_state->display == display) {
            if (! egl_state->active)
                _server_remove_state (&current);
                /* XXX: Do we need to stop the thread? */
        }
    }

    /* is current active state affected ?*/
    if (active_state) { 
        egl_state = (egl_state_t *) active_state->data;
        if (egl_state->display == display)
	    egl_state->destroy_dpy = true;
    }
    /* XXX: should we stop current server thread ? */
    /*
    else if (! active_state) {
    } */
    mutex_unlock (egl_mutex);
}


bool
_server_is_equal (egl_state_t *state,
                             EGLDisplay  display,
                             EGLSurface  drawable,
                             EGLSurface  readable,
                             EGLContext  context)
{
   return (state->display == display && state->drawable == drawable &&
       state->readable == readable && state->context == context);
}

/* we should call real eglMakeCurrent() before, and wait for result
 * if eglMakeCurrent() returns EGL_TRUE, then we call this
 */
void 
_server_make_current (EGLDisplay display,
                                 EGLSurface drawable,
                                 EGLSurface readable,
                                 EGLContext context,
                                 link_list_t *active_state,
                                 link_list_t **active_state_out)
{
    egl_state_t *new_state, *egl_state;
    link_list_t *list = server_states.states;
    link_list_t *new_list;
    link_list_t *match_state = NULL;

    mutex_lock (egl_mutex);
    /* we are switching to None context */
    if (context == EGL_NO_CONTEXT || display == EGL_NO_DISPLAY) {
        /* current is None too */
        if (! active_state) {
            *active_state_out = NULL;
            mutex_unlock (egl_mutex);
            return;
        }
        
        egl_state = (egl_state_t *) active_state->data;
        egl_state->active = false;
        
        if (egl_state->destroy_dpy || egl_state->destroy_ctx)
            _server_remove_state (&active_state);
        
        if (active_state) {
            if (egl_state->destroy_read)
                _server_remove_surface (active_state, true);

            if (egl_state->destroy_draw)
                _server_remove_surface (active_state, false);
        }

        *active_state_out = NULL;
        mutex_unlock (egl_mutex);
        return;
    }

    /* we are switch to one of the valid context */
    if (active_state) {
        egl_state = (egl_state_t *) active_state->data;
        egl_state->active = false;
        
        /* XXX did eglTerminate()/eglDestroyContext()/eglDestroySurface()
         * called affect us?
         */
        if (egl_state->destroy_dpy || egl_state->destroy_ctx)
            _server_remove_state (&active_state);
        
        if (active_state) {
            if (egl_state->destroy_read)
                _server_remove_surface (active_state, true);

            if (egl_state->destroy_draw)
                _server_remove_surface (active_state, false);
        }
    }

    /* get existing state or create a new one */
    *active_state_out = _server_get_state (display, drawable,
                                                   readable, context);
    mutex_unlock (egl_mutex);
}

/* called by eglDestroyContext() - once we know there is matching context
 * we call real eglDestroyContext(), and if returns EGL_TRUE, we call 
 * this function 
 */
void
_server_destroy_context (EGLDisplay display, 
                                    EGLContext context,
                                    link_list_t *active_state)
{
    egl_state_t *state;
    link_list_t *list = server_states.states;
    link_list_t *current;

    mutex_lock (egl_mutex);
    if (server_states.num_contexts == 0 || ! server_states.states) {
        mutex_unlock (egl_mutex);
        return;
    }

    while (list != NULL) {
        current = list;
        list = list->next;
        state = (egl_state_t *)current->data;
    
        if (_server_has_context (state, display, context)) {
            state->destroy_ctx = true;
            if (state->active == false) 
                _server_remove_state (&current);
        }
    }
    mutex_unlock (egl_mutex);
}

void
_server_destroy_surface (EGLDisplay display, 
                                    EGLSurface surface,
                                    link_list_t *active_state)
{
    egl_state_t *state;
    link_list_t *list = server_states.states;
    link_list_t *current;

    mutex_lock (egl_mutex);
    if (server_states.num_contexts == 0 || ! server_states.states) {
        mutex_unlock (egl_mutex);
        return;
    }

    while (list != NULL) {
        current = list;
        list = list->next;
        state = (egl_state_t *)current->data;

        if (state->display == display) {
            if (state->readable == surface)
                state->destroy_read = true;
            if (state->drawable == surface)
                state->destroy_draw = true;

            if (state->active == false) {
                if (state->readable == surface && 
                    state->destroy_read == true)
                    state->readable = EGL_NO_SURFACE;
                if (state->drawable == surface && 
                    state->destroy_draw == true)
                    state->drawable = EGL_NO_SURFACE;
            }
        }
    }

    mutex_unlock (egl_mutex);
}

bool
_match (EGLDisplay display,
                   EGLSurface drawable,
                   EGLSurface readable,
                   EGLContext context, 
                   link_list_t **state)
{
    egl_state_t *egl_state;
    link_list_t *list = server_states.states;
    link_list_t *current;

    mutex_lock (egl_mutex);
    if (server_states.num_contexts == 0 || ! server_states.states) {
        mutex_unlock (egl_mutex);
        return false;
    }

    while (list != NULL) {
        current = list;
        list = list->next;
        egl_state = (egl_state_t *)current->data;

        /* we check matching of display, context, readable and drawable,
         * and also whether destroy_dpy is true or not
         * if destroy_dpy has been set, we should not return as it
         * as a valid state
         */
        if (egl_state->display == display && 
            egl_state->context == context &&
            egl_state->drawable == drawable &&
            egl_state->readable == readable &&
            egl_state->active == false && 
            ! egl_state->destroy_dpy ) {
            egl_state->active = true;
            *state = current;
            mutex_unlock (egl_mutex);
            return true;
        }

    }

    mutex_unlock (egl_mutex);
    return false;
}
/* the server first calls eglInitialize (),
 * then look over the cached states
 */
void 
_server_initialize (EGLDisplay display) 
{
    link_list_t *head = server_states.states;
    link_list_t *list = head;
    link_list_t *current;

    egl_state_t *egl_state;

    mutex_lock (egl_mutex);

    if (server_states.initialized == false ||
        server_states.num_contexts == 0 || (! server_states.states)) {
        mutex_unlock (egl_mutex);
        return;
    }
    
    while (list != NULL) {
        egl_state = (egl_state_t *) list->data;
        current = list;
        list = list->next;

        if (egl_state->display == display) {
            if (egl_state->destroy_dpy)
                egl_state->destroy_dpy = false;
        }
    }
    mutex_unlock (egl_mutex);
}
