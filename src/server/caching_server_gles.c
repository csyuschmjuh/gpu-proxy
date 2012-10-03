/* This file implements the server thread side of GLES2 functions.  Thses
 * functions are called by the command buffer.
 *
 * It references to the server thread side of CACHING_SERVER(server)->active_state.
 * if CACHING_SERVER(server)->active_state is NULL or there is no symbol for the corresponding
 * gl functions, the cached error state is set to GL_INVALID_OPERATION
 * if the cached error has not been set to one of the errors.
 */

/* This file implements the server thread side of egl functions.
 * After the server thread reads the command buffer, if it is 
 * egl calls, it is routed to here.
 *
 * It keeps two global variables for all server threads:
 * (1) dispatch - a dispatch table to real egl and gl calls. It 
 * is initialized during eglGetDisplay() -> caching_server_eglGetDisplay()
 * (2) server_states - this is a double-linked structure contains
 * all active and inactive egl states.  When a client switches context,
 * the previous egl_state is set to be inactive and thus is subject to
 * be destroyed during caching_server_eglTerminate(), caching_server_eglDestroyContext() and
 * caching_server_eglReleaseThread()  The inactive context's surface can also be 
 * destroyed by caching_server_eglDestroySurface().
 * (3) CACHING_SERVER(server)->active_state - this is the pointer to the current active state.
 */

#include "config.h"
#include "caching_server_private.h"

#include "server_dispatch_table.h"
#include "egl_states.h"
#include "server.h"
#include "types_private.h"
#include <EGL/eglext.h>
#include <EGL/egl.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2.h>
#include <stdlib.h>
#include <string.h>

/* global state variable for all server threads */
mutex_static_init (server_states_mutex);
gl_server_states_t server_states;
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
        state->texture_3d_wrap_r[i] = GL_REPEAT;
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

static void 
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

/* the server first calls eglTerminate (display),
 * then look over the cached states
 */
static void 
_server_terminate (server_t *server,
                   EGLDisplay display)
{
    link_list_t *head = server_states.states;
    link_list_t *list = head;
    link_list_t *current;

    mutex_lock (server_states_mutex);

    if (server_states.initialized == false ||
        server_states.num_contexts == 0 || (! server_states.states)) {
        mutex_unlock (server_states_mutex);
        return;
    }
    
    while (list != NULL) {
        egl_state_t *egl_state = (egl_state_t *) list->data;
        current = list;
        list = list->next;

        if (egl_state->display == display) {
            if (! egl_state->active)
                _server_remove_state (&current);
                /* XXX: Do we need to stop the thread? */
        }
    }

    /* is current active state affected ?*/
    if (CACHING_SERVER(server)->active_state) { 
        egl_state_t *egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        if (egl_state->display == display)
            egl_state->destroy_dpy = true;
    }
    /* XXX: should we stop current server thread ? */
    /*
    else if (! CACHING_SERVER(server)->active_state) {
    } */
    mutex_unlock (server_states_mutex);
}

/* we should call real eglMakeCurrent() before, and wait for result
 * if eglMakeCurrent() returns EGL_TRUE, then we call this
 */
static void 
_server_make_current (server_t *server,
                      EGLDisplay display,
                      EGLSurface drawable,
                      EGLSurface readable,
                      EGLContext context)
{
    egl_state_t *egl_state;

    mutex_lock (server_states_mutex);
    /* we are switching to None context */
    if (context == EGL_NO_CONTEXT || display == EGL_NO_DISPLAY) {
        /* current is None too */
        if (! CACHING_SERVER(server)->active_state) {
            CACHING_SERVER(server)->active_state = NULL;
            mutex_unlock (server_states_mutex);
            return;
        }
        
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        egl_state->active = false;
        
        if (egl_state->destroy_dpy || egl_state->destroy_ctx)
            _server_remove_state (&CACHING_SERVER(server)->active_state);
        
        if (CACHING_SERVER(server)->active_state) {
            if (egl_state->destroy_read)
                _server_remove_surface (CACHING_SERVER(server)->active_state, true);

            if (egl_state->destroy_draw)
                _server_remove_surface (CACHING_SERVER(server)->active_state, false);
        }

        CACHING_SERVER(server)->active_state = NULL;
        mutex_unlock (server_states_mutex);
        return;
    }

    /* we are switch to one of the valid context */
    if (CACHING_SERVER(server)->active_state) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        egl_state->active = false;
        
        /* XXX did eglTerminate()/eglDestroyContext()/eglDestroySurface()
         * called affect us?
         */
        if (egl_state->destroy_dpy || egl_state->destroy_ctx)
            _server_remove_state (&CACHING_SERVER(server)->active_state);
        
        if (CACHING_SERVER(server)->active_state) {
            if (egl_state->destroy_read)
                _server_remove_surface (CACHING_SERVER(server)->active_state, true);

            if (egl_state->destroy_draw)
                _server_remove_surface (CACHING_SERVER(server)->active_state, false);
        }
    }

    /* get existing state or create a new one */
    CACHING_SERVER(server)->active_state = _server_get_state (display, drawable,
                                                              readable, context);
    mutex_unlock (server_states_mutex);
}

/* called by eglDestroyContext() - once we know there is matching context
 * we call real eglDestroyContext(), and if returns EGL_TRUE, we call 
 * this function 
 */
static void
_server_destroy_context (server_t *server,
                         EGLDisplay display, 
                         EGLContext context)
{
    egl_state_t *state;
    link_list_t *list = server_states.states;
    link_list_t *current;

    mutex_lock (server_states_mutex);
    if (server_states.num_contexts == 0 || ! server_states.states) {
        mutex_unlock (server_states_mutex);
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
    mutex_unlock (server_states_mutex);
}

static void
_server_destroy_surface (server_t *server,
                         EGLDisplay display, 
                         EGLSurface surface)
{
    egl_state_t *state;
    link_list_t *list = server_states.states;
    link_list_t *current;

    mutex_lock (server_states_mutex);
    if (server_states.num_contexts == 0 || ! server_states.states) {
        mutex_unlock (server_states_mutex);
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

    mutex_unlock (server_states_mutex);
}

static bool
_match (EGLDisplay display,
        EGLSurface drawable,
        EGLSurface readable,
        EGLContext context, 
        link_list_t **state)
{
    egl_state_t *egl_state;
    link_list_t *list = server_states.states;
    link_list_t *current;

    mutex_lock (server_states_mutex);
    if (server_states.num_contexts == 0 || ! server_states.states) {
        mutex_unlock (server_states_mutex);
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
            mutex_unlock (server_states_mutex);
            return true;
        }

    }

    mutex_unlock (server_states_mutex);
    return false;
}
/* the server first calls eglInitialize (),
 * then look over the cached states
 */
static void 
_server_initialize (EGLDisplay display) 
{
    link_list_t *head = server_states.states;
    link_list_t *list = head;

    egl_state_t *egl_state;

    mutex_lock (server_states_mutex);

    if (server_states.initialized == false ||
        server_states.num_contexts == 0 || (! server_states.states)) {
        mutex_unlock (server_states_mutex);
        return;
    }
    
    while (list != NULL) {
        egl_state = (egl_state_t *) list->data;
        list = list->next;

        if (egl_state->display == display) {
            if (egl_state->destroy_dpy)
                egl_state->destroy_dpy = false;
        }
    }
    mutex_unlock (server_states_mutex);
}

static bool
caching_server_glIsValidFunc (server_t *server, 
                              void *func)
{
    egl_state_t *egl_state;

    if (func)
        return true;

    if (CACHING_SERVER(server)->active_state) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        if (egl_state->active == true &&
            egl_state->state.error == GL_NO_ERROR) {
            egl_state->state.error = GL_INVALID_OPERATION;
            return false;
        }
    }
    return false;
}

static bool
caching_server_glIsValidContext (server_t *server)
{
    egl_state_t *egl_state;

    bool is_valid = false;

    if (CACHING_SERVER(server)->active_state) {
        egl_state = (egl_state_t *)CACHING_SERVER(server)->active_state->data;
        if (egl_state->active)
            return true;
    }
    return is_valid;
}

static void
caching_server_glSetError (server_t *server, GLenum error)
{
    egl_state_t *egl_state;

    if (CACHING_SERVER(server)->active_state) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
 
        if (egl_state->active && egl_state->state.error == GL_NO_ERROR)
            egl_state->state.error = error;
    }
}

/* GLES2 core profile API */
static void
caching_server_glActiveTexture (server_t *server, GLenum texture)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glActiveTexture) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (egl_state->state.active_texture == texture)
            return;
        else if (texture > GL_TEXTURE31 || texture < GL_TEXTURE0) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }
        else {
            CACHING_SERVER(server)->super_dispatch.glActiveTexture (server, texture);
            /* FIXME: this maybe not right because this texture may be 
             * invalid object, we save here to save time in glGetError() 
             */
            egl_state->state.active_texture = texture;
        }
    }
}

static void
caching_server_glBindAttribLocation (server_t *server, GLuint program, GLuint index, const GLchar *name)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glBindAttribLocation) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        CACHING_SERVER(server)->super_dispatch.glBindAttribLocation (server, program, index, name);
        egl_state->state.need_get_error = true;
    }
    if (name)
        free ((char *)name);
}

static void
caching_server_glBindBuffer (server_t *server, GLenum target, GLuint buffer)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glBindBuffer) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (target == GL_ARRAY_BUFFER) {
            if (egl_state->state.array_buffer_binding == buffer)
                return;
            else {
                CACHING_SERVER(server)->super_dispatch.glBindBuffer (server, target, buffer);
                egl_state->state.need_get_error = true;

               /* FIXME: we don't know whether it succeeds or not */
               egl_state->state.array_buffer_binding = buffer;
            }
        }
        else if (target == GL_ELEMENT_ARRAY_BUFFER) {
            if (egl_state->state.element_array_buffer_binding == buffer)
                return;
            else {
                CACHING_SERVER(server)->super_dispatch.glBindBuffer (server, target, buffer);
                egl_state->state.need_get_error = true;

               /* FIXME: we don't know whether it succeeds or not */
               egl_state->state.element_array_buffer_binding = buffer;
            }
        }
        else {
            caching_server_glSetError (server, GL_INVALID_ENUM);
        }
    }
}

static void
caching_server_glBindFramebuffer (server_t *server, GLenum target, GLuint framebuffer)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glBindFramebuffer) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (target == GL_FRAMEBUFFER &&
            egl_state->state.framebuffer_binding == framebuffer)
                return;

        if (target != GL_FRAMEBUFFER) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
        }

        CACHING_SERVER(server)->super_dispatch.glBindFramebuffer (server, target, framebuffer);
        /* FIXME: should we save it, it will be invalid if the
         * framebuffer is invalid 
         */
        egl_state->state.framebuffer_binding = framebuffer;

        /* egl_state->state.need_get_error = true; */
    }
}

static void
caching_server_glBindRenderbuffer (server_t *server, GLenum target, GLuint renderbuffer)
{
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glBindRenderbuffer) &&
        caching_server_glIsValidContext (server)) {

        if (target != GL_RENDERBUFFER) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
        }

        CACHING_SERVER(server)->super_dispatch.glBindRenderbuffer (server, target, renderbuffer);
        /* egl_state->state.need_get_error = true; */
    }
}

static void
caching_server_glBindTexture (server_t *server, GLenum target, GLuint texture)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glBindTexture) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (target == GL_TEXTURE_2D &&
            egl_state->state.texture_binding[0] == texture)
            return;
        else if (target == GL_TEXTURE_CUBE_MAP &&
                 egl_state->state.texture_binding[1] == texture)
            return;
        else if (target == GL_TEXTURE_3D_OES &&
                 egl_state->state.texture_binding_3d == texture)
            return;

        if (! (target == GL_TEXTURE_2D || target == GL_TEXTURE_CUBE_MAP
           || target == GL_TEXTURE_3D_OES
          )) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glBindTexture (server, target, texture);
        egl_state->state.need_get_error = true;

        /* FIXME: do we need to save them ? */
        if (target == GL_TEXTURE_2D)
            egl_state->state.texture_binding[0] = texture;
        else if (target == GL_TEXTURE_CUBE_MAP)
            egl_state->state.texture_binding[1] = texture;
        else
            egl_state->state.texture_binding_3d = texture;

        /* egl_state->state.need_get_error = true; */
    }
}

static void
caching_server_glBlendColor (server_t *server, GLclampf red, GLclampf green,
                             GLclampf blue, GLclampf alpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glBlendColor) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
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

        CACHING_SERVER(server)->super_dispatch.glBlendColor (server, red, green, blue, alpha);
    }
}

static void
caching_server_glBlendEquation (server_t *server, GLenum mode)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glBlendEquation) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        state = &egl_state->state;
    
        if (state->blend_equation[0] == mode &&
            state->blend_equation[1] == mode)
            return;

        if (! (mode == GL_FUNC_ADD ||
               mode == GL_FUNC_SUBTRACT ||
               mode == GL_FUNC_REVERSE_SUBTRACT)) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        state->blend_equation[0] = mode;
        state->blend_equation[1] = mode;

        CACHING_SERVER(server)->super_dispatch.glBlendEquation (server, mode);
    }
}

static void
caching_server_glBlendEquationSeparate (server_t *server, GLenum modeRGB, GLenum modeAlpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glBlendEquationSeparate) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
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
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        state->blend_equation[0] = modeRGB;
        state->blend_equation[1] = modeAlpha;

        CACHING_SERVER(server)->super_dispatch.glBlendEquationSeparate (server, modeRGB, modeAlpha);
    }
}

static void
caching_server_glBlendFunc (server_t *server, GLenum sfactor, GLenum dfactor)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glBlendFunc) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
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
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        state->blend_src[0] = state->blend_src[1] = sfactor;
        state->blend_dst[0] = state->blend_dst[1] = dfactor;

        CACHING_SERVER(server)->super_dispatch.glBlendFunc (server, sfactor, dfactor);
    }
}

static void
caching_server_glBlendFuncSeparate (server_t *server, GLenum srcRGB, GLenum dstRGB,
                                    GLenum srcAlpha, GLenum dstAlpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glBlendFuncSeparate) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
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
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        state->blend_src[0] = srcRGB;
        state->blend_src[1] = srcAlpha;
        state->blend_dst[0] = dstRGB;
        state->blend_dst[0] = dstAlpha;

        CACHING_SERVER(server)->super_dispatch.glBlendFuncSeparate (server, srcRGB, dstRGB, srcAlpha, dstAlpha);
    }
}

static void
caching_server_glBufferData (server_t *server, GLenum target, GLsizeiptr size,
                             const GLvoid *data, GLenum usage)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glBufferData) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        /* XXX: we skip rest of check, because driver
         * can generate GL_OUT_OF_MEMORY, which cannot check
         */
        CACHING_SERVER(server)->super_dispatch.glBufferData (server, target, size, data, usage);
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        egl_state->state.need_get_error = true;
    }

    if (data)
        free ((void *)data);
}

static void
caching_server_glBufferSubData (server_t *server, GLenum target, GLintptr offset,
                     GLsizeiptr size, const GLvoid *data)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glBufferSubData) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        /* XXX: we skip rest of check, because driver
         * can generate GL_INVALID_VALUE, when offset + data can be out of
         * bound
         */
        CACHING_SERVER(server)->super_dispatch.glBufferSubData (server, target, offset, size, data);
        egl_state->state.need_get_error = true;
    }

    if (data)
        free ((void *)data);
}

static GLenum
caching_server_glCheckFramebufferStatus (server_t *server, GLenum target)
{
    GLenum result = GL_INVALID_ENUM;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glCheckFramebufferStatus) &&
        caching_server_glIsValidContext (server)) {

        if (target != GL_FRAMEBUFFER) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return result;
        }

        return CACHING_SERVER(server)->super_dispatch.glCheckFramebufferStatus (server, target);
    }

    return result;
}

static void
caching_server_glClear (server_t *server, GLbitfield mask)
{
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glClear) &&
        caching_server_glIsValidContext (server)) {

        if (! (mask & GL_COLOR_BUFFER_BIT ||
               mask & GL_DEPTH_BUFFER_BIT ||
               mask & GL_STENCIL_BUFFER_BIT)) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glClear (server, mask);
    }
}

static void
caching_server_glClearColor (server_t *server, GLclampf red, GLclampf green,
                 GLclampf blue, GLclampf alpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glClearColor) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
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

        CACHING_SERVER(server)->super_dispatch.glClearColor (server, red, green, blue, alpha);
    }
}

static void
caching_server_glClearDepthf (server_t *server, GLclampf depth)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glClearDepthf) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        state = &egl_state->state;

        if (state->depth_clear_value == depth)
            return;

        state->depth_clear_value = depth;

        CACHING_SERVER(server)->super_dispatch.glClearDepthf (server, depth);
    }
}

static void
caching_server_glClearStencil (server_t *server, GLint s)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glClearStencil) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        state = &egl_state->state;

        if (state->stencil_clear_value == s)
            return;

        state->stencil_clear_value = s;

        CACHING_SERVER(server)->super_dispatch.glClearStencil (server, s);
    }
}

static void
caching_server_glColorMask (server_t *server, GLboolean red, GLboolean green,
                GLboolean blue, GLboolean alpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glColorMask) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
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

        CACHING_SERVER(server)->super_dispatch.glColorMask (server, red, green, blue, alpha);
    }
}

static void
caching_server_glCompressedTexImage2D (server_t *server, GLenum target, GLint level,
                                         GLenum internalformat,
                                         GLsizei width, GLsizei height,
                                         GLint border, GLsizei imageSize,
                                         const GLvoid *data)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glCompressedTexImage2D) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        CACHING_SERVER(server)->super_dispatch.glCompressedTexImage2D (server, target, level, internalformat,
                                       width, height, border, imageSize,
                                       data);

        egl_state->state.need_get_error = true;
    }

    if (data)
        free ((void *)data);
}

static void
caching_server_glCompressedTexSubImage2D (server_t *server, GLenum target, GLint level,
                                             GLint xoffset, GLint yoffset,
                                             GLsizei width, GLsizei height,
                                             GLenum format, 
                                             GLsizei imageSize,
                                             const GLvoid *data)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glCompressedTexSubImage2D) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        CACHING_SERVER(server)->super_dispatch.glCompressedTexSubImage2D (server, target, level, xoffset, yoffset,
                                          width, height, format, imageSize,
                                          data);

        egl_state->state.need_get_error = true;
    }

    if (data)
        free ((void *)data);
}

static GLuint
caching_server_glCreateShader (server_t *server, GLenum shaderType)
{
    GLuint result = 0;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glCreateShader) &&
        caching_server_glIsValidContext (server)) {

        if (! (shaderType == GL_VERTEX_SHADER ||
               shaderType == GL_FRAGMENT_SHADER)) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return result;
        }

        result = CACHING_SERVER(server)->super_dispatch.glCreateShader (server, shaderType);
    }

    return result;
}

static void
caching_server_glCullFace (server_t *server, GLenum mode)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glCullFace) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (egl_state->state.cull_face_mode == mode)
            return;

        if (! (mode == GL_FRONT ||
               mode == GL_BACK ||
               mode == GL_FRONT_AND_BACK)) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        egl_state->state.cull_face_mode = mode;

        CACHING_SERVER(server)->super_dispatch.glCullFace (server, mode);
    }
}

static void
caching_server_glDeleteBuffers (server_t *server, GLsizei n, const GLuint *buffers)
{
    egl_state_t *egl_state;
    int count;
    int i, j;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glDeleteBuffers) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        vertex_attrib_list_t *attrib_list = &egl_state->state.vertex_attribs;
        vertex_attrib_t *attribs = attrib_list->attribs;
        count = attrib_list->count;

        if (n < 0) {
            caching_server_glSetError (server, GL_INVALID_VALUE);
            goto FINISH;
        }

        CACHING_SERVER(server)->super_dispatch.glDeleteBuffers (server, n, buffers);

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

static void
caching_server_glDeleteFramebuffers (server_t *server, GLsizei n, const GLuint *framebuffers)
{
    egl_state_t *egl_state;
    int i;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glDeleteFramebuffers) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (n < 0) {
            caching_server_glSetError (server, GL_INVALID_VALUE);
            goto FINISH;
        }

        CACHING_SERVER(server)->super_dispatch.glDeleteFramebuffers (server, n, framebuffers);

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

static void
caching_server_glDeleteRenderbuffers (server_t *server, GLsizei n, const GLuint *renderbuffers)
{
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glDeleteRenderbuffers) &&
        caching_server_glIsValidContext (server)) {

        if (n < 0) {
            caching_server_glSetError (server, GL_INVALID_VALUE);
            goto FINISH;
        }

        CACHING_SERVER(server)->super_dispatch.glDeleteRenderbuffers (server, n, renderbuffers);
    }

FINISH:
    if (renderbuffers)
        free ((void *)renderbuffers);
}

static void
caching_server_glDeleteTextures (server_t *server, GLsizei n, const GLuint *textures)
{
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glDeleteTextures) &&
        caching_server_glIsValidContext (server)) {

        if (n < 0) {
            caching_server_glSetError (server, GL_INVALID_VALUE);
            goto FINISH;
        }

        CACHING_SERVER(server)->super_dispatch.glDeleteTextures (server, n, textures);
    }

FINISH:
    if (textures)
        free ((void *)textures);
}

static void
caching_server_glDepthFunc (server_t *server, GLenum func)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glDepthFunc) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

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
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        egl_state->state.depth_func = func;

        CACHING_SERVER(server)->super_dispatch.glDepthFunc (server, func);
    }
}

static void
caching_server_glDepthMask (server_t *server, GLboolean flag)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glDepthMask) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (egl_state->state.depth_writemask == flag)
            return;

        egl_state->state.depth_writemask = flag;

        CACHING_SERVER(server)->super_dispatch.glDepthMask (server, flag);
    }
}

static void
caching_server_glDepthRangef (server_t *server, GLclampf nearVal, GLclampf farVal)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glDepthRangef) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (egl_state->state.depth_range[0] == nearVal &&
            egl_state->state.depth_range[1] == farVal)
            return;

        egl_state->state.depth_range[0] = nearVal;
        egl_state->state.depth_range[1] = farVal;

        CACHING_SERVER(server)->super_dispatch.glDepthRangef (server, nearVal, farVal);
    }
}

static void
caching_server_glDetachShader (server_t *server, GLuint program, GLuint shader)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glDetachShader) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        /* XXX: command buffer, error code generated */
        CACHING_SERVER(server)->super_dispatch.glDetachShader (server, program, shader);
        egl_state->state.need_get_error = true;
    }
}

 void
caching_server_glSetCap (server_t *server, GLenum cap, GLboolean enable)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    bool needs_call = false;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glDisable) &&
        caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glEnable) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

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
                CACHING_SERVER(server)->super_dispatch.glEnable (server, cap);
            else
                CACHING_SERVER(server)->super_dispatch.glDisable (server, cap);
        }
    }
}

static void
caching_server_glDisable (server_t *server, GLenum cap)
{
    caching_server_glSetCap (server, cap, GL_FALSE);
}

static void
caching_server_glEnable (server_t *server, GLenum cap)
{
    caching_server_glSetCap (server, cap, GL_TRUE);
}

 bool
caching_server_glIndexIsTooLarge (server_t *server, gles2_state_t *state, GLuint index)
{
    if (index > state->max_vertex_attribs) {
        if (! state->max_vertex_attribs_queried) {
            /* XXX: command buffer */
            CACHING_SERVER(server)->super_dispatch.glGetIntegerv (server, GL_MAX_VERTEX_ATTRIBS,
                                  &(state->max_vertex_attribs));
        }
        if (index > state->max_vertex_attribs) {
            if (state->error == GL_NO_ERROR)
                state->error = GL_INVALID_VALUE;
            return true;
        }
    }

    return false;
}

 void 
caching_server_glSetVertexAttribArray (server_t *server, GLuint index, gles2_state_t *state, 
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
    if (caching_server_glIndexIsTooLarge (server, state, index)) {
        return;
    }

    if (enable == GL_FALSE)
        CACHING_SERVER(server)->super_dispatch.glDisableVertexAttribArray (server, index);
    else
        CACHING_SERVER(server)->super_dispatch.glEnableVertexAttribArray (server, index);

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

static void
caching_server_glDisableVertexAttribArray (server_t *server, GLuint index)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glDisableVertexAttribArray) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        state = &egl_state->state;

        caching_server_glSetVertexAttribArray (server, index, state, GL_FALSE);
    }
}

static void
caching_server_glEnableVertexAttribArray (server_t *server, GLuint index)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glEnableVertexAttribArray) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        state = &egl_state->state;

        caching_server_glSetVertexAttribArray (server, index, state, GL_TRUE);
    }
}

static void
caching_server_glDrawArrays (server_t *server, GLenum mode, GLint first, GLsizei count)
{
    gles2_state_t *state;
    egl_state_t *egl_state;
    vertex_attrib_list_t *attrib_list = NULL;
    vertex_attrib_t *attribs = NULL;
    int i;
    bool needs_call = false;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glDrawArrays) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
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
            caching_server_glSetError (server, GL_INVALID_ENUM);
            goto FINISH;
        }
        else if (count < 0) {
            caching_server_glSetError (server, GL_INVALID_VALUE);
            goto FINISH;
        }

        /* if vertex array binding is not 0 */
        if (state->vertex_array_binding) {
            CACHING_SERVER(server)->super_dispatch.glDrawArrays (server, mode, first, count);
            state->need_get_error = true;
            goto FINISH;
        } else
  
        for (i = 0; i < attrib_list->count; i++) {
            if (! attribs[i].array_enabled)
                continue;
            /* we need to create a separate buffer for it */
            else if (! attribs[i].array_buffer_binding) {
                if (! attribs[i].data)
                    continue;
                CACHING_SERVER(server)->super_dispatch.glVertexAttribPointer (server, attribs[i].index,
                                              attribs[i].size,
                                              attribs[i].type,
                                              attribs[i].array_normalized,
                                              0,
                                              attribs[i].data);
                if (! needs_call)
                    needs_call = true;
            }
        }

        /* we need call DrawArrays */
        if (needs_call)
            CACHING_SERVER(server)->super_dispatch.glDrawArrays (server, mode, first, count);
    }

FINISH:
    for (i = 0; i < attrib_list->count; i++) {
        if (attribs[i].data) {
            free (attribs[i].data);
            attribs[i].data = NULL;
        }
    }
}

/* FIXME: we should use pre-allocated buffer if possible */
/*
static char *
caching_server_glCreateIndicesArray (server_t *server, GLenum mode, GLenum type, int count,
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

static void
caching_server_glDrawElements (server_t *server, GLenum mode, GLsizei count, GLenum type,
                               const GLvoid *indices)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    vertex_attrib_list_t *attrib_list = NULL;
    vertex_attrib_t *attribs = NULL;
    int i = -1;
    bool element_binding = false;
    bool needs_call = false;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glDrawElements) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
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
            caching_server_glSetError (server, GL_INVALID_ENUM);
            goto FINISH;
        }
        else if (! (type == GL_UNSIGNED_BYTE  || 
                    type == GL_UNSIGNED_SHORT)) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            goto FINISH;
        }
        else if (count < 0) {
            caching_server_glSetError (server, GL_INVALID_VALUE);
            goto FINISH;
        }

        if (state->vertex_array_binding) {
            CACHING_SERVER(server)->super_dispatch.glDrawElements (server, mode, count, type, indices);
            state->need_get_error = true;
            goto FINISH;
        }

        for (i = 0; i < attrib_list->count; i++) {
            if (! attribs[i].array_enabled)
                continue;
            /* we need to create a separate buffer for it */
            else if (! attribs[i].array_buffer_binding) {
                if (! attribs[i].data)
                    continue;
                CACHING_SERVER(server)->super_dispatch.glVertexAttribPointer (server, attribs[i].index,
                                              attribs[i].size,
                                              attribs[i].type,
                                              attribs[i].array_normalized,
                                              0,
                                              attribs[i].data);
                if (! needs_call)
                    needs_call = true;
            }
        }

        if (needs_call)
            CACHING_SERVER(server)->super_dispatch.glDrawElements (server, mode, type, count, indices);
    }
FINISH:
    for (i = 0; i < attrib_list->count; i++) {
        if (attribs[i].data) {
            free (attribs[i].data);
            attribs[i].data = NULL;
        }
    }
    
    if (element_binding == false) {
        if (indices)
            free ((void *)indices);
    }
}

static void
caching_server_glFramebufferRenderbuffer (server_t *server, GLenum target, GLenum attachment,
                                          GLenum renderbuffertarget,
                                          GLenum renderbuffer)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glFramebufferRenderbuffer) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (target != GL_FRAMEBUFFER) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }
        else if (renderbuffertarget != GL_RENDERBUFFER &&
                 renderbuffer != 0) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glFramebufferRenderbuffer (server, target, attachment,
                                          renderbuffertarget, 
                                          renderbuffer);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glFramebufferTexture2D (server_t *server, GLenum target, GLenum attachment,
                            GLenum textarget, GLuint texture, 
                            GLint level)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glFramebufferTexture2D) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (target != GL_FRAMEBUFFER) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glFramebufferTexture2D (server, target, attachment, textarget,
                                       texture, level);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glFrontFace (server_t *server, GLenum mode)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glFramebufferTexture2D) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (egl_state->state.front_face == mode)
            return;

        if (! (mode == GL_CCW || mode == GL_CW)) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        egl_state->state.front_face = mode;
        CACHING_SERVER(server)->super_dispatch.glFrontFace (server, mode);
    }
}

static void
caching_server_glGenBuffers (server_t *server, GLsizei n, GLuint *buffers)
{
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGenBuffers) &&
        caching_server_glIsValidContext (server)) {

        if (n < 0) {
            caching_server_glSetError (server, GL_INVALID_VALUE);
            return;
        }
    
        CACHING_SERVER(server)->super_dispatch.glGenBuffers (server, n, buffers);
    }
}

static void
caching_server_glGenFramebuffers (server_t *server, GLsizei n, GLuint *framebuffers)
{
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGenFramebuffers) &&
        caching_server_glIsValidContext (server)) {

        if (n < 0) {
            caching_server_glSetError (server, GL_INVALID_VALUE);
            return;
        }
        
        CACHING_SERVER(server)->super_dispatch.glGenFramebuffers (server, n, framebuffers);
    }
}

static void
caching_server_glGenRenderbuffers (server_t *server, GLsizei n, GLuint *renderbuffers)
{
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGenRenderbuffers) &&
        caching_server_glIsValidContext (server)) {

        if (n < 0) {
            caching_server_glSetError (server, GL_INVALID_VALUE);
            return;
        }
        
        CACHING_SERVER(server)->super_dispatch.glGenRenderbuffers (server, n, renderbuffers);
    }
}

static void
caching_server_glGenTextures (server_t *server, GLsizei n, GLuint *textures)
{
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGenTextures) &&
        caching_server_glIsValidContext (server)) {

        if (n < 0) {
            caching_server_glSetError (server, GL_INVALID_VALUE);
            return;
        }
        
        CACHING_SERVER(server)->super_dispatch.glGenTextures (server, n, textures);
    }
}

static void
caching_server_glGenerateMipmap (server_t *server, GLenum target)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGenerateMipmap) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (! (target == GL_TEXTURE_2D       || 
               target == GL_TEXTURE_CUBE_MAP
                                             || 
               target == GL_TEXTURE_3D_OES
                                          )) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glGenerateMipmap (server, target);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glGetBooleanv (server_t *server, GLenum pname, GLboolean *params)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetBooleanv) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

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
            CACHING_SERVER(server)->super_dispatch.glGetBooleanv (server, pname, params);
            break;
        }
    }
}

static void
caching_server_glGetFloatv (server_t *server, GLenum pname, GLfloat *params)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetFloatv) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

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
            CACHING_SERVER(server)->super_dispatch.glGetFloatv (server, pname, params);
            break;
        }
    }
}

static void
caching_server_glGetIntegerv (server_t *server, GLenum pname, GLint *params)
{
    egl_state_t *egl_state;
    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int count;
    int i;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetIntegerv) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
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
                CACHING_SERVER(server)->super_dispatch.glGetIntegerv (server, pname, params);
                egl_state->state.max_combined_texture_image_units_queried = true;
                egl_state->state.max_combined_texture_image_units = *params;
            } else
                *params = egl_state->state.max_combined_texture_image_units;
            break;
        case GL_MAX_CUBE_MAP_TEXTURE_SIZE:
            if (! egl_state->state.max_cube_map_texture_size_queried) {
                CACHING_SERVER(server)->super_dispatch.glGetIntegerv (server, pname, params);
                egl_state->state.max_cube_map_texture_size = *params;
                egl_state->state.max_cube_map_texture_size_queried = true;
            } else
                *params = egl_state->state.max_cube_map_texture_size;
        break;
        case GL_MAX_FRAGMENT_UNIFORM_VECTORS:
            if (! egl_state->state.max_fragment_uniform_vectors_queried) {
                CACHING_SERVER(server)->super_dispatch.glGetIntegerv (server, pname, params);
                egl_state->state.max_fragment_uniform_vectors = *params;
                egl_state->state.max_fragment_uniform_vectors_queried = true;
            } else
                *params = egl_state->state.max_fragment_uniform_vectors;
            break;
        case GL_MAX_RENDERBUFFER_SIZE:
            if (! egl_state->state.max_renderbuffer_size_queried) {
                CACHING_SERVER(server)->super_dispatch.glGetIntegerv (server, pname, params);
                egl_state->state.max_renderbuffer_size = *params;
                egl_state->state.max_renderbuffer_size_queried = true;
            } else
                *params = egl_state->state.max_renderbuffer_size;
            break;
        case GL_MAX_TEXTURE_IMAGE_UNITS:
            if (! egl_state->state.max_texture_image_units_queried) {
                CACHING_SERVER(server)->super_dispatch.glGetIntegerv (server, pname, params);
                egl_state->state.max_texture_image_units = *params;
                egl_state->state.max_texture_image_units_queried = true;
            } else
                *params = egl_state->state.max_texture_image_units;
            break;
        case GL_MAX_VARYING_VECTORS:
            if (! egl_state->state.max_varying_vectors_queried) {
                CACHING_SERVER(server)->super_dispatch.glGetIntegerv (server, pname, params);
                egl_state->state.max_varying_vectors = *params;
                egl_state->state.max_varying_vectors_queried = true;
            } else
                *params = egl_state->state.max_varying_vectors;
            break;
        case GL_MAX_TEXTURE_SIZE:
            if (! egl_state->state.max_texture_size_queried) {
                CACHING_SERVER(server)->super_dispatch.glGetIntegerv (server, pname, params);
                egl_state->state.max_texture_size = *params;
                egl_state->state.max_texture_size_queried = true;
            } else
                *params = egl_state->state.max_texture_size;
            break;
        case GL_MAX_VERTEX_ATTRIBS:
            if (! egl_state->state.max_vertex_attribs_queried) {
                CACHING_SERVER(server)->super_dispatch.glGetIntegerv (server, pname, params);
                egl_state->state.max_vertex_attribs = *params;
                egl_state->state.max_vertex_attribs_queried = true;
            } else
                *params = egl_state->state.max_vertex_attribs;
            break;
        case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
            if (! egl_state->state.max_vertex_texture_image_units_queried) {
                CACHING_SERVER(server)->super_dispatch.glGetIntegerv (server, pname, params);
                egl_state->state.max_vertex_texture_image_units = *params;
                egl_state->state.max_vertex_texture_image_units_queried = true;
            } else
                *params = egl_state->state.max_vertex_texture_image_units;
            break;
        case GL_MAX_VERTEX_UNIFORM_VECTORS:
            if (! egl_state->state.max_vertex_uniform_vectors_queried) {
                CACHING_SERVER(server)->super_dispatch.glGetIntegerv (server, pname, params);
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
            CACHING_SERVER(server)->super_dispatch.glGetIntegerv (server, pname, params);
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
        else if (pname == GL_VERTEX_ARRAY_BINDING_OES)
            egl_state->state.vertex_array_binding = *params;
    }
}

static void
caching_server_glGetActiveAttrib (server_t *server, GLuint program, GLuint index,
                                   GLsizei bufsize, GLsizei *length,
                                   GLint *size, GLenum *type, GLchar *name)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetActiveAttrib) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        CACHING_SERVER(server)->super_dispatch.glGetActiveAttrib (server, program, index, bufsize, length,
                                  size, type, name);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glGetActiveUniform (server_t *server, GLuint program, GLuint index, 
                                    GLsizei bufsize, GLsizei *length, 
                                    GLint *size, GLenum *type, 
                                    GLchar *name)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetActiveUniform) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        CACHING_SERVER(server)->super_dispatch.glGetActiveUniform (server, program, index, bufsize, length,
                                   size, type, name);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glGetAttachedShaders (server_t *server, GLuint program, GLsizei maxCount,
                                      GLsizei *count, GLuint *shaders)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetAttachedShaders) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glGetAttachedShaders (server, program, maxCount, count, shaders);

        egl_state->state.need_get_error = true;
    }
}

static GLint
caching_server_glGetAttribLocation (server_t *server, GLuint program, const GLchar *name)
{
    egl_state_t *egl_state;
    GLint result = -1;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetAttribLocation) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        result = CACHING_SERVER(server)->super_dispatch.glGetAttribLocation (server, program, name);

        egl_state->state.need_get_error = true;
    }
    return result;
}

static void
caching_server_glGetBufferParameteriv (server_t *server, GLenum target, GLenum value, GLint *data)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetBufferParameteriv) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glGetBufferParameteriv (server, target, value, data);

        egl_state->state.need_get_error = true;
    }
}

static GLenum
caching_server_glGetError (server_t *server)
{
    GLenum error = GL_INVALID_OPERATION;
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetError) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        if (! egl_state->state.need_get_error) {
            error = egl_state->state.error;
            egl_state->state.error = GL_NO_ERROR;
            return error;
        }

        error = CACHING_SERVER(server)->super_dispatch.glGetError (server);

        egl_state->state.need_get_error = false;
        egl_state->state.error = GL_NO_ERROR;
    }

    return error;
}

static void
caching_server_glGetFramebufferAttachmentParameteriv (server_t *server, GLenum target,
                                                        GLenum attachment,
                                                        GLenum pname,
                                                        GLint *params)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetFramebufferAttachmentParameteriv) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        if (target != GL_FRAMEBUFFER) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glGetFramebufferAttachmentParameteriv (server, target, attachment,
                                                      pname, params);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glGetProgramInfoLog (server_t *server, GLuint program, GLsizei maxLength,
                                      GLsizei *length, GLchar *infoLog)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetProgramInfoLog) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        CACHING_SERVER(server)->super_dispatch.glGetProgramInfoLog (server, program, maxLength, length, infoLog);

        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glGetProgramiv (server_t *server, GLuint program, GLenum pname, GLint *params)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetProgramiv) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        CACHING_SERVER(server)->super_dispatch.glGetProgramiv (server, program, pname, params);

        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glGetRenderbufferParameteriv (server_t *server, GLenum target,
                                              GLenum pname,
                                              GLint *params)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetRenderbufferParameteriv) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (target != GL_RENDERBUFFER) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glGetRenderbufferParameteriv (server, target, pname, params);

        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glGetShaderInfoLog (server_t *server, GLuint program, GLsizei maxLength,
                                     GLsizei *length, GLchar *infoLog)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetShaderInfoLog) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        CACHING_SERVER(server)->super_dispatch.glGetShaderInfoLog (server, program, maxLength, length, infoLog);

        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glGetShaderPrecisionFormat (server_t *server, GLenum shaderType, 
                                             GLenum precisionType,
                                             GLint *range, 
                                             GLint *precision)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetShaderPrecisionFormat) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        CACHING_SERVER(server)->super_dispatch.glGetShaderPrecisionFormat (server, shaderType, precisionType,
                                           range, precision);

        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glGetShaderSource  (server_t *server, GLuint shader, GLsizei bufSize, 
                                    GLsizei *length, GLchar *source)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetShaderSource) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        CACHING_SERVER(server)->super_dispatch.glGetShaderSource (server, shader, bufSize, length, source);

        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glGetShaderiv (server_t *server, GLuint shader, GLenum pname, GLint *params)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetShaderiv) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        CACHING_SERVER(server)->super_dispatch.glGetShaderiv (server, shader, pname, params);

        egl_state->state.need_get_error = true;
    }
}

static const GLubyte *
caching_server_glGetString (server_t *server, GLenum name)
{
    GLubyte *result = NULL;
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetString) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (! (name == GL_VENDOR                   || 
               name == GL_RENDERER                 ||
               name == GL_SHADING_LANGUAGE_VERSION ||
               name == GL_EXTENSIONS)) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return NULL;
        }

        result = (GLubyte *)CACHING_SERVER(server)->super_dispatch.glGetString (server, name);

        egl_state->state.need_get_error = true;
    }
    return (const GLubyte *)result;
}

static void
caching_server_glGetTexParameteriv (server_t *server, GLenum target, GLenum pname, 
                                     GLint *params)
{
    egl_state_t *egl_state;
    int active_texture_index;
    int target_index;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetTexParameteriv) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (! (target == GL_TEXTURE_2D       || 
               target == GL_TEXTURE_CUBE_MAP
                                             || 
               target == GL_TEXTURE_3D_OES
                                          )) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        if (! (pname == GL_TEXTURE_MAG_FILTER ||
               pname == GL_TEXTURE_MIN_FILTER ||
               pname == GL_TEXTURE_WRAP_S     ||
               pname == GL_TEXTURE_WRAP_T
                                              || 
               pname == GL_TEXTURE_WRAP_R_OES
                                             )) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
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
        else if (pname == GL_TEXTURE_WRAP_R_OES)
            *params = egl_state->state.texture_3d_wrap_r[active_texture_index];
    }
}

static void
caching_server_glGetTexParameterfv (server_t *server, GLenum target, GLenum pname, GLfloat *params)
{
    GLint paramsi;

    caching_server_glGetTexParameteriv (server, target, pname, &paramsi);
    *params = paramsi;
}

static void
caching_server_glGetUniformiv (server_t *server, GLuint program, GLint location, 
                               GLint *params)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetUniformiv) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        CACHING_SERVER(server)->super_dispatch.glGetUniformiv (server, program, location, params);

        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glGetUniformfv (server_t *server, GLuint program, GLint location, 
                               GLfloat *params)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetUniformfv) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        CACHING_SERVER(server)->super_dispatch.glGetUniformfv (server, program, location, params);

        egl_state->state.need_get_error = true;
    }
}

static GLint
caching_server_glGetUniformLocation (server_t *server, GLuint program, const GLchar *name)
{
    GLint result = -1;
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetUniformLocation) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        result = CACHING_SERVER(server)->super_dispatch.glGetUniformLocation (server, program, name);

        egl_state->state.need_get_error = true;
    }
    return result;
}

static void
caching_server_glGetVertexAttribfv (server_t *server, GLuint index, GLenum pname, 
                                     GLfloat *params)
{
    egl_state_t *egl_state;

    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int count;
    int i;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetVertexAttribfv) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
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
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        /* check index is too large */
        if (caching_server_glIndexIsTooLarge (server, &egl_state->state, index)) {
            return;
        }

        /* we cannot use client state */
        if (egl_state->state.vertex_array_binding) {
            CACHING_SERVER(server)->super_dispatch.glGetVertexAttribfv (server, index, pname, params);
            return;
        }

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

static void
caching_server_glGetVertexAttribiv (server_t *server, GLuint index, GLenum pname, GLint *params)
{
    GLfloat paramsf[4];
    int i;

    CACHING_SERVER(server)->super_dispatch.glGetVertexAttribfv (server, index, pname, paramsf);

    if (pname == GL_CURRENT_VERTEX_ATTRIB) {
        for (i = 0; i < 4; i++)
            params[i] = paramsf[i];
    } else
        *params = paramsf[0];
}

static void
caching_server_glGetVertexAttribPointerv (server_t *server, GLuint index, GLenum pname, 
                                            GLvoid **pointer)
{
    egl_state_t *egl_state;

    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int count;
    int i;
    
    *pointer = 0;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetVertexAttribPointerv) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        attrib_list = &egl_state->state.vertex_attribs;
        attribs = attrib_list->attribs;
        count = attrib_list->count;

        if (pname != GL_VERTEX_ATTRIB_ARRAY_POINTER) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        /* XXX: check index validity */
        if (caching_server_glIndexIsTooLarge (server, &egl_state->state, index)) {
            return;
        }

        /* we cannot use client state */
        if (egl_state->state.vertex_array_binding) {
            CACHING_SERVER(server)->super_dispatch.glGetVertexAttribPointerv (server, index, pname, pointer);
            egl_state->state.need_get_error = true;
            return;
        }

        /* look into client state */
        for (i = 0; i < count; i++) {
            if (attribs[i].index == index) {
                *pointer = attribs[i].pointer;
                return;
            }
        }
    }
}

static void
caching_server_glHint (server_t *server, GLenum target, GLenum mode)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glHint) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (target == GL_GENERATE_MIPMAP_HINT &&
            egl_state->state.generate_mipmap_hint == mode)
            return;

        if ( !(mode == GL_FASTEST ||
               mode == GL_NICEST  ||
               mode == GL_DONT_CARE)) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        if (target == GL_GENERATE_MIPMAP_HINT)
            egl_state->state.generate_mipmap_hint = mode;

        CACHING_SERVER(server)->super_dispatch.glHint (server, target, mode);

        if (target != GL_GENERATE_MIPMAP_HINT)
        egl_state->state.need_get_error = true;
    }
}

static GLboolean
caching_server_glIsBuffer (server_t *server, GLuint buffer)
{
    GLboolean result = GL_FALSE;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glIsBuffer) &&
        caching_server_glIsValidContext (server)) {

        result = CACHING_SERVER(server)->super_dispatch.glIsBuffer (server, buffer);
    }
    return result;
}

static GLboolean
caching_server_glIsEnabled (server_t *server, GLenum cap)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glIsEnabled) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

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
            caching_server_glSetError (server, GL_INVALID_ENUM);
            break;
        }
    }
    return result;
}

static GLboolean
caching_server_glIsFramebuffer (server_t *server, GLuint framebuffer)
{
    GLboolean result = GL_FALSE;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glIsFramebuffer) &&
        caching_server_glIsValidContext (server)) {

        result = CACHING_SERVER(server)->super_dispatch.glIsFramebuffer (server, framebuffer);
    }
    return result;
}

static GLboolean
caching_server_glIsProgram (server_t *server, GLuint program)
{
    GLboolean result = GL_FALSE;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glIsProgram) &&
        caching_server_glIsValidContext (server)) {

        result = CACHING_SERVER(server)->super_dispatch.glIsProgram (server, program);
    }
    return result;
}

static GLboolean
caching_server_glIsRenderbuffer (server_t *server, GLuint renderbuffer)
{
    GLboolean result = GL_FALSE;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glIsRenderbuffer) &&
        caching_server_glIsValidContext (server)) {

        result = CACHING_SERVER(server)->super_dispatch.glIsRenderbuffer (server, renderbuffer);
    }
    return result;
}

static GLboolean
caching_server_glIsShader (server_t *server, GLuint shader)
{
    GLboolean result = GL_FALSE;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glIsShader) &&
        caching_server_glIsValidContext (server)) {

        result = CACHING_SERVER(server)->super_dispatch.glIsShader (server, shader);
    }
    return result;
}

static GLboolean
caching_server_glIsTexture (server_t *server, GLuint texture)
{
    GLboolean result = GL_FALSE;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glIsTexture) &&
        caching_server_glIsValidContext (server)) {

        result = CACHING_SERVER(server)->super_dispatch.glIsTexture (server, texture);
    }
    return result;
}

static void
caching_server_glLineWidth (server_t *server, GLfloat width)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glLineWidth) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (egl_state->state.line_width == width)
            return;

        if (width < 0) {
            caching_server_glSetError (server, GL_INVALID_VALUE);
            return;
        }

        egl_state->state.line_width = width;
        CACHING_SERVER(server)->super_dispatch.glLineWidth (server, width);
    }
}

static void
caching_server_glPixelStorei (server_t *server, GLenum pname, GLint param)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glPixelStorei) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if ((pname == GL_PACK_ALIGNMENT                &&
             egl_state->state.pack_alignment == param) ||
            (pname == GL_UNPACK_ALIGNMENT              &&
             egl_state->state.unpack_alignment == param))
            return;

        if (! (param == 1 ||
               param == 2 ||
               param == 4 ||
               param == 8)) {
            caching_server_glSetError (server, GL_INVALID_VALUE);
            return;
        }
        else if (! (pname == GL_PACK_ALIGNMENT ||
                    pname == GL_UNPACK_ALIGNMENT)) {
            caching_server_glSetError (server, GL_INVALID_VALUE);
            return;
        }

        if (pname == GL_PACK_ALIGNMENT)
           egl_state->state.pack_alignment = param;
        else
           egl_state->state.unpack_alignment = param;

        CACHING_SERVER(server)->super_dispatch.glPixelStorei (server, pname, param);
    }
}

static void
caching_server_glPolygonOffset (server_t *server, GLfloat factor, GLfloat units)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glPolygonOffset) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (egl_state->state.polygon_offset_factor == factor &&
            egl_state->state.polygon_offset_units == units)
            return;

        egl_state->state.polygon_offset_factor = factor;
        egl_state->state.polygon_offset_units = units;

        CACHING_SERVER(server)->super_dispatch.glPolygonOffset (server, factor, units);
    }
}

static void
caching_server_glReadPixels (server_t *server, GLint x, GLint y,
                             GLsizei width, GLsizei height,
                             GLenum format, GLenum type, GLvoid *data)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glReadPixels) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        CACHING_SERVER(server)->super_dispatch.glReadPixels (server, x, y, width, height, format, type, data);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glSampleCoverage (server_t *server, GLclampf value, GLboolean invert)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glSampleCoverage) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (value == egl_state->state.sample_coverage_value &&
            invert == egl_state->state.sample_coverage_invert)
            return;

        egl_state->state.sample_coverage_invert = invert;
        egl_state->state.sample_coverage_value = value;

        CACHING_SERVER(server)->super_dispatch.glSampleCoverage (server, value, invert);
    }
}

static void
caching_server_glScissor (server_t *server, GLint x, GLint y, GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glScissor) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (x == egl_state->state.scissor_box[0]     &&
            y == egl_state->state.scissor_box[1]     &&
            width == egl_state->state.scissor_box[2] &&
            height == egl_state->state.scissor_box[3])
            return;

        if (width < 0 || height < 0) {
            caching_server_glSetError (server, GL_INVALID_VALUE);
            return;
        }

        egl_state->state.scissor_box[0] = x;
        egl_state->state.scissor_box[1] = y;
        egl_state->state.scissor_box[2] = width;
        egl_state->state.scissor_box[3] = height;

        CACHING_SERVER(server)->super_dispatch.glScissor (server, x, y, width, height);
    }
}

static void
caching_server_glShaderBinary (server_t *server, GLsizei n, const GLuint *shaders,
                               GLenum binaryformat, const void *binary,
                               GLsizei length)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glShaderBinary) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        CACHING_SERVER(server)->super_dispatch.glShaderBinary (server, n, shaders, binaryformat, binary, length);
        egl_state->state.need_get_error = true;
    }
    if (binary)
        free ((void *)binary);
}

static void
caching_server_glShaderSource (server_t *server, GLuint shader, GLsizei count,
                               const GLchar **string, const GLint *length)
{
    egl_state_t *egl_state;
    int i;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glShaderSource) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        CACHING_SERVER(server)->super_dispatch.glShaderSource (server, shader, count, string, length);
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

static void
caching_server_glStencilFuncSeparate (server_t *server, GLenum face, GLenum func,
                                       GLint ref, GLuint mask)
{
    egl_state_t *egl_state;
    bool needs_call = false;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glStencilFuncSeparate) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        if (! (face == GL_FRONT         ||
               face == GL_BACK          ||
               face == GL_FRONT_AND_BACK)) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
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
            caching_server_glSetError (server, GL_INVALID_ENUM);
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
        CACHING_SERVER(server)->super_dispatch.glStencilFuncSeparate (server, face, func, ref, mask);
}

static void
caching_server_glStencilFunc (server_t *server, GLenum func, GLint ref, GLuint mask)
{
    caching_server_glStencilFuncSeparate (server, GL_FRONT_AND_BACK, func, ref, mask);
}

static void
caching_server_glStencilMaskSeparate (server_t *server, GLenum face, GLuint mask)
{
    egl_state_t *egl_state;
    bool needs_call = false;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glStencilMaskSeparate) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        if (! (face == GL_FRONT         ||
               face == GL_BACK          ||
               face == GL_FRONT_AND_BACK)) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
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
        CACHING_SERVER(server)->super_dispatch.glStencilMaskSeparate (server, face, mask);
}

static void
caching_server_glStencilMask (server_t *server, GLuint mask)
{
    caching_server_glStencilMaskSeparate (server, GL_FRONT_AND_BACK, mask);
}

static void
caching_server_glStencilOpSeparate (server_t *server, GLenum face, GLenum sfail, 
                                     GLenum dpfail, GLenum dppass)
{
    egl_state_t *egl_state;
    bool needs_call = false;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glStencilOpSeparate) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        if (! (face == GL_FRONT         ||
               face == GL_BACK          ||
               face == GL_FRONT_AND_BACK)) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
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
            caching_server_glSetError (server, GL_INVALID_ENUM);
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
            caching_server_glSetError (server, GL_INVALID_ENUM);
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
            caching_server_glSetError (server, GL_INVALID_ENUM);
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
        CACHING_SERVER(server)->super_dispatch.glStencilOpSeparate (server, face, sfail, dpfail, dppass);
}

static void
caching_server_glStencilOp (server_t *server, GLenum sfail, GLenum dpfail, GLenum dppass)
{
    caching_server_glStencilOpSeparate (server, GL_FRONT_AND_BACK, sfail, dpfail, dppass);
}

static void
caching_server_glTexImage2D (server_t *server, GLenum target, GLint level, 
                              GLint internalformat,
                              GLsizei width, GLsizei height, GLint border,
                              GLenum format, GLenum type, 
                              const GLvoid *data)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glTexImage2D) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glTexImage2D (server, target, level, internalformat, width, height,
                             border, format, type, data);
        egl_state->state.need_get_error = true;
    }
    if (data)
      free ((void *)data);
}

static void
caching_server_glTexParameteri (server_t *server, GLenum target, GLenum pname, GLint param)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    int active_texture_index;
    int target_index;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glTexParameteri) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        state = &egl_state->state;
        active_texture_index = state->active_texture - GL_TEXTURE0;

        if (! (target == GL_TEXTURE_2D       || 
               target == GL_TEXTURE_CUBE_MAP
                                             || 
               target == GL_TEXTURE_3D_OES
                                          )) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
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
        else if (pname == GL_TEXTURE_WRAP_R_OES) {
            if (state->texture_3d_wrap_r[active_texture_index] != param) {
                state->texture_3d_wrap_r[active_texture_index] = param;
            }
            else
                return;
        }

        if (! (pname == GL_TEXTURE_MAG_FILTER ||
               pname == GL_TEXTURE_MIN_FILTER ||
               pname == GL_TEXTURE_WRAP_S     ||
               pname == GL_TEXTURE_WRAP_T
                                              || 
               pname == GL_TEXTURE_WRAP_R_OES
                                             )) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        if (pname == GL_TEXTURE_MAG_FILTER &&
            ! (param == GL_NEAREST ||
               param == GL_LINEAR ||
               param == GL_NEAREST_MIPMAP_NEAREST ||
               param == GL_LINEAR_MIPMAP_NEAREST  ||
               param == GL_NEAREST_MIPMAP_LINEAR  ||
               param == GL_LINEAR_MIPMAP_LINEAR)) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }
        else if (pname == GL_TEXTURE_MIN_FILTER &&
                 ! (param == GL_NEAREST ||
                    param == GL_LINEAR)) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }
        else if ((pname == GL_TEXTURE_WRAP_S ||
                  pname == GL_TEXTURE_WRAP_T
                                             || 
                  pname == GL_TEXTURE_WRAP_R_OES
                                                ) &&
                 ! (param == GL_CLAMP_TO_EDGE   ||
                    param == GL_MIRRORED_REPEAT ||
                    param == GL_REPEAT)) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glTexParameteri (server, target, pname, param);
    }
}

static void
caching_server_glTexParameterf (server_t *server, GLenum target, GLenum pname, GLfloat param)
{
    GLint parami = param;
    caching_server_glTexParameteri (server, target, pname, parami);
}

static void
caching_server_glTexSubImage2D (server_t *server, GLenum target, GLint level,
                                  GLint xoffset, GLint yoffset,
                                  GLsizei width, GLsizei height,
                                  GLenum format, GLenum type, 
                                  const GLvoid *data)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glTexSubImage2D) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glTexSubImage2D (server, target, level, xoffset, yoffset,
                                width, height, format, type, data);
        egl_state->state.need_get_error = true;
    }
    if (data)
        free ((void *)data);
}

void
caching_server_glUniformfv (server_t *server, int i, GLint location,
                GLsizei count, const GLfloat *value)
{
    egl_state_t *egl_state;

    if (! caching_server_glIsValidContext (server))
        goto FINISH;

    switch (i) {
    case 1:
        if(! caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glUniform1fv))
            goto FINISH;
        CACHING_SERVER(server)->super_dispatch.glUniform1fv (server, location, count, value);
        break;
    case 2:
        if(! caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glUniform2fv))
            goto FINISH;
        CACHING_SERVER(server)->super_dispatch.glUniform2fv (server, location, count, value);
        break;
    case 3:
        if(! caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glUniform3fv))
            goto FINISH;
        CACHING_SERVER(server)->super_dispatch.glUniform3fv (server, location, count, value);
        break;
    default:
        if(! caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glUniform4fv))
            goto FINISH;
        CACHING_SERVER(server)->super_dispatch.glUniform4fv (server, location, count, value);
        break;
    }

    egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    egl_state->state.need_get_error = true;

FINISH:
    if (value)
        free ((GLfloat *)value);
}

static void
caching_server_glUniform1fv (server_t *server, GLint location, GLsizei count, 
                            const GLfloat *value)
{
    caching_server_glUniformfv (server, 1, location, count, value);
}

static void
caching_server_glUniform2fv (server_t *server, GLint location, GLsizei count, 
                            const GLfloat *value)
{
    caching_server_glUniformfv (server, 2, location, count, value);
}

static void
caching_server_glUniform3fv (server_t *server, GLint location, GLsizei count, 
                            const GLfloat *value)
{
    caching_server_glUniformfv (server, 3, location, count, value);
}

static void
caching_server_glUniform4fv (server_t *server, GLint location, GLsizei count, 
                            const GLfloat *value)
{
    caching_server_glUniformfv (server, 4, location, count, value);
}

 void
caching_server_glUniformiv (server_t *server,
                int i,
                GLint location,
                GLsizei count,
                const GLint *value)
{
    egl_state_t *egl_state;

    if (! caching_server_glIsValidContext (server))
        goto FINISH;

    switch (i) {
    case 1:
        if(! caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glUniform1iv))
            goto FINISH;
        CACHING_SERVER(server)->super_dispatch.glUniform1iv (server, location, count, value);
        break;
    case 2:
        if(! caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glUniform2iv))
            goto FINISH;
        CACHING_SERVER(server)->super_dispatch.glUniform2iv (server, location, count, value);
        break;
    case 3:
        if(! caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glUniform3iv))
            goto FINISH;
        CACHING_SERVER(server)->super_dispatch.glUniform3iv (server, location, count, value);
        break;
    default:
        if(! caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glUniform4iv))
            goto FINISH;
        CACHING_SERVER(server)->super_dispatch.glUniform4iv (server, location, count, value);
        break;
    }

    egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    egl_state->state.need_get_error = true;

FINISH:
    if (value)
        free ((GLint *)value);
}

static void
caching_server_glUniform1iv (server_t *server, GLint location, GLsizei count, 
                            const GLint *value)
{
    caching_server_glUniformiv (server, 1, location, count, value);
}

static void
caching_server_glUniform2iv (server_t *server, GLint location, GLsizei count, 
                            const GLint *value)
{
    caching_server_glUniformiv (server, 2, location, count, value);
}

static void
caching_server_glUniform3iv (server_t *server, GLint location, GLsizei count, 
                            const GLint *value)
{
    caching_server_glUniformiv (server, 3, location, count, value);
}

static void
caching_server_glUniform4iv (server_t *server, GLint location, GLsizei count, 
                            const GLint *value)
{
    caching_server_glUniformiv (server, 4, location, count, value);
}

static void
caching_server_glUniformMatrix2fv (server_t *server, GLint location,
				   GLsizei count, GLboolean transpose,
				   const GLfloat *value)
{
    egl_state_t *egl_state;

    if (! caching_server_glIsValidContext (server))
        goto FINISH;

    egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    if (count < 0) {
        caching_server_glSetError (server, GL_INVALID_VALUE);
        goto FINISH;
    }

    CACHING_SERVER(server)->super_dispatch.glUniformMatrix2fv (server, 
                                                               location,
                                                               count,
                                                               transpose,
                                                               value);
    egl_state->state.need_get_error = true;

FINISH:
    if (value)
        free ((GLfloat *)value);
}

static void
caching_server_glUniformMatrix3fv (server_t *server, GLint location,
				   GLsizei count, GLboolean transpose,
				   const GLfloat *value)
{
    egl_state_t *egl_state;

    if (! caching_server_glIsValidContext (server))
        goto FINISH;

    egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    if (count < 0) {
        caching_server_glSetError (server, GL_INVALID_VALUE);
        goto FINISH;
    }

    CACHING_SERVER(server)->super_dispatch.glUniformMatrix3fv (server, 
                                                               location,
                                                               count,
                                                               transpose,
                                                               value);
    egl_state->state.need_get_error = true;

FINISH:
    if (value)
        free ((GLfloat *)value);
}

static void
caching_server_glUniformMatrix4fv (server_t *server, GLint location,
				   GLsizei count, GLboolean transpose,
				   const GLfloat *value)
{
    egl_state_t *egl_state;

    if (! caching_server_glIsValidContext (server))
        goto FINISH;

    egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    if (count < 0) {
        caching_server_glSetError (server, GL_INVALID_VALUE);
        goto FINISH;
    }

    CACHING_SERVER(server)->super_dispatch.glUniformMatrix4fv (server, 
                                                               location,
                                                               count,
                                                               transpose,
                                                               value);
    egl_state->state.need_get_error = true;

FINISH:
    if (value)
        free ((GLfloat *)value);
}

static void
caching_server_glUseProgram (server_t *server, GLuint program)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glUseProgram) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        if (egl_state->state.current_program == program)
            return;

        CACHING_SERVER(server)->super_dispatch.glUseProgram (server, program);
        /* FIXME: this maybe not right because this program may be invalid
         * object, we save here to save time in glGetError() */
        egl_state->state.current_program = program;
        /* FIXME: do we need to have this ? */
        // egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glVertexAttrib1f (server_t *server, GLuint index, GLfloat v0)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glVertexAttrib1f) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        if (caching_server_glIndexIsTooLarge (server, &egl_state->state, index)) {
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glVertexAttrib1f (server, index, v0);
    }
}

static void
caching_server_glVertexAttrib2f (server_t *server, GLuint index, GLfloat v0, GLfloat v1)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glVertexAttrib2f) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        if (caching_server_glIndexIsTooLarge (server, &egl_state->state, index)) {
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glVertexAttrib2f (server, index, v0, v1);
    }
}

static void
caching_server_glVertexAttrib3f (server_t *server, GLuint index, GLfloat v0, 
                                 GLfloat v1, GLfloat v2)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glVertexAttrib3f) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        if (caching_server_glIndexIsTooLarge (server, &egl_state->state, index)) {
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glVertexAttrib3f (server, index, v0, v1, v2);
    }
}

static void
caching_server_glVertexAttrib4f (server_t *server, GLuint index, GLfloat v0, GLfloat v1, 
                                 GLfloat v2, GLfloat v3)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glVertexAttrib3f) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        if (caching_server_glIndexIsTooLarge (server, &egl_state->state, index))
            return;

        CACHING_SERVER(server)->super_dispatch.glVertexAttrib4f (server, index, v0, v1, v2, v3);
    }
}

 void
caching_server_glVertexAttribfv (server_t *server, int i, GLuint index, const GLfloat *v)
{
    egl_state_t *egl_state;
    
    if (! caching_server_glIsValidContext (server))
        goto FINISH;
    
    egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

    if (caching_server_glIndexIsTooLarge (server, &egl_state->state, index))
        goto FINISH;

    switch (i) {
        case 1:
            if(! caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glVertexAttrib1fv))
                goto FINISH;
            CACHING_SERVER(server)->super_dispatch.glVertexAttrib1fv (server, index, v);
            break;
        case 2:
            if(! caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glVertexAttrib2fv))
                goto FINISH;
            CACHING_SERVER(server)->super_dispatch.glVertexAttrib2fv (server, index, v);
            break;
        case 3:
            if(! caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glVertexAttrib3fv))
                goto FINISH;
            CACHING_SERVER(server)->super_dispatch.glVertexAttrib3fv (server, index, v);
            break;
        default:
            if(! caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glVertexAttrib4fv))
                goto FINISH;
            CACHING_SERVER(server)->super_dispatch.glVertexAttrib4fv (server, index, v);
            break;
    }

FINISH:
    if (v)
        free ((GLfloat *)v);
}

static void
caching_server_glVertexAttrib1fv (server_t *server, GLuint index, const GLfloat *v)
{
    caching_server_glVertexAttribfv (server, 1, index, v);
}

static void
caching_server_glVertexAttrib2fv (server_t *server, GLuint index, const GLfloat *v)
{
    caching_server_glVertexAttribfv (server, 2, index, v);
}

static void
caching_server_glVertexAttrib3fv (server_t *server, GLuint index, const GLfloat *v)
{
    caching_server_glVertexAttribfv (server, 3, index, v);
}

static void
caching_server_glVertexAttrib4fv (server_t *server, GLuint index, const GLfloat *v)
{
    caching_server_glVertexAttribfv (server, 4, index, v);
}

static void
caching_server_glVertexAttribPointer (server_t *server, GLuint index, GLint size,
                                      GLenum type, GLboolean normalized,
                                      GLsizei stride, const GLvoid *pointer)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int i, found_index = -1;
    int count;
    GLint bound_buffer = 0;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glVertexAttribPointer) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        state = &egl_state->state;
        attrib_list = &state->vertex_attribs;
        attribs = attrib_list->attribs;
        count = attrib_list->count;
        
        if (! (type == GL_BYTE                 ||
               type == GL_UNSIGNED_BYTE        ||
               type == GL_SHORT                ||
               type == GL_UNSIGNED_SHORT       ||
               type == GL_FLOAT)) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }
        else if (size > 4 || size < 1 || stride < 0) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        /* check max_vertex_attribs */
        if (caching_server_glIndexIsTooLarge (server, &egl_state->state, index)) {
            return;
        }
        
        if (egl_state->state.vertex_array_binding) {
            CACHING_SERVER(server)->super_dispatch.glVertexAttribPointer (server, index, size, type, normalized,
                                          stride, pointer);
            //egl_state->state.need_get_error = true;
            return;
        }
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
            CACHING_SERVER(server)->super_dispatch.glVertexAttribPointer (server, index, size, type, normalized,
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

static void
caching_server_glViewport (server_t *server, GLint x, GLint y, GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glViewport) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        if (egl_state->state.viewport[0] == x     &&
            egl_state->state.viewport[1] == y     &&
            egl_state->state.viewport[2] == width &&
            egl_state->state.viewport[3] == height)
            return;

        if (x < 0 || y < 0) {
            caching_server_glSetError (server, GL_INVALID_VALUE);
            return;
        }

        egl_state->state.viewport[0] = x;
        egl_state->state.viewport[1] = y;
        egl_state->state.viewport[2] = width;
        egl_state->state.viewport[3] = height;

        CACHING_SERVER(server)->super_dispatch.glViewport (server, x, y, width, height);
    }
}
/* end of GLES2 core profile */

static void
caching_server_glEGLImageTargetTexture2DOES (server_t *server, GLenum target, GLeglImageOES image)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glEGLImageTargetTexture2DOES) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        if (target != GL_TEXTURE_2D) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glEGLImageTargetTexture2DOES (server, target, image);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glGetProgramBinaryOES (server_t *server, GLuint program, GLsizei bufSize, 
                            GLsizei *length, GLenum *binaryFormat, 
                            GLvoid *binary)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetProgramBinaryOES) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glGetProgramBinaryOES (server, program, bufSize, length, 
                                      binaryFormat, binary);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glProgramBinaryOES (server_t *server, GLuint program, GLenum binaryFormat,
                        const GLvoid *binary, GLint length)
{
    egl_state_t *egl_state;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glProgramBinaryOES) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glProgramBinaryOES (server, program, binaryFormat, binary, length);
        egl_state->state.need_get_error = true;
    }

    if (binary)
        free ((void *)binary);
}

static void* 
caching_server_glMapBufferOES (server_t *server, GLenum target, GLenum access)
{
    egl_state_t *egl_state;
    void *result = NULL;
    
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glMapBufferOES) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (access != GL_WRITE_ONLY_OES) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return result;
        }
        else if (! (target == GL_ARRAY_BUFFER ||
                    target == GL_ELEMENT_ARRAY_BUFFER)) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return result;
        }

        result = CACHING_SERVER(server)->super_dispatch.glMapBufferOES (server, target, access);
        egl_state->state.need_get_error = true;
    }
    return result;
}

static GLboolean
caching_server_glUnmapBufferOES (server_t *server, GLenum target)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glUnmapBufferOES) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (! (target == GL_ARRAY_BUFFER ||
               target == GL_ELEMENT_ARRAY_BUFFER)) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return result;
        }

        result = CACHING_SERVER(server)->super_dispatch.glUnmapBufferOES (server, target);
        egl_state->state.need_get_error = true;
    }
    return result;
}

static void
caching_server_glGetBufferPointervOES (server_t *server, GLenum target, GLenum pname, GLvoid **params)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetBufferPointervOES) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (! (target == GL_ARRAY_BUFFER ||
               target == GL_ELEMENT_ARRAY_BUFFER)) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glGetBufferPointervOES (server, target, pname, params);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glTexImage3DOES (server_t *server, GLenum target, GLint level, GLenum internalformat,
                      GLsizei width, GLsizei height, GLsizei depth,
                      GLint border, GLenum format, GLenum type,
                      const GLvoid *pixels)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glTexImage3DOES) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        CACHING_SERVER(server)->super_dispatch.glTexImage3DOES (server, target, level, internalformat, 
                                width, height, depth, 
                                border, format, type, pixels);
        egl_state->state.need_get_error = true;
    }
    if (pixels)
        free ((void *)pixels);
}

static void
caching_server_glTexSubImage3DOES (server_t *server, GLenum target, GLint level,
                          GLint xoffset, GLint yoffset, GLint zoffset,
                          GLsizei width, GLsizei height, GLsizei depth,
                          GLenum format, GLenum type, const GLvoid *data)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glTexSubImage3DOES) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        CACHING_SERVER(server)->super_dispatch.glTexSubImage3DOES (server, target, level, 
                                   xoffset, yoffset, zoffset,
                                   width, height, depth, 
                                   format, type, data);
        egl_state->state.need_get_error = true;
    }
    if (data)
        free ((void *)data);
}

static void
caching_server_glCompressedTexImage3DOES (server_t *server, GLenum target, GLint level,
                                 GLenum internalformat,
                                 GLsizei width, GLsizei height, 
                                 GLsizei depth,
                                 GLint border, GLsizei imageSize,
                                 const GLvoid *data)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glCompressedTexImage3DOES) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        CACHING_SERVER(server)->super_dispatch.glCompressedTexImage3DOES (server, target, level, internalformat,
                                          width, height, depth,
                                          border, imageSize, data);

        egl_state->state.need_get_error = true;
    }
    if (data)
        free ((void *)data);
}

static void
caching_server_glCompressedTexSubImage3DOES (server_t *server, GLenum target, GLint level,
                                     GLint xoffset, GLint yoffset, 
                                     GLint zoffset,
                                     GLsizei width, GLsizei height, 
                                     GLsizei depth,
                                     GLenum format, GLsizei imageSize,
                                     const GLvoid *data)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glCompressedTexSubImage3DOES) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        CACHING_SERVER(server)->super_dispatch.glCompressedTexSubImage3DOES (server, target, level,
                                             xoffset, yoffset, zoffset,
                                             width, height, depth,
                                             format, imageSize, data);

        egl_state->state.need_get_error = true;
    }

    if (data)
        free ((void *)data);
}

static void
caching_server_glFramebufferTexture3DOES (server_t *server, GLenum target, GLenum attachment,
                                GLenum textarget, GLuint texture,
                                GLint level, GLint zoffset)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glFramebufferTexture3DOES) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (target != GL_FRAMEBUFFER) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glFramebufferTexture3DOES (server, target, attachment, textarget,
                                          texture, level, zoffset);
        egl_state->state.need_get_error = true;
    }
}

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
caching_server_glBindVertexArrayOES (server_t *server, GLuint array)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glBindVertexArrayOES) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

        if (egl_state->state.vertex_array_binding == array)
            return;

        CACHING_SERVER(server)->super_dispatch.glBindVertexArrayOES (server, array);
        egl_state->state.need_get_error = true;
        /* FIXME: should we save this ? */
        egl_state->state.vertex_array_binding = array;
    }
}

static void
caching_server_glDeleteVertexArraysOES (server_t *server, GLsizei n, const GLuint *arrays)
{
    egl_state_t *egl_state;
    int i;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glDeleteVertexArraysOES) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        
        if (n <= 0) {
            caching_server_glSetError (server, GL_INVALID_VALUE);
            goto FINISH;
        }

        CACHING_SERVER(server)->super_dispatch.glDeleteVertexArraysOES (server, n, arrays);

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
caching_server_glGenVertexArraysOES (server_t *server, GLsizei n, GLuint *arrays)
{

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGenVertexArraysOES) &&
        caching_server_glIsValidContext (server)) {
        if (n <= 0) {
            caching_server_glSetError (server, GL_INVALID_VALUE);
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glGenVertexArraysOES (server, n, arrays);
    }
}

static GLboolean
caching_server_glIsVertexArrayOES (server_t *server, GLuint array)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glIsVertexArrayOES) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        
        result = CACHING_SERVER(server)->super_dispatch.glIsVertexArrayOES (server, array);

        if (result == GL_FALSE && 
            egl_state->state.vertex_array_binding == array)
            egl_state->state.vertex_array_binding = 0;
    }
    return result;
}

static void
caching_server_glGetPerfMonitorGroupsAMD (server_t *server, GLint *numGroups, GLsizei groupSize, 
                                 GLuint *groups)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetPerfMonitorGroupsAMD) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glGetPerfMonitorGroupsAMD (server, numGroups, groupSize, groups);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glGetPerfMonitorCountersAMD (server_t *server, GLuint group, GLint *numCounters, 
                                   GLint *maxActiveCounters, 
                                   GLsizei counterSize,
                                   GLuint *counters)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetPerfMonitorCountersAMD) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glGetPerfMonitorCountersAMD (server, group, numCounters,
                                            maxActiveCounters,
                                            counterSize, counters);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glGetPerfMonitorGroupStringAMD (server_t *server, GLuint group, GLsizei bufSize, 
                                       GLsizei *length, 
                                       GLchar *groupString)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetPerfMonitorGroupStringAMD) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glGetPerfMonitorGroupStringAMD (server, group, bufSize, length,
                                               groupString);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glGetPerfMonitorCounterStringAMD (server_t *server, GLuint group, GLuint counter, 
                                         GLsizei bufSize, 
                                         GLsizei *length, 
                                         GLchar *counterString)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetPerfMonitorCounterStringAMD) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glGetPerfMonitorCounterStringAMD (server, group, counter, bufSize, 
                                                 length, counterString);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glGetPerfMonitorCounterInfoAMD (server_t *server, GLuint group, GLuint counter, 
                                       GLenum pname, GLvoid *data)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetPerfMonitorCounterInfoAMD) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glGetPerfMonitorCounterInfoAMD (server, group, counter, 
                                               pname, data); 
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glGenPerfMonitorsAMD (server_t *server, GLsizei n, GLuint *monitors) 
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGenPerfMonitorsAMD) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glGenPerfMonitorsAMD (server, n, monitors); 
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glDeletePerfMonitorsAMD (server_t *server, GLsizei n, GLuint *monitors) 
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glDeletePerfMonitorsAMD) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glDeletePerfMonitorsAMD (server, n, monitors); 
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glSelectPerfMonitorCountersAMD (server_t *server, GLuint monitor, GLboolean enable,
                                      GLuint group, GLint numCounters,
                                      GLuint *countersList) 
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glSelectPerfMonitorCountersAMD) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glSelectPerfMonitorCountersAMD (server, monitor, enable, group,
                                               numCounters, countersList); 
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glGetPerfMonitorCounterDataAMD (server_t *server, GLuint monitor, GLenum pname,
                                       GLsizei dataSize, GLuint *data,
                                       GLint *bytesWritten)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetPerfMonitorCounterDataAMD) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glGetPerfMonitorCounterDataAMD (server, monitor, pname, dataSize,
                                               data, bytesWritten); 
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glDiscardFramebufferEXT (server_t *server,
                             GLenum target, GLsizei numAttachments,
                             const GLenum *attachments)
{
    int i;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glDiscardFramebufferEXT) &&
        caching_server_glIsValidContext (server)) {
        
        if (target != GL_FRAMEBUFFER) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            goto FINISH;
        }

        for (i = 0; i < numAttachments; i++) {
            if (! (attachments[i] == GL_COLOR_ATTACHMENT0  ||
                   attachments[i] == GL_DEPTH_ATTACHMENT   ||
                   attachments[i] == GL_STENCIL_ATTACHMENT ||
                   attachments[i] == GL_COLOR_EXT          ||
                   attachments[i] == GL_DEPTH_EXT          ||
                   attachments[i] == GL_STENCIL_EXT)) {
                caching_server_glSetError (server, GL_INVALID_ENUM);
                goto FINISH;
            }
        }
        CACHING_SERVER(server)->super_dispatch.glDiscardFramebufferEXT (server, target, numAttachments, 
                                        attachments);
    }
FINISH:
    if (attachments)
        free ((void *)attachments);
}

static void
caching_server_glMultiDrawArraysEXT (server_t *server,
                                     GLenum mode,
                                     GLint *first,
                                     GLsizei *count,
                                     GLsizei primcount)
{
    /* not implemented */
}

void 
caching_server_glMultiDrawElementsEXT (server_t *server, GLenum mode, const GLsizei *count, GLenum type,
                             const GLvoid **indices, GLsizei primcount)
{
    /* not implemented */
}

static void
caching_server_glFramebufferTexture2DMultisampleEXT (server_t *server, GLenum target, 
                                            GLenum attachment,
                                            GLenum textarget, 
                                            GLuint texture,
                                            GLint level, GLsizei samples)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glFramebufferTexture2DMultisampleEXT) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        
        if (target != GL_FRAMEBUFFER) {
           caching_server_glSetError (server, GL_INVALID_ENUM);
           return;
        }

        CACHING_SERVER(server)->super_dispatch.glFramebufferTexture2DMultisampleEXT (server, target, attachment, 
                                                     textarget, texture, 
                                                     level, samples);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glFramebufferTexture2DMultisampleIMG (server_t *server, GLenum target, 
                                                     GLenum attachment,
                                                     GLenum textarget, 
                                                     GLuint texture,
                                                     GLint level, GLsizei samples)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glFramebufferTexture2DMultisampleIMG) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        
        if (target != GL_FRAMEBUFFER) {
           caching_server_glSetError (server, GL_INVALID_ENUM);
           return;
        }

        CACHING_SERVER(server)->super_dispatch.glFramebufferTexture2DMultisampleIMG (server, target, attachment,
                                                     textarget, texture,
                                                     level, samples);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glDeleteFencesNV (server_t *server, GLsizei n, const GLuint *fences)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glDeleteFencesNV) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        if (n <= 0) {
            caching_server_glSetError (server, GL_INVALID_VALUE);
            goto FINISH;
        }
    
        CACHING_SERVER(server)->super_dispatch.glDeleteFencesNV (server, n, fences); 
        egl_state->state.need_get_error = true;
    }
FINISH:
    if (fences)
        free ((void *)fences);
}

static void
caching_server_glGenFencesNV (server_t *server, GLsizei n, GLuint *fences)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGenFencesNV) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        if (n <= 0) {
            caching_server_glSetError (server, GL_INVALID_VALUE);
            return;
        }
    
        CACHING_SERVER(server)->super_dispatch.glGenFencesNV (server, n, fences); 
        egl_state->state.need_get_error = true;
    }
}

static GLboolean
caching_server_glIsFenceNV (server_t *server, GLuint fence)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glIsFenceNV) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        result = CACHING_SERVER(server)->super_dispatch.glIsFenceNV (server, fence);
        egl_state->state.need_get_error = true;
    }
    return result;
}

static GLboolean
caching_server_glTestFenceNV (server_t *server, GLuint fence)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glTestFenceNV) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        result = CACHING_SERVER(server)->super_dispatch.glTestFenceNV (server, fence);
        egl_state->state.need_get_error = true;
    }
    return result;
}

static void 
caching_server_glGetFenceivNV (server_t *server, GLuint fence, GLenum pname, int *params)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetFenceivNV) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glGetFenceivNV (server, fence, pname, params);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glSetFenceNV (server_t *server, GLuint fence, GLenum condition)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glSetFenceNV) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glSetFenceNV (server, fence, condition);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glCoverageMaskNV (server_t *server, GLboolean mask)
{

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glCoverageMaskNV) &&
        caching_server_glIsValidContext (server)) {
    
        CACHING_SERVER(server)->super_dispatch.glCoverageMaskNV (server, mask);
    }
}

static void
caching_server_glCoverageOperationNV (server_t *server, GLenum operation)
{
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glCoverageOperationNV) &&
        caching_server_glIsValidContext (server)) {
    
        if (! (operation == GL_COVERAGE_ALL_FRAGMENTS_NV  ||
               operation == GL_COVERAGE_EDGE_FRAGMENTS_NV ||
               operation == GL_COVERAGE_AUTOMATIC_NV)) {
            caching_server_glSetError (server, GL_INVALID_ENUM);
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glCoverageOperationNV (server, operation);
    }
}

static void
caching_server_glGetDriverControlsQCOM (server_t *server, GLint *num, GLsizei size, 
                              GLuint *driverControls)
{
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetDriverControlsQCOM) &&
        caching_server_glIsValidContext (server)) {
    
        CACHING_SERVER(server)->super_dispatch.glGetDriverControlsQCOM (server, num, size, driverControls);
    }
}

static void
caching_server_glGetDriverControlStringQCOM (server_t *server, GLuint driverControl, GLsizei bufSize,
                                    GLsizei *length, 
                                    GLchar *driverControlString)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glGetDriverControlStringQCOM) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glGetDriverControlStringQCOM (server, driverControl, bufSize,
                                             length, driverControlString);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glExtGetTexturesQCOM (server_t *server, GLuint *textures, GLint maxTextures, 
                           GLint *numTextures)
{
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glExtGetTexturesQCOM) &&
        caching_server_glIsValidContext (server)) {
    
        CACHING_SERVER(server)->super_dispatch.glExtGetTexturesQCOM (server, textures, maxTextures, numTextures);
    }
}

static void
caching_server_glExtGetBuffersQCOM (server_t *server, GLuint *buffers, GLint maxBuffers, 
                          GLint *numBuffers) 
{
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glExtGetBuffersQCOM) &&
        caching_server_glIsValidContext (server)) {
    
        CACHING_SERVER(server)->super_dispatch.glExtGetBuffersQCOM (server, buffers, maxBuffers, numBuffers);
    }
}

static void
caching_server_glExtGetRenderbuffersQCOM (server_t *server, GLuint *renderbuffers, 
                                GLint maxRenderbuffers,
                                GLint *numRenderbuffers)
{
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glExtGetRenderbuffersQCOM) &&
        caching_server_glIsValidContext (server)) {
    
        CACHING_SERVER(server)->super_dispatch.glExtGetRenderbuffersQCOM (server, renderbuffers, maxRenderbuffers,
                                          numRenderbuffers);
    }
}

static void
caching_server_glExtGetFramebuffersQCOM (server_t *server, GLuint *framebuffers, GLint maxFramebuffers,
                               GLint *numFramebuffers)
{
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glExtGetFramebuffersQCOM) &&
        caching_server_glIsValidContext (server)) {
    
        CACHING_SERVER(server)->super_dispatch.glExtGetFramebuffersQCOM (server, framebuffers, maxFramebuffers,
                                         numFramebuffers);
    }
}

static void
caching_server_glExtGetTexLevelParameterivQCOM (server_t *server, GLuint texture, GLenum face, 
                                        GLint level, GLenum pname, 
                                        GLint *params)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glExtGetTexLevelParameterivQCOM) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glExtGetTexLevelParameterivQCOM (server, texture, face, level,
                                                pname, params);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glExtGetTexSubImageQCOM (server_t *server, GLenum target, GLint level,
                                GLint xoffset, GLint yoffset, 
                                GLint zoffset,
                                GLsizei width, GLsizei height, 
                                GLsizei depth,
                                GLenum format, GLenum type, GLvoid *texels)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glExtGetTexSubImageQCOM) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glExtGetTexSubImageQCOM (server, target, level,
                                        xoffset, yoffset, zoffset,
                                        width, height, depth,
                                        format, type, texels);
    egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glExtGetBufferPointervQCOM (server_t *server, GLenum target, GLvoid **params)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glExtGetBufferPointervQCOM) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glExtGetBufferPointervQCOM (server, target, params);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glExtGetShadersQCOM (server_t *server, GLuint *shaders, GLint maxShaders, 
                          GLint *numShaders)
{
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glExtGetShadersQCOM) &&
        caching_server_glIsValidContext (server)) {
    
        CACHING_SERVER(server)->super_dispatch.glExtGetShadersQCOM (server, shaders, maxShaders, numShaders);
    }
}

static void
caching_server_glExtGetProgramsQCOM (server_t *server, GLuint *programs, GLint maxPrograms,
                           GLint *numPrograms)
{
    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glExtGetProgramsQCOM) &&
        caching_server_glIsValidContext (server)) {
    
        CACHING_SERVER(server)->super_dispatch.glExtGetProgramsQCOM (server, programs, maxPrograms, numPrograms);
    }
}

static GLboolean
caching_server_glExtIsProgramBinaryQCOM (server_t *server, GLuint program)
{
    egl_state_t *egl_state;
    bool result = false;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glExtIsProgramBinaryQCOM) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        result = CACHING_SERVER(server)->super_dispatch.glExtIsProgramBinaryQCOM (server, program);
        egl_state->state.need_get_error = true;
    }
    return result;
}

static void
caching_server_glExtGetProgramBinarySourceQCOM (server_t *server, GLuint program, GLenum shadertype,
                                        GLchar *source, GLint *length)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glExtGetProgramBinarySourceQCOM) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glExtGetProgramBinarySourceQCOM (server, program, shadertype,
                                                source, length);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glStartTilingQCOM (server_t *server, GLuint x, GLuint y, GLuint width, GLuint height,
                       GLbitfield preserveMask)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glStartTilingQCOM) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glStartTilingQCOM (server, x, y, width, height, preserveMask);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_server_glEndTilingQCOM (server_t *server, GLbitfield preserveMask)
{
    egl_state_t *egl_state;

    if (caching_server_glIsValidFunc (server, CACHING_SERVER(server)->super_dispatch.glEndTilingQCOM) &&
        caching_server_glIsValidContext (server)) {
        egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glEndTilingQCOM (server, preserveMask);
        egl_state->state.need_get_error = true;
    }
}

 EGLint
caching_server_eglGetError (server_t *server)
{
    EGLint error = EGL_NOT_INITIALIZED;

    if (! CACHING_SERVER(server)->super_dispatch.eglGetError)
        return error;

    error = CACHING_SERVER(server)->super_dispatch.eglGetError(server);

    return error;
}

static EGLDisplay
caching_server_eglGetDisplay (server_t *server,
                  EGLNativeDisplayType display_id)
{
    EGLDisplay display = EGL_NO_DISPLAY;

    display = CACHING_SERVER(server)->super_dispatch.eglGetDisplay (server, display_id);

    return display;
}

static EGLBoolean
caching_server_eglInitialize (server_t *server,
                 EGLDisplay display,
                 EGLint *major,
                 EGLint *minor)
{
    EGLBoolean result = EGL_FALSE;

    if (CACHING_SERVER(server)->super_dispatch.eglInitialize) 
        result = CACHING_SERVER(server)->super_dispatch.eglInitialize (server, display, major, minor);

    if (result == EGL_TRUE)
        _server_initialize (display);

    return result;
}

static EGLBoolean
caching_server_eglTerminate (server_t *server,
                EGLDisplay display)
{
    EGLBoolean result = EGL_FALSE;

    if (CACHING_SERVER(server)->super_dispatch.eglTerminate) {
        result = CACHING_SERVER(server)->super_dispatch.eglTerminate (server, display);

        if (result == EGL_TRUE) {
            /* XXX: remove srv structure */
            _server_terminate (server, display);
        }
    }

    return result;
}

static const char *
caching_server_eglQueryString (server_t *server,
                   EGLDisplay display,
                   EGLint name)
{
    const char *result = NULL;

    if (CACHING_SERVER(server)->super_dispatch.eglQueryString)
        result = CACHING_SERVER(server)->super_dispatch.eglQueryString (server, display, name);

    return result;
}

static EGLBoolean
caching_server_eglGetConfigs (server_t *server,
                  EGLDisplay display,
                  EGLConfig *configs,
                  EGLint config_size,
                  EGLint *num_config)
{
    EGLBoolean result = EGL_FALSE;

    if (CACHING_SERVER(server)->super_dispatch.eglGetConfigs)
        result = CACHING_SERVER(server)->super_dispatch.eglGetConfigs (server, display, configs, config_size, num_config);

    return result;
}

static EGLBoolean
caching_server_eglChooseConfig (server_t *server,
                    EGLDisplay display,
                    const EGLint *attrib_list,
                    EGLConfig *configs,
                    EGLint config_size,
                    EGLint *num_config)
{
    EGLBoolean result = EGL_FALSE;

    if (CACHING_SERVER(server)->super_dispatch.eglChooseConfig)
        result = CACHING_SERVER(server)->super_dispatch.eglChooseConfig (server, display, attrib_list, configs,
                                           config_size, num_config);

    return result;
}

static EGLBoolean
caching_server_eglGetConfigAttrib (server_t *server,
                        EGLDisplay display,
                        EGLConfig config,
                        EGLint attribute,
                        EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (CACHING_SERVER(server)->super_dispatch.eglGetConfigAttrib)
        result = CACHING_SERVER(server)->super_dispatch.eglGetConfigAttrib (server, display, config, attribute, value);

    return result;
}

static EGLSurface
caching_server_eglCreateWindowSurface (server_t *server,
                            EGLDisplay display,
                            EGLConfig config,
                            EGLNativeWindowType win,
                            const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    if (CACHING_SERVER(server)->super_dispatch.eglCreateWindowSurface)
        surface = CACHING_SERVER(server)->super_dispatch.eglCreateWindowSurface (server, display, config, win,
                                                   attrib_list);

    return surface;
}

static EGLSurface
caching_server_eglCreatePbufferSurface (server_t *server,
                             EGLDisplay display,
                             EGLConfig config,
                             const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    if (CACHING_SERVER(server)->super_dispatch.eglCreatePbufferSurface)
        surface = CACHING_SERVER(server)->super_dispatch.eglCreatePbufferSurface (server, display, config, attrib_list);

    return surface;
}

static EGLSurface
caching_server_eglCreatePixmapSurface (server_t *server,
                            EGLDisplay display,
                            EGLConfig config,
                            EGLNativePixmapType pixmap, 
                            const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    if (CACHING_SERVER(server)->super_dispatch.eglCreatePixmapSurface)
        surface = CACHING_SERVER(server)->super_dispatch.eglCreatePixmapSurface (server, display, config, pixmap,
                                                   attrib_list);

    return surface;
}

static EGLBoolean
caching_server_eglDestroySurface (server_t *server,
                                  EGLDisplay display,
                                  EGLSurface surface)
{
    EGLBoolean result = EGL_FALSE;

    if (!CACHING_SERVER(server)->active_state)
        return result;

    if (CACHING_SERVER(server)->super_dispatch.eglDestroySurface) { 
        result = CACHING_SERVER(server)->super_dispatch.eglDestroySurface (server, display, surface);

        if (result == EGL_TRUE) {
            /* update srv states */
            _server_destroy_surface (server, display, surface);
        }
    }

    return result;
}

static EGLBoolean
caching_server_eglQuerySurface (server_t *server,
                    EGLDisplay display,
                    EGLSurface surface,
                    EGLint attribute,
                    EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (CACHING_SERVER(server)->super_dispatch.eglQuerySurface) 
        result = CACHING_SERVER(server)->super_dispatch.eglQuerySurface (server, display, surface, attribute, value);

    return result;
}

static EGLBoolean
caching_server_eglBindAPI (server_t *server,
               EGLenum api)
{
    EGLBoolean result = EGL_FALSE;

    if (CACHING_SERVER(server)->super_dispatch.eglBindAPI) 
        result = CACHING_SERVER(server)->super_dispatch.eglBindAPI (server, api);

    return result;
}

static EGLenum 
caching_server_eglQueryAPI (server_t *server)
{
    EGLenum result = EGL_NONE;

    if (CACHING_SERVER(server)->super_dispatch.eglQueryAPI) 
        result = CACHING_SERVER(server)->super_dispatch.eglQueryAPI (server);

    return result;
}

static EGLBoolean
caching_server_eglWaitClient (server_t *server)
{
    EGLBoolean result = EGL_FALSE;

    if (CACHING_SERVER(server)->super_dispatch.eglWaitClient)
        result = CACHING_SERVER(server)->super_dispatch.eglWaitClient (server);

    return result;
}

static EGLBoolean
caching_server_eglReleaseThread (server_t *server)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t *egl_state;

    if (CACHING_SERVER(server)->super_dispatch.eglReleaseThread) {
        result = CACHING_SERVER(server)->super_dispatch.eglReleaseThread (server);

        if (result == EGL_TRUE) {
            if (! CACHING_SERVER(server)->active_state)
                return result;

            egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

            _server_make_current (server,
                                  egl_state->display,
                                  EGL_NO_SURFACE,
                                  EGL_NO_SURFACE,
                                  EGL_NO_CONTEXT);
        }
    }

    return result;
}

static EGLSurface
caching_server_eglCreatePbufferFromClientBuffer (server_t *server,
                                        EGLDisplay display,
                                        EGLenum buftype,
                                        EGLClientBuffer buffer,
                                        EGLConfig config,
                                        const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;
    
    if (CACHING_SERVER(server)->super_dispatch.eglCreatePbufferFromClientBuffer)
        surface = CACHING_SERVER(server)->super_dispatch.eglCreatePbufferFromClientBuffer (server, display, buftype,
                                                             buffer, config,
                                                             attrib_list);

    return surface;
}

static EGLBoolean
caching_server_eglSurfaceAttrib (server_t *server,
                     EGLDisplay display,
                     EGLSurface surface, 
                     EGLint attribute,
                     EGLint value)
{
    EGLBoolean result = EGL_FALSE;
    
    if (CACHING_SERVER(server)->super_dispatch.eglSurfaceAttrib)
        result = CACHING_SERVER(server)->super_dispatch.eglSurfaceAttrib (server, display, surface, attribute, value);

    return result;
}
    
static EGLBoolean
caching_server_eglBindTexImage (server_t *server,
                     EGLDisplay display,
                     EGLSurface surface,
                     EGLint buffer)
{
    EGLBoolean result = EGL_FALSE;
    
    if (CACHING_SERVER(server)->super_dispatch.eglBindTexImage)
        result = CACHING_SERVER(server)->super_dispatch.eglBindTexImage (server, display, surface, buffer);

    return result;
}

static EGLBoolean
caching_server_eglReleaseTexImage (server_t *server,
                        EGLDisplay display,
                        EGLSurface surface,
                        EGLint buffer)
{
    EGLBoolean result = EGL_FALSE;
    
    if (CACHING_SERVER(server)->super_dispatch.eglReleaseTexImage)
        result = CACHING_SERVER(server)->super_dispatch.eglReleaseTexImage (server, display, surface, buffer);

    return result;
}

static EGLBoolean
caching_server_eglSwapInterval (server_t *server,
                    EGLDisplay display,
                    EGLint interval)
{
    EGLBoolean result = EGL_FALSE;
    
    if (CACHING_SERVER(server)->super_dispatch.eglSwapInterval)
        result = CACHING_SERVER(server)->super_dispatch.eglSwapInterval (server, display, interval);

    return result;
}

static EGLContext
caching_server_eglCreateContext (server_t *server,
                     EGLDisplay display,
                     EGLConfig config,
                     EGLContext share_context,
                     const EGLint *attrib_list)
{
    EGLContext result = EGL_NO_CONTEXT;
    
    if (CACHING_SERVER(server)->super_dispatch.eglCreateContext)
        result = CACHING_SERVER(server)->super_dispatch.eglCreateContext (server, display, config, share_context, 
                                            attrib_list);

    return result;
}

static EGLBoolean
caching_server_eglDestroyContext (server_t *server,
                      EGLDisplay dpy,
                      EGLContext ctx)
{
    EGLBoolean result = GL_FALSE;

    if (CACHING_SERVER(server)->super_dispatch.eglDestroyContext) {
        result = CACHING_SERVER(server)->super_dispatch.eglDestroyContext (server, dpy, ctx); 

        if (result == GL_TRUE) {
            _server_destroy_context (dpy, ctx, CACHING_SERVER(server)->active_state);
        }
    }

    return result;
}

static EGLContext
caching_server_eglGetCurrentContext (server_t *server)
{
    return CACHING_SERVER(server)->super_dispatch.eglGetCurrentContext (server);
    egl_state_t *state;

    if (!CACHING_SERVER(server)->active_state)
        return EGL_NO_CONTEXT;

    state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    return state->context;
}

static EGLDisplay
caching_server_eglGetCurrentDisplay (server_t *server)
{
    return CACHING_SERVER(server)->super_dispatch.eglGetCurrentDisplay (server);
    egl_state_t *state;

    if (!CACHING_SERVER(server)->active_state)
        return EGL_NO_DISPLAY;

    state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
    return state->display;
}

static EGLSurface
caching_server_eglGetCurrentSurface (server_t *server,
                          EGLint readdraw)
{
    return CACHING_SERVER(server)->super_dispatch.eglGetCurrentSurface (server, readdraw);
    egl_state_t *state;
    EGLSurface surface = EGL_NO_SURFACE;

    if (!CACHING_SERVER(server)->active_state)
        return EGL_NO_SURFACE;

    state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;

    if (state->display == EGL_NO_DISPLAY || state->context == EGL_NO_CONTEXT)
        goto FINISH;

    if (readdraw == EGL_DRAW)
        surface = state->drawable;
    else
        surface = state->readable;
 FINISH:
    return surface;
}


static EGLBoolean
caching_server_eglQueryContext (server_t *server,
                    EGLDisplay display,
                    EGLContext ctx,
                    EGLint attribute,
                    EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglQueryContext)
        return result;

    result = CACHING_SERVER(server)->super_dispatch.eglQueryContext (server, display, ctx, attribute, value);

    return result;
}

static EGLBoolean
caching_server_eglWaitGL (server_t *server)
{
    EGLBoolean result = EGL_FALSE;
    
    if (! CACHING_SERVER(server)->super_dispatch.eglWaitGL)
        return result;

    result = CACHING_SERVER(server)->super_dispatch.eglWaitGL (server);

    return result;
}

static EGLBoolean
caching_server_eglWaitNative (server_t *server,
                  EGLint engine)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglWaitNative)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglWaitNative (server, engine);

    return result;
}

static EGLBoolean
caching_server_eglSwapBuffers (server_t *server,
                   EGLDisplay display,
                   EGLSurface surface)
{
    EGLBoolean result = EGL_BAD_DISPLAY;

    result = CACHING_SERVER(server)->super_dispatch.eglSwapBuffers (server, display, surface);

    return result;
}

static EGLBoolean
caching_server_eglCopyBuffers (server_t *server,
                   EGLDisplay display,
                   EGLSurface surface,
                   EGLNativePixmapType target)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglCopyBuffers)
        return result;

    result = CACHING_SERVER(server)->super_dispatch.eglCopyBuffers (server, display, surface, target);

    return result;
}

static __eglMustCastToProperFunctionPointerType
caching_server_eglGetProcAddress (server_t *server,
                       const char *procname)
{
    return CACHING_SERVER(server)->super_dispatch.eglGetProcAddress (server, procname);
}

static EGLBoolean 
caching_server_eglMakeCurrent (server_t *server,
                   EGLDisplay display,
                   EGLSurface draw,
                   EGLSurface read,
                   EGLContext ctx) 
{
    EGLBoolean result = EGL_FALSE;
    link_list_t *exist = NULL;
    bool found = false;

    if (! CACHING_SERVER(server)->super_dispatch.eglMakeCurrent)
        return result;

    /* look for existing */
    found = _match (display, draw, read, ctx, &exist);
    if (found == true) {
        /* set active to exist, tell client about it */
        CACHING_SERVER(server)->active_state = exist;

        /* call real makeCurrent */
        return CACHING_SERVER(server)->super_dispatch.eglMakeCurrent (server, display, draw, read, ctx);
    }

    /* We could not find in the saved state, we don't know whether
     * parameters are valid or not 
     */
    result = CACHING_SERVER(server)->super_dispatch.eglMakeCurrent (server, display, draw, read, ctx);
    if (result == EGL_TRUE) {
        _server_make_current (server, display, draw, read, ctx);
    }
    return result;
}

/* start of eglext.h */
static EGLBoolean
caching_server_eglLockSurfaceKHR (server_t *server,
                       EGLDisplay display,
                       EGLSurface surface,
                       const EGLint *attrib_list)
{
    EGLBoolean result = EGL_FALSE;
    
    if (! CACHING_SERVER(server)->super_dispatch.eglLockSurfaceKHR)
        return result;

    result = CACHING_SERVER(server)->super_dispatch.eglLockSurfaceKHR (server, display, surface, attrib_list);

    return result;
}

static EGLBoolean
caching_server_eglUnlockSurfaceKHR (server_t *server,
                         EGLDisplay display,
                         EGLSurface surface)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglUnlockSurfaceKHR)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglUnlockSurfaceKHR (server, display, surface);

    return result;
}

static EGLImageKHR
caching_server_eglCreateImageKHR (server_t *server,
                       EGLDisplay display,
                       EGLContext ctx,
                       EGLenum target,
                       EGLClientBuffer buffer,
                       const EGLint *attrib_list)
{
    EGLImageKHR result = EGL_NO_IMAGE_KHR;

    if (! CACHING_SERVER(server)->super_dispatch.eglCreateImageKHR)
        return result;

    if (display == EGL_NO_DISPLAY)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglCreateImageKHR (server, display, ctx, target,
                                         buffer, attrib_list);

    return result;
}

static EGLBoolean
caching_server_eglDestroyImageKHR (server_t *server,
                        EGLDisplay display,
                        EGLImageKHR image)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglDestroyImageKHR)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglDestroyImageKHR (server, display, image);

    return result;
}

static EGLSyncKHR
caching_server_eglCreateSyncKHR (server_t *server,
                      EGLDisplay display,
                      EGLenum type,
                      const EGLint *attrib_list)
{
    EGLSyncKHR result = EGL_NO_SYNC_KHR;

    if (! CACHING_SERVER(server)->super_dispatch.eglCreateSyncKHR)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglCreateSyncKHR (server, display, type, attrib_list);

    return result;
}

static EGLBoolean
caching_server_eglDestroySyncKHR (server_t *server,
                       EGLDisplay display,
                       EGLSyncKHR sync)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglDestroySyncKHR)
        return result;

    result = CACHING_SERVER(server)->super_dispatch.eglDestroySyncKHR (server, display, sync);

    return result;
}

static EGLint
caching_server_eglClientWaitSyncKHR (server_t *server,
                           EGLDisplay display,
                           EGLSyncKHR sync,
                           EGLint flags,
                           EGLTimeKHR timeout)
{
    EGLint result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglClientWaitSyncKHR)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglClientWaitSyncKHR (server, display, sync, flags, timeout);

    return result;
}

static EGLBoolean
caching_server_eglSignalSyncKHR (server_t *server,
                      EGLDisplay display,
                      EGLSyncKHR sync,
                      EGLenum mode)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglSignalSyncKHR)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglSignalSyncKHR (server, display, sync, mode);

    return result;
}

static EGLBoolean
caching_server_eglGetSyncAttribKHR (server_t *server,
                          EGLDisplay display,
                          EGLSyncKHR sync,
                          EGLint attribute,
                          EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglGetSyncAttribKHR)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglGetSyncAttribKHR (server, display, sync, attribute, value);

    return result;
}

static EGLSyncNV 
caching_server_eglCreateFenceSyncNV (server_t *server,
                                     EGLDisplay display,
                                     EGLenum condition,
                                     const EGLint *attrib_list)
{
    EGLSyncNV result = EGL_NO_SYNC_NV;

    if (! CACHING_SERVER(server)->super_dispatch.eglCreateFenceSyncNV)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglCreateFenceSyncNV (server, display, condition, attrib_list);

    return result;
}

static EGLBoolean 
caching_server_eglDestroySyncNV (server_t *server,
                      EGLSyncNV sync)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglDestroySyncNV)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglDestroySyncNV (server, sync);

    return result;
}

static EGLBoolean
caching_server_eglFenceNV (server_t *server,
               EGLSyncNV sync)
{
    EGLBoolean result = EGL_FALSE;
    
    if (! CACHING_SERVER(server)->super_dispatch.eglFenceNV)
        return result;

    result = CACHING_SERVER(server)->super_dispatch.eglFenceNV (server, sync);

    return result;
}

static EGLint
caching_server_eglClientWaitSyncNV (server_t *server,
                          EGLSyncNV sync,
                          EGLint flags,
                          EGLTimeNV timeout)
{
    /* XXX: is this supposed to be default value ? */
    EGLint result = EGL_TIMEOUT_EXPIRED_NV;

    if (! CACHING_SERVER(server)->super_dispatch.eglClientWaitSyncNV)
        return result;

    result = CACHING_SERVER(server)->super_dispatch.eglClientWaitSyncNV (server, sync, flags, timeout);

    return result;
}

static EGLBoolean
caching_server_eglSignalSyncNV (server_t *server,
                     EGLSyncNV sync,
                     EGLenum mode)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglSignalSyncNV)
        return result;

    result = CACHING_SERVER(server)->super_dispatch.eglSignalSyncNV (server, sync, mode);

    return result;
}

static EGLBoolean
caching_server_eglGetSyncAttribNV (server_t *server,
                         EGLSyncNV sync,
                         EGLint attribute,
                         EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglGetSyncAttribNV)
        return result;

    result = CACHING_SERVER(server)->super_dispatch.eglGetSyncAttribNV (server, sync, attribute, value);

    return result;
}

static EGLSurface
caching_server_eglCreatePixmapSurfaceHI (server_t *server,
                                         EGLDisplay display,
                                         EGLConfig config,
                                         struct EGLClientPixmapHI *pixmap)
{
    EGLSurface result = EGL_NO_SURFACE;

    if (! CACHING_SERVER(server)->super_dispatch.eglCreatePixmapSurfaceHI)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglCreatePixmapSurfaceHI (server, display, config, pixmap);

    return result;
}

static EGLImageKHR
caching_server_eglCreateDRMImageMESA (server_t *server,
                                      EGLDisplay display,
                                      const EGLint *attrib_list)
{
    EGLImageKHR result = EGL_NO_IMAGE_KHR;

    if (! CACHING_SERVER(server)->super_dispatch.eglCreateDRMImageMESA)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglCreateDRMImageMESA (server, display, attrib_list);

    return result;
}

static EGLBoolean
caching_server_eglExportDRMImageMESA (server_t *server,
                                      EGLDisplay display,
                                      EGLImageKHR image,
                                      EGLint *name,
                                      EGLint *handle,
                                      EGLint *stride)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglExportDRMImageMESA)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglExportDRMImageMESA (server, display, image, name, handle, stride);

    return result;
}

static EGLBoolean 
caching_server_eglPostSubBufferNV (server_t *server,
                                   EGLDisplay display,
                                   EGLSurface surface, 
                                   EGLint x,
                                   EGLint y,
                                   EGLint width,
                                   EGLint height)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglExportDRMImageMESA)
        return result;

    result = CACHING_SERVER(server)->super_dispatch.eglPostSubBufferNV (server, display, surface, x, y, width, height);

    return result;
}

static void *
caching_server_eglMapImageSEC (server_t *server,
                               EGLDisplay display,
                               EGLImageKHR image)
{
    void *result = NULL;

    if (! CACHING_SERVER(server)->super_dispatch.eglMapImageSEC)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglMapImageSEC (server, display, image);

    return result;
}

static EGLBoolean
caching_server_eglUnmapImageSEC (server_t *server,
                                 EGLDisplay display,
                                 EGLImageKHR image)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglUnmapImageSEC)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglUnmapImageSEC (server, display, image);

    return result;
}

static EGLBoolean
caching_server_eglGetImageAttribSEC (server_t *server,
                                     EGLDisplay display,
                                     EGLImageKHR image,
                                     EGLint attribute,
                                     EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglGetImageAttribSEC)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglGetImageAttribSEC (server, display, image, attribute, value);

    return result;
}

static void caching_server_command_post_hook(server_t *server, command_t *command)
{

    switch (command->type) {
    case COMMAND_GLATTACHSHADER:
    case COMMAND_GLCOMPILESHADER:
    case COMMAND_GLCOPYTEXIMAGE2D:
    case COMMAND_GLCOPYTEXSUBIMAGE2D:
    case COMMAND_GLDELETEPROGRAM:
    case COMMAND_GLDELETESHADER:
    case COMMAND_GLDETACHSHADER:
    case COMMAND_GLLINKPROGRAM:
    case COMMAND_GLVALIDATEPROGRAM:
    case COMMAND_GLRELEASESHADERCOMPILER:
    case COMMAND_GLRENDERBUFFERSTORAGE:
    case COMMAND_GLUNIFORM1F:
    case COMMAND_GLUNIFORM1I:
    case COMMAND_GLUNIFORM2F:
    case COMMAND_GLUNIFORM2I:
    case COMMAND_GLUNIFORM3F:
    case COMMAND_GLUNIFORM3I:
    case COMMAND_GLUNIFORM4F:
    case COMMAND_GLUNIFORM4I:
    case COMMAND_GLCOPYTEXSUBIMAGE3DOES:
    case COMMAND_GLBEGINPERFMONITORAMD:
    case COMMAND_GLENDPERFMONITORAMD:
    case COMMAND_GLBLITFRAMEBUFFERANGLE:
    case COMMAND_GLRENDERBUFFERSTORAGEMULTISAMPLEANGLE:
    case COMMAND_GLRENDERBUFFERSTORAGEMULTISAMPLEAPPLE:
    case COMMAND_GLRESOLVEMULTISAMPLEFRAMEBUFFERAPPLE:
    case COMMAND_GLRENDERBUFFERSTORAGEMULTISAMPLEEXT:
    case COMMAND_GLRENDERBUFFERSTORAGEMULTISAMPLEIMG:
    case COMMAND_GLFINISHFENCENV:
    case COMMAND_GLCOVERAGEMASKNV:
    case COMMAND_GLGETDRIVERCONTROLSQCOM:
    case COMMAND_GLENABLEDRIVERCONTROLQCOM:
    case COMMAND_GLDISABLEDRIVERCONTROLQCOM:
    case COMMAND_GLEXTTEXOBJECTSTATEOVERRIDEIQCOM: {
        egl_state_t *egl_state = (egl_state_t *) CACHING_SERVER(server)->active_state->data;
        egl_state->state.need_get_error = true;
        break;
    }
    default:
        break;
    }
}

void
caching_server_init (caching_server_t *server, buffer_t *buffer)
{
    server_init (&server->super, buffer);
    server->super.command_post_hook = caching_server_command_post_hook;
    server->super_dispatch = server->super.dispatch;
    server->active_state = NULL;

    mutex_lock (server_states_mutex);
    if (server_states.initialized == false) {
        server_states.num_contexts = 0;
        server_states.states = NULL;
        server_states.initialized = true;
    }
    mutex_unlock (server_states_mutex);

#include "caching_server_dispatch_autogen.c"
}

caching_server_t *
caching_server_new (buffer_t *buffer)
{
    caching_server_t *server = malloc (sizeof(caching_server_t));
    caching_server_init (server, buffer);
    return server;
}
