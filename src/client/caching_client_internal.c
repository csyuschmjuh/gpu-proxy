/* This file implements the client thread side of GLES2 functions.  Thses
 * functions are called by the command buffer.
 *
 * It references to the client thread side of CACHING_CLIENT(client)->active_state.
 * if CACHING_CLIENT(client)->active_state is NULL or there is no symbol for the corresponding
 * gl functions, the cached error state is set to GL_INVALID_OPERATION
 * if the cached error has not been set to one of the errors.
 */

/* This file implements the client thread side of egl functions.
 * After the client thread reads the command buffer, if it is 
 * egl calls, it is routed to here.
 *
 * It keeps two global variables for all client threads:
 * (1) dispatch - a dispatch table to real egl and gl calls. It 
 * is initialized during eglGetDisplay() -> caching_client_eglGetDisplay()
 * (2) client_states - this is a double-linked structure contains
 * all active and inactive egl states.  When a client switches context,
 * the previous egl_state is set to be inactive and thus is subject to
 * be destroyed during caching_client_eglTerminate(), caching_server_eglDestroyContext() and
 * caching_client_eglReleaseThread()  The inactive context's surface can also be 
 * destroyed by caching_client_eglDestroySurface().
 * (3) CACHING_CLIENT(client)->active_state - this is the pointer to the current active state.
 */

#include "config.h"
#include "caching_client_private.h"

#include "egl_states.h"
#include "types_private.h"
#include <EGL/eglext.h>
#include <EGL/egl.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2.h>
#include <stdlib.h>
#include <string.h>

#include "command.h"

/* global state variable for all client threads */
extern mutex_t cached_gl_states_mutex;
extern gl_states_t cached_gl_states;


static bool
_caching_client_has_context (egl_state_t *state,
                             EGLDisplay  display,
                             EGLContext  context)
{
    return (state->context == context && state->display == display);
}

static void
_caching_client_init_gles2_states (egl_state_t *egl_state)
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
_caching_client_set_egl_states (egl_state_t *egl_state, 
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
_caching_client_remove_state (link_list_t **state)
{
    egl_state_t *egl_state = (egl_state_t *) (*state)->data;

    if (egl_state->state.vertex_attribs.attribs != 
        egl_state->state.vertex_attribs.embedded_attribs)
        free (egl_state->state.vertex_attribs.attribs);

    if (*state == cached_gl_states.states) {
        cached_gl_states.states = cached_gl_states.states->next;
        if (cached_gl_states.states != NULL)
            cached_gl_states.states->prev = NULL;
    }

    if ((*state)->prev)
        (*state)->prev->next = (*state)->next;
    if ((*state)->next)
        (*state)->next->prev = (*state)->prev;

    free (egl_state);
    free (*state);
    *state = NULL;

    cached_gl_states.num_contexts --;
}

static void 
_caching_client_remove_surface (link_list_t *state, bool read)
{
    egl_state_t *egl_state = (egl_state_t *) state->data;

    if (read == true)
        egl_state->readable = EGL_NO_SURFACE;
    else
        egl_state->drawable = EGL_NO_SURFACE;

}

static link_list_t *
_caching_client_get_state (EGLDisplay dpy,
                           EGLSurface draw,
                           EGLSurface read,
                           EGLContext ctx)
{
    link_list_t *new_list;
    egl_state_t   *new_state;
    link_list_t *list = cached_gl_states.states;

    if (cached_gl_states.num_contexts == 0 || ! cached_gl_states.states) {
        cached_gl_states.num_contexts = 1;
        cached_gl_states.states = (link_list_t *)malloc (sizeof (link_list_t));
        cached_gl_states.states->prev = NULL;
        cached_gl_states.states->next = NULL;

        new_state = (egl_state_t *)malloc (sizeof (egl_state_t));

        _caching_client_init_gles2_states (new_state);
        _caching_client_set_egl_states (new_state, dpy, draw, read, ctx); 
        cached_gl_states.states->data = new_state;
        new_state->active = true;
	return cached_gl_states.states;
    }

    /* look for matching context in existing states */
    while (list) {
        egl_state_t *state = (egl_state_t *)list->data;

        if (state->context == ctx &&
            state->display == dpy) {
            _caching_client_set_egl_states (state, dpy, draw, read, ctx);
            state->active = true;
            return list;
        }
        list = list->next;
    }

    /* we have not found a context match */
    new_state = (egl_state_t *) malloc (sizeof (egl_state_t));

    _caching_client_init_gles2_states (new_state);
    _caching_client_set_egl_states (new_state, dpy, draw, read, ctx); 
    cached_gl_states.num_contexts ++;

    list = cached_gl_states.states;
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

/* the client first calls eglTerminate (display),
 * then look over the cached states
 */
static void 
_caching_client_terminate (client_t *client, EGLDisplay display)
{
    link_list_t *head = cached_gl_states.states;
    link_list_t *list = head;
    link_list_t *current;

    mutex_lock (cached_gl_states_mutex);

    if (cached_gl_states.initialized == false ||
        cached_gl_states.num_contexts == 0 || (! cached_gl_states.states)) {
        mutex_unlock (cached_gl_states_mutex);
        return;
    }
    
    while (list != NULL) {
        egl_state_t *egl_state = (egl_state_t *) list->data;
        current = list;
        list = list->next;

        if (egl_state->display == display) {
            if (! egl_state->active)
                _caching_client_remove_state (&current);
                /* XXX: Do we need to stop the thread? */
        }
    }

    /* is current active state affected ?*/
    if (CACHING_CLIENT(client)->active_state) { 
        egl_state_t *egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
        if (egl_state->display == display)
            egl_state->destroy_dpy = true;
    }
    /* XXX: should we stop current client thread ? */
    /*
    else if (! CACHING_CLIENT(client)->active_state) {
    } */
    mutex_unlock (cached_gl_states_mutex);
}

/* we should call real eglMakeCurrent() before, and wait for result
 * if eglMakeCurrent() returns EGL_TRUE, then we call this
 */
static void 
_caching_client_make_current (client_t *client,
                              EGLDisplay display,
                              EGLSurface drawable,
                              EGLSurface readable,
                              EGLContext context)
{
    egl_state_t *egl_state;

    mutex_lock (cached_gl_states_mutex);
    /* we are switching to None context */
    if (context == EGL_NO_CONTEXT || display == EGL_NO_DISPLAY) {
        /* current is None too */
        if (! CACHING_CLIENT(client)->active_state) {
            CACHING_CLIENT(client)->active_state = NULL;
            mutex_unlock (cached_gl_states_mutex);
            return;
        }
        
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
        egl_state->active = false;
        
        if (egl_state->destroy_dpy || egl_state->destroy_ctx)
            _caching_client_remove_state (&CACHING_CLIENT(client)->active_state);
        
        if (CACHING_CLIENT(client)->active_state) {
            if (egl_state->destroy_read)
                _caching_client_remove_surface (CACHING_CLIENT(client)->active_state, true);

            if (egl_state->destroy_draw)
                _caching_client_remove_surface (CACHING_CLIENT(client)->active_state, false);
        }

        CACHING_CLIENT(client)->active_state = NULL;
        mutex_unlock (cached_gl_states_mutex);
        return;
    }

    /* we are switch to one of the valid context */
    if (CACHING_CLIENT(client)->active_state) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
        egl_state->active = false;
        
        /* XXX did eglTerminate()/eglDestroyContext()/eglDestroySurface()
         * called affect us?
         */
        if (egl_state->destroy_dpy || egl_state->destroy_ctx)
            _caching_client_remove_state (&CACHING_CLIENT(client)->active_state);
        
        if (CACHING_CLIENT(client)->active_state) {
            if (egl_state->destroy_read)
                _caching_client_remove_surface (CACHING_CLIENT(client)->active_state, true);

            if (egl_state->destroy_draw)
                _caching_client_remove_surface (CACHING_CLIENT(client)->active_state, false);
        }
    }

    /* get existing state or create a new one */
    CACHING_CLIENT(client)->active_state = 
        _caching_client_get_state (display, drawable, readable, context);
    mutex_unlock (cached_gl_states_mutex);
}

/* called by eglDestroyContext() - once we know there is matching context
 * we call real eglDestroyContext(), and if returns EGL_TRUE, we call 
 * this function 
 */
static void
_caching_client_destroy_context (client_t *client,
                                 EGLDisplay display, 
                                 EGLContext context)
{
    egl_state_t *state;
    link_list_t *list = cached_gl_states.states;
    link_list_t *current;

    mutex_lock (cached_gl_states_mutex);
    if (cached_gl_states.num_contexts == 0 || ! cached_gl_states.states) {
        mutex_unlock (cached_gl_states_mutex);
        return;
    }

    while (list != NULL) {
        current = list;
        list = list->next;
        state = (egl_state_t *)current->data;
    
        if (_caching_client_has_context (state, display, context)) {
            state->destroy_ctx = true;
            if (state->active == false) 
                _caching_client_remove_state (&current);
        }
    }
    mutex_unlock (cached_gl_states_mutex);
}

static void
_caching_client_destroy_surface (client_t *client,
                                 EGLDisplay display, 
                                 EGLSurface surface)
{
    egl_state_t *state;
    link_list_t *list = cached_gl_states.states;
    link_list_t *current;

    mutex_lock (cached_gl_states_mutex);
    if (cached_gl_states.num_contexts == 0 || ! cached_gl_states.states) {
        mutex_unlock (cached_gl_states_mutex);
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

    mutex_unlock (cached_gl_states_mutex);
}

static bool
_match (EGLDisplay display,
        EGLSurface drawable,
        EGLSurface readable,
        EGLContext context, 
        link_list_t **state)
{
    egl_state_t *egl_state;
    link_list_t *list = cached_gl_states.states;
    link_list_t *current;

    mutex_lock (cached_gl_states_mutex);
    if (cached_gl_states.num_contexts == 0 || ! cached_gl_states.states) {
        mutex_unlock (cached_gl_states_mutex);
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
            egl_state->active == false      && 
            ! egl_state->destroy_dpy ) {
                egl_state->active = true;
                *state = current;
                mutex_unlock (cached_gl_states_mutex);
                return true;
        }
    }

    mutex_unlock (cached_gl_states_mutex);
    return false;
}
/* the client first calls eglInitialize (),
 * then look over the cached states
 */
/*
static void 
_caching_client_initialize (EGLDisplay display) 
{
    link_list_t *head = cached_gl_states.states;
    link_list_t *list = head;

    egl_state_t *egl_state;

    mutex_lock (cached_gl_states_mutex);

    if (cached_gl_states.initialized == false ||
        cached_gl_states.num_contexts == 0 || (! cached_gl_states.states)) {
        mutex_unlock (cached_gl_states_mutex);
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
    mutex_unlock (cached_gl_states_mutex);
}*/
/*
static bool
caching_client_glIsValidFunc (client_t *client, 
                              void *func)
{
    egl_state_t *egl_state;

    if (func)
        return true;

    if (CACHING_CLIENT(client)->active_state) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
    
        if (egl_state->active == true &&
            egl_state->state.error == GL_NO_ERROR) {
            // FIXME: why?
            // egl_state->state.error = GL_INVALID_OPERATION;
            return true;
        }
    }
    return false;
}
*/

static bool
caching_client_glIsValidContext (client_t *client)
{
    egl_state_t *egl_state;

    bool is_valid = false;

    if (CACHING_CLIENT(client)->active_state) {
        egl_state = (egl_state_t *)CACHING_CLIENT(client)->active_state->data;
        if (egl_state->active)
            return true;
    }
    return is_valid;
}

static void
caching_client_glSetError (client_t *client, GLenum error)
{
    egl_state_t *egl_state;

    if (CACHING_CLIENT(client)->active_state) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
 
        if (egl_state->active && egl_state->state.error == GL_NO_ERROR)
            egl_state->state.error = error;
    }
}

/* GLES2 core profile API */
static void
caching_client_glActiveTexture (client_t *client, GLenum texture)
{
    egl_state_t *egl_state;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (egl_state->state.active_texture == texture)
            return;
        else if (texture > GL_TEXTURE31 || texture < GL_TEXTURE0) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }
        else {
            command_t *command = 
                client_get_space_for_command (COMMAND_GLACTIVETEXTURE);
            command_glactivetexture_init (command, texture);
            client_run_command_async (command);
            /* FIXME: this maybe not right because this texture may be 
             * invalid object, we save here to save time in glGetError() 
             */
            egl_state->state.active_texture = texture;
        }
    }
}

static void
caching_client_glBindBuffer (client_t *client, GLenum target, GLuint buffer)
{
    egl_state_t *egl_state;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (target == GL_ARRAY_BUFFER) {
            if (egl_state->state.array_buffer_binding == buffer)
                return;
            else {
                command_t *command = 
                    client_get_space_for_command (COMMAND_GLBINDBUFFER);
                command_glbindbuffer_init (command, target, buffer);
                client_run_command_async (command);
                egl_state->state.need_get_error = true;

               /* FIXME: we don't know whether it succeeds or not */
               egl_state->state.array_buffer_binding = buffer;
            }
        }
        else if (target == GL_ELEMENT_ARRAY_BUFFER) {
            if (egl_state->state.element_array_buffer_binding == buffer)
                return;
            else {
                command_t *command = 
                    client_get_space_for_command (COMMAND_GLBINDBUFFER);
                command_glbindbuffer_init (command, target, buffer);
                client_run_command_async (command);
                egl_state->state.need_get_error = true;

               /* FIXME: we don't know whether it succeeds or not */
               egl_state->state.element_array_buffer_binding = buffer;
            }
        }
        else {
            caching_client_glSetError (client, GL_INVALID_ENUM);
        }
    }
}

static void
caching_client_glBindFramebuffer (client_t *client, GLenum target, GLuint framebuffer)
{
    egl_state_t *egl_state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (target == GL_FRAMEBUFFER &&
            egl_state->state.framebuffer_binding == framebuffer)
                return;

        if (target != GL_FRAMEBUFFER) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
        }

        command_t *command = client_get_space_for_command (COMMAND_GLBINDFRAMEBUFFER);
        command_glbindframebuffer_init (command, target, framebuffer);
        client_run_command_async (command);
        /* FIXME: should we save it, it will be invalid if the
         * framebuffer is invalid 
         */
        egl_state->state.framebuffer_binding = framebuffer;

        egl_state->state.need_get_error = true;
    }
}

static void
caching_client_glBindRenderbuffer (client_t *client, GLenum target, GLuint renderbuffer)
{
    egl_state_t *egl_state;
 
    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (target != GL_RENDERBUFFER) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
        }

        command_t *command = client_get_space_for_command (COMMAND_GLBINDRENDERBUFFER);
        command_glbindrenderbuffer_init (command, target, renderbuffer);
        client_run_command_async (command);
        /* FIXME: should we save it, it will be invalid if the
         * renderbuffer is invalid 
         */
        egl_state->state.renderbuffer_binding = renderbuffer;
        egl_state->state.need_get_error = true;
    }
}

static void
caching_client_glBindTexture (client_t *client, GLenum target, GLuint texture)
{
    egl_state_t *egl_state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

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
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }
        
        command_t *command = client_get_space_for_command (COMMAND_GLBINDTEXTURE);
        command_glbindtexture_init (command, target, texture);
        client_run_command_async (command);

        egl_state->state.need_get_error = true;

        /* FIXME: do we need to save them ? */
        if (target == GL_TEXTURE_2D)
            egl_state->state.texture_binding[0] = texture;
        else if (target == GL_TEXTURE_CUBE_MAP)
            egl_state->state.texture_binding[1] = texture;
        else
            egl_state->state.texture_binding_3d = texture;

    }
}

static void
caching_client_glBlendColor (client_t *client, GLclampf red, 
                             GLclampf green,
                             GLclampf blue, GLclampf alpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
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

        command_t *command = client_get_space_for_command (COMMAND_GLBLENDCOLOR);
        command_glblendcolor_init (command, red, green, blue, alpha);
        client_run_command_async (command);
    }
}

static void
caching_client_glBlendEquation (client_t *client, GLenum mode)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
        state = &egl_state->state;
    
        if (state->blend_equation[0] == mode &&
            state->blend_equation[1] == mode)
            return;

        if (! (mode == GL_FUNC_ADD ||
               mode == GL_FUNC_SUBTRACT ||
               mode == GL_FUNC_REVERSE_SUBTRACT)) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }

        state->blend_equation[0] = mode;
        state->blend_equation[1] = mode;

        command_t *command = client_get_space_for_command (COMMAND_GLBLENDEQUATION);
        command_glblendequation_init (command, mode);
        client_run_command_async (command);
    }
}

static void
caching_client_glBlendEquationSeparate (client_t *client, GLenum modeRGB, GLenum modeAlpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
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
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }

        state->blend_equation[0] = modeRGB;
        state->blend_equation[1] = modeAlpha;

        command_t *command = client_get_space_for_command (COMMAND_GLBLENDEQUATIONSEPARATE);
        command_glblendequationseparate_init (command, modeRGB, modeAlpha);
        client_run_command_async (command);
    }
}

static void
caching_client_glBlendFunc (client_t *client, GLenum sfactor, GLenum dfactor)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
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
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }

        state->blend_src[0] = state->blend_src[1] = sfactor;
        state->blend_dst[0] = state->blend_dst[1] = dfactor;

        command_t *command = client_get_space_for_command (COMMAND_GLBLENDFUNC);
        command_glblendfunc_init (command, sfactor, dfactor);
        client_run_command_async (command);
    }
}

static void
caching_client_glBlendFuncSeparate (client_t *client, GLenum srcRGB, GLenum dstRGB,
                                    GLenum srcAlpha, GLenum dstAlpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
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
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }

        state->blend_src[0] = srcRGB;
        state->blend_src[1] = srcAlpha;
        state->blend_dst[0] = dstRGB;
        state->blend_dst[0] = dstAlpha;

        command_t *command = client_get_space_for_command (COMMAND_GLBLENDFUNCSEPARATE);
        command_glblendfuncseparate_init (command, srcRGB, dstRGB, srcAlpha, dstAlpha);
        client_run_command_async (command);
    }
}

static GLenum
caching_client_glCheckFramebufferStatus (client_t *client, GLenum target)
{
    GLenum result = GL_INVALID_ENUM;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {

        if (target != GL_FRAMEBUFFER) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return result;
        }

        command_t *command = client_get_space_for_command (COMMAND_GLCHECKFRAMEBUFFERSTATUS);
        command_glcheckframebufferstatus_init (command, target);
        client_run_command (command);

        return ((command_glcheckframebufferstatus_t *)command)->result;
    }

    return result;
}

static void
caching_client_glClear (client_t *client, GLbitfield mask)
{
    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {

        if (! (mask & GL_COLOR_BUFFER_BIT ||
               mask & GL_DEPTH_BUFFER_BIT ||
               mask & GL_STENCIL_BUFFER_BIT)) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }

        command_t *command = client_get_space_for_command (COMMAND_GLCLEAR);
        command_glclear_init (command, mask);
        client_run_command_async (command);
    }
}

static void
caching_client_glClearColor (client_t *client, GLclampf red, GLclampf green,
                 GLclampf blue, GLclampf alpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
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

        command_t *command = client_get_space_for_command (COMMAND_GLCLEARCOLOR);
        command_glclearcolor_init (command, red, green, blue, alpha);
        client_run_command_async (command);
    }
}

static void
caching_client_glClearDepthf (client_t *client, GLclampf depth)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
        state = &egl_state->state;

        if (state->depth_clear_value == depth)
            return;

        state->depth_clear_value = depth;

        command_t *command = client_get_space_for_command (COMMAND_GLCLEARDEPTHF);
        command_glcleardepthf_init (command, depth);
        client_run_command_async (command);
    }
}

static void
caching_client_glClearStencil (client_t *client, GLint s)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
        state = &egl_state->state;

        if (state->stencil_clear_value == s)
            return;

        state->stencil_clear_value = s;

        command_t *command = client_get_space_for_command (COMMAND_GLCLEARSTENCIL);
        command_glclearstencil_init (command, s);
        client_run_command_async (command);
    }
}

static void
caching_client_glColorMask (client_t *client, GLboolean red, GLboolean green,
                GLboolean blue, GLboolean alpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
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

        command_t *command = client_get_space_for_command (COMMAND_GLCOLORMASK);
        command_glcolormask_init (command, red, green, blue, alpha);
        client_run_command_async (command);
    }
}

static GLuint
caching_client_glCreateProgram (client_t *client)
{
    GLuint result = 0;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        command_t *command = client_get_space_for_command (COMMAND_GLCREATEPROGRAM);
        command_glcreateprogram_init (command);
        client_run_command (command);

        result = ((command_glcreateprogram_t *)command)->result;

        if (result == 0) {
            egl_state_t *egl_state; egl_state = 
                (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
            egl_state->state.need_get_error = true;
        }
    }
    return result;
}

static GLuint
caching_client_glCreateShader (client_t *client, GLenum shaderType)
{
    GLuint result = 0;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {

        if (! (shaderType == GL_VERTEX_SHADER ||
               shaderType == GL_FRAGMENT_SHADER)) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return result;
        }
        
        command_t *command = client_get_space_for_command (COMMAND_GLCREATESHADER);
        command_glcreateshader_init (command, shaderType);
        client_run_command (command);

        result = ((command_glcreateshader_t *)command)->result;

        if (result == 0) {
            egl_state_t *egl_state; egl_state = 
                (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
            egl_state->state.need_get_error = true;
        }
    }

    return result;
}

static void
caching_client_glCullFace (client_t *client, GLenum mode)
{
    egl_state_t *egl_state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (egl_state->state.cull_face_mode == mode)
            return;

        if (! (mode == GL_FRONT ||
               mode == GL_BACK ||
               mode == GL_FRONT_AND_BACK)) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }

        egl_state->state.cull_face_mode = mode;

        command_t *command = client_get_space_for_command (COMMAND_GLCULLFACE);
        command_glcullface_init (command, mode);
        client_run_command_async (command);
    }
}

static void
caching_client_glDeleteBuffers (client_t *client, GLsizei n, const GLuint *buffers)
{
    egl_state_t *egl_state;
    int count;
    int i, j;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
        vertex_attrib_list_t *attrib_list = &egl_state->state.vertex_attribs;
        vertex_attrib_t *attribs = attrib_list->attribs;
        count = attrib_list->count;

        if (n < 0) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }

        command_t *command = client_get_space_for_command (COMMAND_GLDELETEBUFFERS);
        command_gldeletebuffers_init (command, n, buffers);
        client_run_command_async (command);

        /* check array_buffer_binding and element_array_buffer_binding */
        for (i = 0; i < n; i++) {
            if (buffers[i] == egl_state->state.array_buffer_binding)
                egl_state->state.array_buffer_binding = 0;
            else if (buffers[i] == egl_state->state.element_array_buffer_binding)
                egl_state->state.element_array_buffer_binding = 0;
        }
        
        /* update client state */
        if (count == 0)
            return;

        for (i = 0; i < n; i++) {
            if (attribs[0].array_buffer_binding == buffers[i]) {
                for (j = 0; j < count; j++) {
                    attribs[j].array_buffer_binding = 0;
                }
                break;
            }
        }
    }

}

static void
caching_client_glDeleteFramebuffers (client_t *client, GLsizei n, const GLuint *framebuffers)
{
    egl_state_t *egl_state;
    int i;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (n < 0) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }
        
        command_t *command = client_get_space_for_command (COMMAND_GLDELETEFRAMEBUFFERS);
        command_gldeleteframebuffers_init (command, n, framebuffers);
        client_run_command_async (command);

        for (i = 0; i < n; i++) {
            if (egl_state->state.framebuffer_binding == framebuffers[i]) {
                egl_state->state.framebuffer_binding = 0;
                break;
            }
        }
    }
}

static void
caching_client_glDeleteRenderbuffers (client_t *client, GLsizei n, const GLuint *renderbuffers)
{
    INSTRUMENT();
 
    if (caching_client_glIsValidContext (client)) {
        if (n < 0) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }

        command_t *command = client_get_space_for_command (COMMAND_GLDELETERENDERBUFFERS);
        command_gldeleterenderbuffers_init (command, n, renderbuffers);
        client_run_command_async (command);
    }
}

static void
caching_client_glDeleteTextures (client_t *client, GLsizei n, const GLuint *textures)
{
    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {

        if (n < 0) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }

        command_t *command = client_get_space_for_command (COMMAND_GLDELETETEXTURES);
        command_gldeletetextures_init (command, n, textures);
        client_run_command_async (command);
    }
}

static void
caching_client_glDepthFunc (client_t *client, GLenum func)
{
    egl_state_t *egl_state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

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
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }

        egl_state->state.depth_func = func;

        command_t *command = client_get_space_for_command (COMMAND_GLDEPTHFUNC);
        command_gldepthfunc_init (command, func);
        client_run_command_async (command);
    }
}

static void
caching_client_glDepthMask (client_t *client, GLboolean flag)
{
    egl_state_t *egl_state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (egl_state->state.depth_writemask == flag)
            return;

        egl_state->state.depth_writemask = flag;

        command_t *command = client_get_space_for_command (COMMAND_GLDEPTHMASK);
        command_gldepthmask_init (command, flag);
        client_run_command_async (command);
    }
}

static void
caching_client_glDepthRangef (client_t *client, GLclampf nearVal, GLclampf farVal)
{
    egl_state_t *egl_state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (egl_state->state.depth_range[0] == nearVal &&
            egl_state->state.depth_range[1] == farVal)
            return;

        egl_state->state.depth_range[0] = nearVal;
        egl_state->state.depth_range[1] = farVal;

        command_t *command = client_get_space_for_command (COMMAND_GLDEPTHRANGEF);
        command_gldepthrangef_init (command, nearVal, farVal);
        client_run_command_async (command);
    }
}

void
caching_client_glSetCap (client_t *client, GLenum cap, GLboolean enable)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    bool needs_call = false;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

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
            //state->need_get_error = true;
            break;
        }

        if (needs_call) {
            if (enable) {
                command_t *command = client_get_space_for_command (COMMAND_GLENABLE);
                command_glenable_init (command, cap);
                client_run_command_async (command);
            }
            else {
                command_t *command = client_get_space_for_command (COMMAND_GLDISABLE);
                command_gldisable_init (command, cap);
                client_run_command_async (command);
            }
        }
    }
}

static void
caching_client_glDisable (client_t *client, GLenum cap)
{
    caching_client_glSetCap (client, cap, GL_FALSE);
}

static void
caching_client_glEnable (client_t *client, GLenum cap)
{
    caching_client_glSetCap (client, cap, GL_TRUE);
}

static bool
caching_client_glIndexIsTooLarge (client_t *client, gles2_state_t *state, GLuint index)
{
    if (index > state->max_vertex_attribs) {
        if (! state->max_vertex_attribs_queried) {
            /* XXX: command buffer */
            command_t *command = client_get_space_for_command (COMMAND_GLGETINTEGERV);
            command_glgetintegerv_init (command, GL_MAX_VERTEX_ATTRIBS,
                                        &(state->max_vertex_attribs));
            client_run_command (command);
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
caching_client_glSetVertexAttribArray (client_t *client, GLuint index, gles2_state_t *state, 
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
    if (caching_client_glIndexIsTooLarge (client, state, index)) {
        return;
    }

    if (enable == GL_FALSE) {
            command_t *command = client_get_space_for_command (COMMAND_GLDISABLEVERTEXATTRIBARRAY);
            command_gldisablevertexattribarray_init (command, index);
            client_run_command_async (command);
    }
    else {
            command_t *command = client_get_space_for_command (COMMAND_GLENABLEVERTEXATTRIBARRAY);
            command_glenablevertexattribarray_init (command, index);
            client_run_command_async (command);
    }

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
caching_client_glDisableVertexAttribArray (client_t *client, GLuint index)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
        state = &egl_state->state;

        caching_client_glSetVertexAttribArray (client, index, state, GL_FALSE);
    }
}

static void
caching_client_glEnableVertexAttribArray (client_t *client, GLuint index)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
        state = &egl_state->state;

        caching_client_glSetVertexAttribArray (client, index, state, GL_TRUE);
    }
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

static void
caching_client_glDrawArrays (client_t *client, GLenum mode, GLint first, GLsizei count)
{
    gles2_state_t *state;
    egl_state_t *egl_state;
    vertex_attrib_list_t *attrib_list = NULL;
    vertex_attrib_t *attribs = NULL;
    int i;
    bool needs_call = false;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
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
            caching_client_glSetError (client, GL_INVALID_ENUM);
            goto FINISH;
        }
        else if (count < 0) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            goto FINISH;
        }
        else if (count == 0)
            goto FINISH;

        /* if vertex array binding is not 0 */
        if (state->vertex_array_binding) {
            command_t *command = client_get_space_for_command (COMMAND_GLDRAWARRAYS);
            command_gldrawarrays_init (command, mode, first, count);
            client_run_command_async (command);
            //state->need_get_error = true;
            goto FINISH;
        } 
  
        for (i = 0; i < attrib_list->count; i++) {
            if (! attribs[i].array_enabled)
                continue;
            /* we need to create a separate buffer for it */
            else if (! attribs[i].array_buffer_binding) {
                attribs[i].data = _create_data_array (&attribs[i], count);
                if (! attribs[i].data)
                    continue;
                command_t *command = client_get_space_for_command (COMMAND_GLVERTEXATTRIBPOINTER);
                command_glvertexattribpointer_init (command, 
                                                    attribs[i].index,
                                                    attribs[i].size,
                                                    attribs[i].type,
                                                    attribs[i].array_normalized,
                                                    0,
                                                    (const void *)attribs[i].data);
                client_run_command_async (command);
                if (! needs_call)
                    needs_call = true;
            }
        }

        /* we need call DrawArrays */
        if (needs_call) {
            command_t *command = client_get_space_for_command (COMMAND_GLDRAWARRAYS);
            command_gldrawarrays_init (command, mode, first, count);
            client_run_command_async (command);
        }
    }

FINISH:
    for (i = 0; i < attrib_list->count; i++) {
        if (attribs[i].data) {
            /* server is responsible for free them, client resets to NULL */
            attribs[i].data = NULL;
        }
    }
}

/* FIXME: we should use pre-allocated buffer if possible */

static char *
caching_client_glCreateIndicesArray (GLenum mode, 
                                     GLenum type, int count,
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
}

static void
caching_client_glDrawElements (client_t *client, GLenum mode, GLsizei count, GLenum type,
                               const GLvoid *indices)
{
    egl_state_t *egl_state;
    gles2_state_t *state;

    vertex_attrib_list_t *attrib_list = NULL;
    vertex_attrib_t *attribs = NULL;
    int i = -1;
    bool element_binding = false;
    bool needs_call = false;
    char *indices_copy = NULL;

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
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
            caching_client_glSetError (client, GL_INVALID_ENUM);
            goto FINISH;
        }
        else if (! (type == GL_UNSIGNED_BYTE  || 
                    type == GL_UNSIGNED_SHORT)) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            goto FINISH;
        }
        else if (count < 0) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            goto FINISH;
        }
        else if (count == 0)
            goto FINISH;

        if (state->vertex_array_binding) {
            if (!element_binding) {
                indices_copy = caching_client_glCreateIndicesArray (mode, 
                                                                    type, 
                                                                    count,
                                                                    (char *)indices);
                if (indices_copy) {
                    command_t *command = client_get_space_for_command (COMMAND_GLDRAWELEMENTS);
                    command_gldrawelements_init (command, mode, count, 
                                             type, indices_copy);
                    client_run_command_async (command);
                    //state->need_get_error = true;
                    goto FINISH;
                }
            }
            else {
                command_t *command = client_get_space_for_command (COMMAND_GLDRAWELEMENTS);
                command_gldrawelements_init (command, mode, count, 
                                             type, indices);
                client_run_command_async (command);
                //state->need_get_error = true;
                goto FINISH;
            }
        }

        for (i = 0; i < attrib_list->count; i++) {
            if (! attribs[i].array_enabled)
                continue;
            /* we need to create a separate buffer for it */
            else if (! attribs[i].array_buffer_binding) {
                attribs[i].data = _create_data_array (&attribs[i], count);
                if (! attribs[i].data)
                    continue;
                command_t *command = client_get_space_for_command (COMMAND_GLVERTEXATTRIBPOINTER);
                command_glvertexattribpointer_init (command, 
                                                    attribs[i].index,
                                                    attribs[i].size,
                                                    attribs[i].type,
                                                    attribs[i].array_normalized,
                                                    0,
                                                    (const void *)attribs[i].data);
                client_run_command_async (command);
                if (! needs_call)
                    needs_call = true;
            }
        }

        if (needs_call) {
            if (! element_binding ) {
                indices_copy = caching_client_glCreateIndicesArray (mode, 
                                                                    type, 
                                                                    count,
                                                                    (char *)indices);
                if (indices_copy) {
                    command_t *command = client_get_space_for_command (COMMAND_GLDRAWELEMENTS);
                    command_gldrawelements_init (command, mode, count, 
                                                 type, indices_copy);
                    client_run_command_async (command);
                }
            }
            else {
                command_t *command = client_get_space_for_command (COMMAND_GLDRAWELEMENTS);
                command_gldrawelements_init (command, mode, count, 
                                             type, indices);
                client_run_command_async (command);
            }
        }
    }
FINISH:
    for (i = 0; i < attrib_list->count; i++) {
        if (attribs[i].data) {
            /* server release them, client reset to NULL */
            attribs[i].data = NULL;
        }
    }
}

static void
caching_client_glFramebufferRenderbuffer (client_t *client, GLenum target, GLenum attachment,
                                          GLenum renderbuffertarget,
                                          GLenum renderbuffer)
{
    egl_state_t *egl_state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (target != GL_FRAMEBUFFER) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }
        else if (renderbuffertarget != GL_RENDERBUFFER &&
                 renderbuffer != 0) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }

        command_t *command = client_get_space_for_command (COMMAND_GLFRAMEBUFFERRENDERBUFFER);
        command_glframebufferrenderbuffer_init (command, target,
                                                attachment,
                                                renderbuffertarget,
                                                renderbuffer);
        client_run_command_async (command);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_client_glFramebufferTexture2D (client_t *client, GLenum target, GLenum attachment,
                            GLenum textarget, GLuint texture, 
                            GLint level)
{
    egl_state_t *egl_state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (target != GL_FRAMEBUFFER) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }
        
        command_t *command = client_get_space_for_command (COMMAND_GLFRAMEBUFFERTEXTURE2D);
        command_glframebuffertexture2d_init (command, target,
                                           attachment,
                                           textarget,
                                           texture,
                                           level);
        client_run_command_async (command);

        egl_state->state.need_get_error = true;
    }
}

static void
caching_client_glFrontFace (client_t *client, GLenum mode)
{
    egl_state_t *egl_state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (egl_state->state.front_face == mode)
            return;

        if (! (mode == GL_CCW || mode == GL_CW)) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }

        egl_state->state.front_face = mode;
        command_t *command = client_get_space_for_command (COMMAND_GLFRONTFACE);
        command_glfrontface_init (command, mode);
        client_run_command_async (command);
    }
}

static void
caching_client_glGenBuffers (client_t *client, GLsizei n, GLuint *buffers)
{
    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {

        if (n < 0) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }
    
        command_t *command = client_get_space_for_command (COMMAND_GLGENBUFFERS);
        command_glgenbuffers_init (command, n, buffers);
        client_run_command (command);
    }
}

static void
caching_client_glGenFramebuffers (client_t *client, GLsizei n, GLuint *framebuffers)
{
    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {

        if (n < 0) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }
        
        command_t *command = client_get_space_for_command (COMMAND_GLGENFRAMEBUFFERS);
        command_glgenframebuffers_init (command, n, framebuffers);
        client_run_command (command);
    }
}

static void
caching_client_glGenRenderbuffers (client_t *client, GLsizei n, GLuint *renderbuffers)
{
    INSTRUMENT();
        
    if (caching_client_glIsValidContext (client)) {

        if (n < 0) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }
        
        command_t *command = client_get_space_for_command (COMMAND_GLGENRENDERBUFFERS);
        command_glgenrenderbuffers_init (command, n, renderbuffers);
        client_run_command (command);
    }
}

static void
caching_client_glGenTextures (client_t *client, GLsizei n, GLuint *textures)
{
    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {

        if (n < 0) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }
        
        command_t *command = client_get_space_for_command (COMMAND_GLGENTEXTURES);
        command_glgentextures_init (command, n, textures);
        client_run_command (command);
    }
}

static void
caching_client_glGenerateMipmap (client_t *client, GLenum target)
{
    egl_state_t *egl_state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (! (target == GL_TEXTURE_2D       || 
               target == GL_TEXTURE_CUBE_MAP
                                             || 
               target == GL_TEXTURE_3D_OES
                                          )) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }
        
        command_t *command = client_get_space_for_command (COMMAND_GLGENERATEMIPMAP);
        command_glgeneratemipmap_init (command, target);
        client_run_command_async (command);

        egl_state->state.need_get_error = true;
    }
}

static void caching_client_glGetActiveAttrib (client_t *client, 
                                              GLuint program,
                                              GLuint index, 
                                              GLsizei bufsize,
                                              GLsizei *length, 
                                              GLint *size, 
                                              GLenum *type, 
                                              GLchar *name)
{
    egl_state_t *egl_state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        command_t *command = client_get_space_for_command (COMMAND_GLGETACTIVEATTRIB);
        command_glgetactiveattrib_init (command, program, index, bufsize,
                                        length, size, type, name);
        client_run_command_ (command);

        if (*length == 0)
            egl_state->state.need_get_error = true;
    }
}

static void caching_client_glGetActiveUniform (client_t *client, 
                                               GLuint program,
                                               GLuint index, 
                                               GLsizei bufsize,
                                               GLsizei *length, 
                                               GLint *size, 
                                               GLenum *type, 
                                               GLchar *name)
{
    egl_state_t *egl_state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        command_t *command = client_get_space_for_command (COMMAND_GLGETACTIVEUNIFORM);
        command_glgetactiveuniform_init (command, program, index, bufsize,
                                        length, size, type, name);
        client_run_command_ (command);

        if (*length == 0)
            egl_state->state.need_get_error = true;
    }
}

static GLint  
caching_client_glGetAttribLocation (client_t *client, GLuint program,
                                    const GLchar *name)
{
    GLint result = -1;
    egl_state_t *egl_state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        command_t *command = client_get_space_for_command (COMMAND_GLGETATTRIBLOCATION);
        command_glgetattriblocation_init (command, program, name);
        client_run_command_ (command);
        result = ((command_glgetattriblocation_t *)command)->result;

        if (result == -1)
            egl_state->state.need_get_error = true;
    }
}

static GLint  
caching_client_glGetUniformLocation (client_t *client, GLuint program,
                                  const GLchar *name)
{
    GLint result = -1;
    egl_state_t *egl_state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        command_t *command = client_get_space_for_command (COMMAND_GLGETUNIFORMLOCATION);
        command_glgetuniformlocation_init (command, program, name);
        client_run_command_ (command);
        result = ((command_glgetattriblocation_t *)command)->result;

        if (result == -1)
            egl_state->state.need_get_error = true;
    }
}

static void
caching_client_glGetBooleanv (client_t *client, GLenum pname, GLboolean *params)
{
    egl_state_t *egl_state;
    command_t *command;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

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
            command = client_get_space_for_command (COMMAND_GLGETBOOLEANV);
            command_glgetbooleanv_init (command, pname, params);
            client_run_command (command);
            break;
        }
    }
}

static void
caching_client_glGetFloatv (client_t *client, GLenum pname, GLfloat *params)
{
    egl_state_t *egl_state;
    command_t *command;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

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
            command = client_get_space_for_command (COMMAND_GLGETFLOATV);
            command_glgetfloatv_init (command, pname, params);
            client_run_command (command);
            break;
        }
    }
}

static void
caching_client_glGetIntegerv (client_t *client, GLenum pname, GLint *params)
{
    egl_state_t *egl_state;
    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int count;
    int i;
    command_t *command;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
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
                command = client_get_space_for_command (COMMAND_GLGETINTEGERV);
                command_glgetintegerv_init (command, pname, params);
                client_run_command (command);
                egl_state->state.max_combined_texture_image_units_queried = true;
                egl_state->state.max_combined_texture_image_units = *params;
            } else
                *params = egl_state->state.max_combined_texture_image_units;
            break;
        case GL_MAX_CUBE_MAP_TEXTURE_SIZE:
            if (! egl_state->state.max_cube_map_texture_size_queried) {
                command = client_get_space_for_command (COMMAND_GLGETINTEGERV);
                command_glgetintegerv_init (command, pname, params);
                client_run_command (command);
                egl_state->state.max_cube_map_texture_size = *params;
                egl_state->state.max_cube_map_texture_size_queried = true;
            } else
                *params = egl_state->state.max_cube_map_texture_size;
        break;
        case GL_MAX_FRAGMENT_UNIFORM_VECTORS:
            if (! egl_state->state.max_fragment_uniform_vectors_queried) {
                command = client_get_space_for_command (COMMAND_GLGETINTEGERV);
                command_glgetintegerv_init (command, pname, params);
                client_run_command (command);
                egl_state->state.max_fragment_uniform_vectors = *params;
                egl_state->state.max_fragment_uniform_vectors_queried = true;
            } else
                *params = egl_state->state.max_fragment_uniform_vectors;
            break;
        case GL_MAX_RENDERBUFFER_SIZE:
            if (! egl_state->state.max_renderbuffer_size_queried) {
                command = client_get_space_for_command (COMMAND_GLGETINTEGERV);
                command_glgetintegerv_init (command, pname, params);
                client_run_command (command);
                egl_state->state.max_renderbuffer_size = *params;
                egl_state->state.max_renderbuffer_size_queried = true;
            } else
                *params = egl_state->state.max_renderbuffer_size;
            break;
        case GL_MAX_TEXTURE_IMAGE_UNITS:
            if (! egl_state->state.max_texture_image_units_queried) {
                command = client_get_space_for_command (COMMAND_GLGETINTEGERV);
                command_glgetintegerv_init (command, pname, params);
                client_run_command (command);
                egl_state->state.max_texture_image_units = *params;
                egl_state->state.max_texture_image_units_queried = true;
            } else
                *params = egl_state->state.max_texture_image_units;
            break;
        case GL_MAX_VARYING_VECTORS:
            if (! egl_state->state.max_varying_vectors_queried) {
                command = client_get_space_for_command (COMMAND_GLGETINTEGERV);
                command_glgetintegerv_init (command, pname, params);
                client_run_command (command);
                egl_state->state.max_varying_vectors = *params;
                egl_state->state.max_varying_vectors_queried = true;
            } else
                *params = egl_state->state.max_varying_vectors;
            break;
        case GL_MAX_TEXTURE_SIZE:
            if (! egl_state->state.max_texture_size_queried) {
                command = client_get_space_for_command (COMMAND_GLGETINTEGERV);
                command_glgetintegerv_init (command, pname, params);
                client_run_command (command);
                egl_state->state.max_texture_size = *params;
                egl_state->state.max_texture_size_queried = true;
            } else
                *params = egl_state->state.max_texture_size;
            break;
        case GL_MAX_VERTEX_ATTRIBS:
            if (! egl_state->state.max_vertex_attribs_queried) {
                command = client_get_space_for_command (COMMAND_GLGETINTEGERV);
                command_glgetintegerv_init (command, pname, params);
                client_run_command (command);
                egl_state->state.max_vertex_attribs = *params;
                egl_state->state.max_vertex_attribs_queried = true;
            } else
                *params = egl_state->state.max_vertex_attribs;
            break;
        case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
            if (! egl_state->state.max_vertex_texture_image_units_queried) {
                command = client_get_space_for_command (COMMAND_GLGETINTEGERV);
                command_glgetintegerv_init (command, pname, params);
                client_run_command (command);
                egl_state->state.max_vertex_texture_image_units = *params;
                egl_state->state.max_vertex_texture_image_units_queried = true;
            } else
                *params = egl_state->state.max_vertex_texture_image_units;
            break;
        case GL_MAX_VERTEX_UNIFORM_VECTORS:
            if (! egl_state->state.max_vertex_uniform_vectors_queried) {
                command = client_get_space_for_command (COMMAND_GLGETINTEGERV);
                command_glgetintegerv_init (command, pname, params);
                client_run_command (command);
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
            command = client_get_space_for_command (COMMAND_GLGETINTEGERV);
            command_glgetintegerv_init (command, pname, params);
            client_run_command (command);
            egl_state->state.need_get_error = true;
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

static GLenum
caching_client_glGetError (client_t *client)
{
    GLenum error = GL_INVALID_OPERATION;
    egl_state_t *egl_state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
    
        if (! egl_state->state.need_get_error) {
            error = egl_state->state.error;
            egl_state->state.error = GL_NO_ERROR;
            return error;
        }

        command_t *command = client_get_space_for_command (COMMAND_GLGETERROR);
        command_glgeterror_init (command);
        client_run_command (command);
        error = ((command_glgeterror_t *)command)->result;

        egl_state->state.need_get_error = false;
        egl_state->state.error = GL_NO_ERROR;
    }

    return error;
}

static void
caching_client_glGetFramebufferAttachmentParameteriv (client_t *client, GLenum target,
                                                        GLenum attachment,
                                                        GLenum pname,
                                                        GLint *params)
{
    egl_state_t *egl_state;
    GLint original_params = params[0];

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
    
        if (target != GL_FRAMEBUFFER) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }
        
        command_t *command = client_get_space_for_command (COMMAND_GLGETFRAMEBUFFERATTACHMENTPARAMETERIV);
        command_glgetframebufferattachmentparameteriv_init (command,
                                                            target, 
                                                            attachment,
                                                            pname,
                                                            params);
        client_run_command (command);

        if (original_params == params[0])
            egl_state->state.need_get_error = true;
    }
}

static void
caching_client_glGetRenderbufferParameteriv (client_t *client, GLenum target,
                                              GLenum pname,
                                              GLint *params)
{
    egl_state_t *egl_state;
    GLint original_params = params[0];

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (target != GL_RENDERBUFFER) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }

        command_t *command = client_get_space_for_command (COMMAND_GLGETRENDERBUFFERPARAMETERIV);
        command_glgetrenderbufferparameteriv_init (command,
                                                   target, 
                                                   pname,
                                                   params);
        client_run_command (command);

        if (original_params == params[0])
            egl_state->state.need_get_error = true;
    }
}

static const GLubyte *
caching_client_glGetString (client_t *client, GLenum name)
{
    GLubyte *result = NULL;
    egl_state_t *egl_state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (! (name == GL_VENDOR                   || 
               name == GL_RENDERER                 ||
               name == GL_SHADING_LANGUAGE_VERSION ||
               name == GL_EXTENSIONS)) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return NULL;
        }

        command_t *command = client_get_space_for_command (COMMAND_GLGETSTRING);
        command_glgetstring_init (command, name);
        client_run_command (command);

        result = (GLubyte *)((command_glgetstring_t *)command)->result;

        if (result == 0)
            egl_state->state.need_get_error = true;
    }
    return (const GLubyte *)result;
}

static void
caching_client_glGetTexParameteriv (client_t *client, GLenum target, GLenum pname, 
                                     GLint *params)
{
    egl_state_t *egl_state;
    int active_texture_index;
    int target_index;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (! (target == GL_TEXTURE_2D       || 
               target == GL_TEXTURE_CUBE_MAP
                                             || 
               target == GL_TEXTURE_3D_OES
                                          )) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }

        if (! (pname == GL_TEXTURE_MAG_FILTER ||
               pname == GL_TEXTURE_MIN_FILTER ||
               pname == GL_TEXTURE_WRAP_S     ||
               pname == GL_TEXTURE_WRAP_T
                                              || 
               pname == GL_TEXTURE_WRAP_R_OES
                                             )) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
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
caching_client_glGetTexParameterfv (client_t *client, GLenum target, GLenum pname, GLfloat *params)
{
    GLint paramsi;

    caching_client_glGetTexParameteriv (client, target, pname, &paramsi);
    *params = paramsi;
}

static void
caching_client_glGetVertexAttribfv (client_t *client, GLuint index, GLenum pname, 
                                     GLfloat *params)
{
    egl_state_t *egl_state;

    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int count;
    int i;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
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
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }

        /* check index is too large */
        if (caching_client_glIndexIsTooLarge (client, &egl_state->state, index)) {
            return;
        }

        /* we cannot use client state */
        if (egl_state->state.vertex_array_binding) {
            command_t *command = client_get_space_for_command (COMMAND_GLGETVERTEXATTRIBFV);
            command_glgetvertexattribfv_init (command, index, pname, params);
            client_run_command (command);
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
caching_client_glGetVertexAttribiv (client_t *client, GLuint index, GLenum pname, GLint *params)
{
    GLfloat paramsf[4];
    int i;

    caching_client_glGetVertexAttribfv (client, index, pname, paramsf);

    if (pname == GL_CURRENT_VERTEX_ATTRIB) {
        for (i = 0; i < 4; i++)
            params[i] = paramsf[i];
    } else
        *params = paramsf[0];
}

static void
caching_client_glGetVertexAttribPointerv (client_t *client, GLuint index, GLenum pname, 
                                            GLvoid **pointer)
{
    egl_state_t *egl_state;

    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int count;
    int i;

    INSTRUMENT();
    
    *pointer = 0;

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
        attrib_list = &egl_state->state.vertex_attribs;
        attribs = attrib_list->attribs;
        count = attrib_list->count;

        if (pname != GL_VERTEX_ATTRIB_ARRAY_POINTER) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }

        /* XXX: check index validity */
        if (caching_client_glIndexIsTooLarge (client, &egl_state->state, index)) {
            return;
        }

        /* we cannot use client state */
        if (egl_state->state.vertex_array_binding) {
            command_t *command = client_get_space_for_command (COMMAND_GLGETVERTEXATTRIBPOINTERV);
            command_glgetvertexattribpointerv_init (command, index, pname, pointer);
            client_run_command (command);
            if (*pointer == NULL)
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
caching_client_glHint (client_t *client, GLenum target, GLenum mode)
{
    egl_state_t *egl_state;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (target == GL_GENERATE_MIPMAP_HINT &&
            egl_state->state.generate_mipmap_hint == mode)
            return;

        if ( !(mode == GL_FASTEST ||
               mode == GL_NICEST  ||
               mode == GL_DONT_CARE)) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }

        if (target == GL_GENERATE_MIPMAP_HINT)
            egl_state->state.generate_mipmap_hint = mode;

        command_t *command = client_get_space_for_command (COMMAND_GLHINT);
        command_glhint_init (command, target, mode);
        client_run_command_async (command);

        if (target != GL_GENERATE_MIPMAP_HINT)
            egl_state->state.need_get_error = true;
    }
}

static GLboolean
caching_client_glIsEnabled (client_t *client, GLenum cap)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

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
            caching_client_glSetError (client, GL_INVALID_ENUM);
            break;
        }
    }
    return result;
}

static void
caching_client_glLineWidth (client_t *client, GLfloat width)
{
    egl_state_t *egl_state;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (egl_state->state.line_width == width)
            return;

        if (width < 0) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }

        egl_state->state.line_width = width;
        command_t *command = client_get_space_for_command (COMMAND_GLLINEWIDTH);
        command_gllinewidth_init (command, width);
        client_run_command_async (command);
    }
}

static void
caching_client_glPixelStorei (client_t *client, GLenum pname, GLint param)
{
    egl_state_t *egl_state;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if ((pname == GL_PACK_ALIGNMENT                &&
             egl_state->state.pack_alignment == param) ||
            (pname == GL_UNPACK_ALIGNMENT              &&
             egl_state->state.unpack_alignment == param))
            return;

        if (! (param == 1 ||
               param == 2 ||
               param == 4 ||
               param == 8)) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }
        else if (! (pname == GL_PACK_ALIGNMENT ||
                    pname == GL_UNPACK_ALIGNMENT)) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }

        if (pname == GL_PACK_ALIGNMENT)
           egl_state->state.pack_alignment = param;
        else
           egl_state->state.unpack_alignment = param;

        command_t *command = client_get_space_for_command (COMMAND_GLPIXELSTOREI);
        command_glpixelstorei_init (command, pname, param);
        client_run_command_async (command);
    }
}

static void
caching_client_glPolygonOffset (client_t *client, GLfloat factor, GLfloat units)
{
    egl_state_t *egl_state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (egl_state->state.polygon_offset_factor == factor &&
            egl_state->state.polygon_offset_units == units)
            return;

        egl_state->state.polygon_offset_factor = factor;
        egl_state->state.polygon_offset_units = units;

        command_t *command = client_get_space_for_command (COMMAND_GLPOLYGONOFFSET);
        command_glpolygonoffset_init (command, factor, units);
        client_run_command_async (command);
    }
}

static void
caching_client_glSampleCoverage (client_t *client, GLclampf value, GLboolean invert)
{
    egl_state_t *egl_state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (value == egl_state->state.sample_coverage_value &&
            invert == egl_state->state.sample_coverage_invert)
            return;

        egl_state->state.sample_coverage_invert = invert;
        egl_state->state.sample_coverage_value = value;

        command_t *command = client_get_space_for_command (COMMAND_GLSAMPLECOVERAGE);
        command_glsamplecoverage_init (command, value, invert);
        client_run_command_async (command);
    }
}

static void
caching_client_glScissor (client_t *client, GLint x, GLint y, GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (x == egl_state->state.scissor_box[0]     &&
            y == egl_state->state.scissor_box[1]     &&
            width == egl_state->state.scissor_box[2] &&
            height == egl_state->state.scissor_box[3])
            return;

        if (width < 0 || height < 0) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }

        egl_state->state.scissor_box[0] = x;
        egl_state->state.scissor_box[1] = y;
        egl_state->state.scissor_box[2] = width;
        egl_state->state.scissor_box[3] = height;
        
        command_t *command = client_get_space_for_command (COMMAND_GLSCISSOR);
        command_glscissor_init (command, x, y, width, height);
        client_run_command_async (command);
    }
}

static void
caching_client_glStencilFuncSeparate (client_t *client, GLenum face, GLenum func,
                                       GLint ref, GLuint mask)
{
    egl_state_t *egl_state;
    bool needs_call = false;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
    
        if (! (face == GL_FRONT         ||
               face == GL_BACK          ||
               face == GL_FRONT_AND_BACK)) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
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
            caching_client_glSetError (client, GL_INVALID_ENUM);
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

    if (needs_call) {
        command_t *command = client_get_space_for_command (COMMAND_GLSTENCILFUNCSEPARATE);
        command_glstencilfuncseparate_init (command, face, func, ref, mask); 
        client_run_command_async (command);
    }
}

static void
caching_client_glStencilFunc (client_t *client, GLenum func, GLint ref, GLuint mask)
{
    INSTRUMENT();
    caching_client_glStencilFuncSeparate (client, GL_FRONT_AND_BACK, func, ref, mask);
}

static void
caching_client_glStencilMaskSeparate (client_t *client, GLenum face, GLuint mask)
{
    egl_state_t *egl_state;
    bool needs_call = false;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
    
        if (! (face == GL_FRONT         ||
               face == GL_BACK          ||
               face == GL_FRONT_AND_BACK)) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
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
    if (needs_call) {
        command_t *command = client_get_space_for_command (COMMAND_GLSTENCILMASKSEPARATE);
        command_glstencilmaskseparate_init (command, face, mask); 
        client_run_command_async (command);
    }
}

static void
caching_client_glStencilMask (client_t *client, GLuint mask)
{
    INSTRUMENT();
    caching_client_glStencilMaskSeparate (client, GL_FRONT_AND_BACK, mask);
}

static void
caching_client_glStencilOpSeparate (client_t *client, GLenum face, GLenum sfail, 
                                     GLenum dpfail, GLenum dppass)
{
    egl_state_t *egl_state;
    bool needs_call = false;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
    
        if (! (face == GL_FRONT         ||
               face == GL_BACK          ||
               face == GL_FRONT_AND_BACK)) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
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
            caching_client_glSetError (client, GL_INVALID_ENUM);
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
            caching_client_glSetError (client, GL_INVALID_ENUM);
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
            caching_client_glSetError (client, GL_INVALID_ENUM);
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

    if (needs_call) {
        command_t *command = client_get_space_for_command (COMMAND_GLSTENCILOPSEPARATE);
        command_glstencilopseparate_init (command, face, sfail, dpfail, dppass); 
        client_run_command_async (command);
    }
}

static void
caching_client_glStencilOp (client_t *client, GLenum sfail, GLenum dpfail, GLenum dppass)
{
    INSTRUMENT();

    caching_client_glStencilOpSeparate (client, GL_FRONT_AND_BACK, sfail, dpfail, dppass);
}

static void
caching_client_glTexParameteri (client_t *client, GLenum target, GLenum pname, GLint param)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    int active_texture_index;
    int target_index;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
        state = &egl_state->state;
        active_texture_index = state->active_texture - GL_TEXTURE0;

        if (! (target == GL_TEXTURE_2D       || 
               target == GL_TEXTURE_CUBE_MAP
                                             || 
               target == GL_TEXTURE_3D_OES
                                          )) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
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
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }

        if (pname == GL_TEXTURE_MAG_FILTER &&
            ! (param == GL_NEAREST ||
               param == GL_LINEAR ||
               param == GL_NEAREST_MIPMAP_NEAREST ||
               param == GL_LINEAR_MIPMAP_NEAREST  ||
               param == GL_NEAREST_MIPMAP_LINEAR  ||
               param == GL_LINEAR_MIPMAP_LINEAR)) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }
        else if (pname == GL_TEXTURE_MIN_FILTER &&
                 ! (param == GL_NEAREST ||
                    param == GL_LINEAR)) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
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
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }
        command_t *command = client_get_space_for_command (COMMAND_GLTEXPARAMETERI);
        command_gltexparameteri_init (command, target, pname, param);
        client_run_command_async (command);
    }
}

static void
caching_client_glTexParameterf (client_t *client, GLenum target, GLenum pname, GLfloat param)
{
    GLint parami = param;

    INSTRUMENT();
    caching_client_glTexParameteri (client, target, pname, parami);
}

void
caching_client_glUniformfv (client_t *client, int i, GLint location,
                GLsizei count, const GLfloat *value)
{
    egl_state_t *egl_state;
    command_t *command = NULL;

    if (! caching_client_glIsValidContext (client))
        return;

    switch (i) {
    case 1:
        command = client_get_space_for_command (COMMAND_GLUNIFORM1FV);
        command_gluniform1fv_init (command, location, count, value);
        break;
    case 2:
        command = client_get_space_for_command (COMMAND_GLUNIFORM2FV);
        command_gluniform2fv_init (command, location, count, value);
        break;
    case 3:
        command = client_get_space_for_command (COMMAND_GLUNIFORM3FV);
        command_gluniform3fv_init (command, location, count, value);
        break;
    default:
        command = client_get_space_for_command (COMMAND_GLUNIFORM4FV);
        command_gluniform4fv_init (command, location, count, value);
        break;
    }

    if (command)
        client_run_command_async (command);

    egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
    /* XXX: should we set this? */
    //egl_state->state.need_get_error = true;
}

static void
caching_client_glUniform1fv (client_t *client, GLint location, GLsizei count, 
                            const GLfloat *value)
{
    INSTRUMENT();
    caching_client_glUniformfv (client, 1, location, count, value);
}

static void
caching_client_glUniform2fv (client_t *client, GLint location, GLsizei count, 
                            const GLfloat *value)
{
    INSTRUMENT();
    caching_client_glUniformfv (client, 2, location, count, value);
}

static void
caching_client_glUniform3fv (client_t *client, GLint location, GLsizei count, 
                            const GLfloat *value)
{
    INSTRUMENT();
    caching_client_glUniformfv (client, 3, location, count, value);
}

static void
caching_client_glUniform4fv (client_t *client, GLint location, GLsizei count, 
                            const GLfloat *value)
{
    INSTRUMENT();
    caching_client_glUniformfv (client, 4, location, count, value);
}

 void
caching_client_glUniformiv (client_t *client,
                int i,
                GLint location,
                GLsizei count,
                const GLint *value)
{
    egl_state_t *egl_state;
    command_t *command = NULL;

    if (! caching_client_glIsValidContext (client))
        return;

    switch (i) {
    case 1:
        command = client_get_space_for_command (COMMAND_GLUNIFORM1IV);
        command_gluniform1iv_init (command, location, count, value);
        break;
    case 2:
        command = client_get_space_for_command (COMMAND_GLUNIFORM2IV);
        command_gluniform2iv_init (command, location, count, value);
        break;
    case 3:
        command = client_get_space_for_command (COMMAND_GLUNIFORM3IV);
        command_gluniform3iv_init (command, location, count, value);
        break;
    default:
        command = client_get_space_for_command (COMMAND_GLUNIFORM4IV);
        command_gluniform4iv_init (command, location, count, value);
        break;
    }
    
    if (command)
        client_run_command_async (command);

    egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
    /* XXX: should we set this? */
    //egl_state->state.need_get_error = true;
}

static void
caching_client_glUniform1iv (client_t *client, GLint location, GLsizei count, 
                            const GLint *value)
{
    caching_client_glUniformiv (client, 1, location, count, value);
}

static void
caching_client_glUniform2iv (client_t *client, GLint location, GLsizei count, 
                            const GLint *value)
{
    caching_client_glUniformiv (client, 2, location, count, value);
}

static void
caching_client_glUniform3iv (client_t *client, GLint location, GLsizei count, 
                            const GLint *value)
{
    caching_client_glUniformiv (client, 3, location, count, value);
}

static void
caching_client_glUniform4iv (client_t *client, GLint location, GLsizei count, 
                            const GLint *value)
{
    caching_client_glUniformiv (client, 4, location, count, value);
}

static void
caching_client_glUniformMatrix2fv (client_t *client, GLint location,
				   GLsizei count, GLboolean transpose,
				   const GLfloat *value)
{
    //egl_state_t *egl_state;

    INSTRUMENT();

    if (! caching_client_glIsValidContext (client))
        return;
    
    //egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
 
    /* This is really pitiful as we cannot completely detect whether
     * this call will generate error or not.  We can detect all 
     * error conditions, except for unform type mismatch
     */ 
    /*
    if (egl_state->current_program <= 0) {
        caching_client_glSetError (client, GL_INVALID_OPERATION);
        return;
    }

    if (transpose != GL_FALSE) {
	caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    if (location <= 0 || location_is_not_valid (location)) {
        caching_client_glSetError (client, GL_INVALID_OPERATION);
        return;
    }*/

    if (count < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return; 
    }

    command_t *command = client_get_space_for_command (COMMAND_GLUNIFORMMATRIX2FV);
    command_gluniformmatrix2fv_init (command, location, count, transpose, value);
    
    client_run_command_async (command);

    /* FIXME: should we set this?  it has large impact on performance */
    //egl_state->state.need_get_error = true;
}

static void
caching_client_glUniformMatrix3fv (client_t *client, GLint location,
				   GLsizei count, GLboolean transpose,
				   const GLfloat *value)
{
    //egl_state_t *egl_state;

    INSTRUMENT();

    if (! caching_client_glIsValidContext (client))
        return;

    //egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
    /* This is really pitiful as we cannot completely detect whether
     * this call will generate error or not.  We can detect all 
     * error conditions, except for unform type mismatch
     */ 
    /*
    if (egl_state->current_program <= 0) {
        caching_client_glSetError (client, GL_INVALID_OPERATION);
        return;
    }

    if (transpose != GL_FALSE) {
	caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    if (location <= 0 || location_is_not_valid (location)) {
        caching_client_glSetError (client, GL_INVALID_OPERATION);
        return;
    } */

    if (count < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return; 
    }
    command_t *command = client_get_space_for_command (COMMAND_GLUNIFORMMATRIX3FV);
    command_gluniformmatrix3fv_init (command, location, count, transpose, value);
    
    client_run_command_async (command);

    /* FIXME: should we set this?  it has large impact on performance */
    //egl_state->state.need_get_error = true;
}

static void
caching_client_glUniformMatrix4fv (client_t *client, GLint location,
				   GLsizei count, GLboolean transpose,
				   const GLfloat *value)
{
    //egl_state_t *egl_state;

    INSTRUMENT();

    if (! caching_client_glIsValidContext (client))
        return;

    //egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
    /* This is really pitiful as we cannot completely detect whether
     * this call will generate error or not.  We can detect all 
     * error conditions, except for unform type mismatch
     */ 
    /*
    if (egl_state->current_program <= 0) {
        caching_client_glSetError (client, GL_INVALID_OPERATION);
        return;
    }

    if (transpose != GL_FALSE) {
	caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    if (location <= 0 || location_is_not_valid (location)) {
        caching_client_glSetError (client, GL_INVALID_OPERATION);
        return;
    } */

    if (count < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return; 
    }
    command_t *command = client_get_space_for_command (COMMAND_GLUNIFORMMATRIX4FV);
    command_gluniformmatrix4fv_init (command, location, count, transpose, value);
    
    client_run_command_async (command);

    /* FIXME: should we set this?  it has large impact on performance */
    //egl_state->state.need_get_error = true;
}

static void
caching_client_glUseProgram (client_t *client, GLuint program)
{
    egl_state_t *egl_state;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
    
        if (egl_state->state.current_program == program)
            return;
        if (program < 0) {
            caching_client_glSetError (client, GL_INVALID_OPERATION);
            return;
        }
        command_t *command = client_get_space_for_command (COMMAND_GLUSEPROGRAM);
        command_gluseprogram_init (command, program);
    
        client_run_command_async (command);

        /* FIXME: this maybe not right because this program may be invalid
         * object, we save here to save time in glGetError() */
        egl_state->state.current_program = program;
        /* FIXME: do we need to have this ? */
        egl_state->state.need_get_error = true;
    }
}

static void
caching_client_glVertexAttrib1f (client_t *client, GLuint index, GLfloat v0)
{
    egl_state_t *egl_state;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
    
        if (caching_client_glIndexIsTooLarge (client, &egl_state->state, index)) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }
        
        command_t *command = client_get_space_for_command (COMMAND_GLVERTEXATTRIB1F);
        command_glvertexattrib1f_init (command, index, v0);
    
        client_run_command_async (command);

    }
}

static void
caching_client_glVertexAttrib2f (client_t *client, GLuint index, GLfloat v0, GLfloat v1)
{
    egl_state_t *egl_state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
    
        if (caching_client_glIndexIsTooLarge (client, &egl_state->state, index)) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }
        
        command_t *command = client_get_space_for_command (COMMAND_GLVERTEXATTRIB2F);
        command_glvertexattrib2f_init (command, index, v0, v1);
    
        client_run_command_async (command);

    }
}

static void
caching_client_glVertexAttrib3f (client_t *client, GLuint index, GLfloat v0, 
                                 GLfloat v1, GLfloat v2)
{
    egl_state_t *egl_state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
    
        if (caching_client_glIndexIsTooLarge (client, &egl_state->state, index)) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }

        command_t *command = client_get_space_for_command (COMMAND_GLVERTEXATTRIB3F);
        command_glvertexattrib3f_init (command, index, v0, v1, v2);
    
        client_run_command_async (command);
    }
}

static void
caching_client_glVertexAttrib4f (client_t *client, GLuint index, GLfloat v0, GLfloat v1, 
                                 GLfloat v2, GLfloat v3)
{
    egl_state_t *egl_state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
    
        if (caching_client_glIndexIsTooLarge (client, &egl_state->state, index)) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }

        command_t *command = client_get_space_for_command (COMMAND_GLVERTEXATTRIB4F);
        command_glvertexattrib4f_init (command, index, v0, v1, v2, v3);
    
        client_run_command_async (command);
    }
}

 void
caching_client_glVertexAttribfv (client_t *client, int i, GLuint index, const GLfloat *v)
{
    egl_state_t *egl_state;
    command_t *command = NULL;
    
    if (! caching_client_glIsValidContext (client))
        return;
    
    egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

    if (caching_client_glIndexIsTooLarge (client, &egl_state->state, index)) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    switch (i) {
        case 1:
            command = client_get_space_for_command (COMMAND_GLVERTEXATTRIB1FV);
            command_glvertexattrib1fv_init (command, index, v);
            break;
        case 2:
            command = client_get_space_for_command (COMMAND_GLVERTEXATTRIB2FV);
            command_glvertexattrib2fv_init (command, index, v);
            break;
        case 3:
            command = client_get_space_for_command (COMMAND_GLVERTEXATTRIB3FV);
            command_glvertexattrib3fv_init (command, index, v);
            break;
        default:
            command = client_get_space_for_command (COMMAND_GLVERTEXATTRIB4FV);
            command_glvertexattrib4fv_init (command, index, v);
            break;
    }

    if (command)
        client_run_command_async (command);
}

static void
caching_client_glVertexAttrib1fv (client_t *client, GLuint index, const GLfloat *v)
{
    INSTRUMENT();
    caching_client_glVertexAttribfv (client, 1, index, v);
}

static void
caching_client_glVertexAttrib2fv (client_t *client, GLuint index, const GLfloat *v)
{
    INSTRUMENT();
    caching_client_glVertexAttribfv (client, 2, index, v);
}

static void
caching_client_glVertexAttrib3fv (client_t *client, GLuint index, const GLfloat *v)
{
    INSTRUMENT();
    caching_client_glVertexAttribfv (client, 3, index, v);
}

static void
caching_client_glVertexAttrib4fv (client_t *client, GLuint index, const GLfloat *v)
{
    INSTRUMENT();
    caching_client_glVertexAttribfv (client, 4, index, v);
}

static void
caching_client_glVertexAttribPointer (client_t *client, GLuint index, GLint size,
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

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
        state = &egl_state->state;
        attrib_list = &state->vertex_attribs;
        attribs = attrib_list->attribs;
        count = attrib_list->count;
        
        if (! (type == GL_BYTE                 ||
               type == GL_UNSIGNED_BYTE        ||
               type == GL_SHORT                ||
               type == GL_UNSIGNED_SHORT       ||
               type == GL_FLOAT)) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }
        else if (size > 4 || size < 1 || stride < 0) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }

        /* check max_vertex_attribs */
        if (caching_client_glIndexIsTooLarge (client, &egl_state->state, index)) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }
        
        if (egl_state->state.vertex_array_binding) {
            command_t *command = client_get_space_for_command (COMMAND_GLVERTEXATTRIBPOINTER);
            command_glvertexattribpointer_init (command, index, size,
                                                type, normalized, stride,
                                                pointer);
            client_run_command_async (command);
            /* FIXME: Do we need set this flag? */
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
            command_t *command = client_get_space_for_command (COMMAND_GLVERTEXATTRIBPOINTER);
            command_glvertexattribpointer_init (command, index, size,
                                                type, normalized, stride,
                                                pointer);
            client_run_command_async (command);
            /* FIXME: Do we need set this flag? */
            egl_state->state.need_get_error = true;
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
caching_client_glViewport (client_t *client, GLint x, GLint y, GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
    
        if (egl_state->state.viewport[0] == x     &&
            egl_state->state.viewport[1] == y     &&
            egl_state->state.viewport[2] == width &&
            egl_state->state.viewport[3] == height)
            return;

        if (x < 0 || y < 0) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }

        egl_state->state.viewport[0] = x;
        egl_state->state.viewport[1] = y;
        egl_state->state.viewport[2] = width;
        egl_state->state.viewport[3] = height;
            
        command_t *command = client_get_space_for_command (COMMAND_GLVIEWPORT);
        command_glviewport_init (command, x, y, width, height);
        client_run_command_async (command);
    }
}
/* end of GLES2 core profile */

static void
caching_client_glEGLImageTargetTexture2DOES (client_t *client, GLenum target, GLeglImageOES image)
{
    egl_state_t *egl_state;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
    
        if (target != GL_TEXTURE_2D) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }
        
        command_t *command = client_get_space_for_command (COMMAND_GLEGLIMAGETARGETTEXTURE2DOES);
        command_gleglimagetargettexture2does_init (command, target, image);
        client_run_command_async (command);

        egl_state->state.need_get_error = true;
    }
}

static void* 
caching_client_glMapBufferOES (client_t *client, GLenum target, GLenum access)
{
    egl_state_t *egl_state;
    void *result = NULL;

    INSTRUMENT();
    
    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (access != GL_WRITE_ONLY_OES) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return result;
        }
        else if (! (target == GL_ARRAY_BUFFER ||
                    target == GL_ELEMENT_ARRAY_BUFFER)) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return result;
        }

        command_t *command = client_get_space_for_command (COMMAND_GLMAPBUFFEROES);
        command_glmapbufferoes_init (command, target, access);
        client_run_command (command);
        result = ((command_glmapbufferoes_t *)command)->result;
     
        if (result == NULL)
            egl_state->state.need_get_error = true;
    }
    return result;
}

static GLboolean
caching_client_glUnmapBufferOES (client_t *client, GLenum target)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (! (target == GL_ARRAY_BUFFER ||
               target == GL_ELEMENT_ARRAY_BUFFER)) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return result;
        }

        command_t *command = client_get_space_for_command (COMMAND_GLUNMAPBUFFEROES);
        command_glunmapbufferoes_init (command, target);
        client_run_command (command);
        result = ((command_glunmapbufferoes_t *)command)->result;
        if (result != GL_TRUE)
            egl_state->state.need_get_error = true;
    }
    return result;
}

static void
caching_client_glGetBufferPointervOES (client_t *client, GLenum target, GLenum pname, GLvoid **params)
{
    egl_state_t *egl_state;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (! (target == GL_ARRAY_BUFFER ||
               target == GL_ELEMENT_ARRAY_BUFFER)) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }

        command_t *command = client_get_space_for_command (COMMAND_GLGETBUFFERPOINTERVOES);
        command_glgetbufferpointervoes_init (command, target, pname, params);
        client_run_command (command);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_client_glFramebufferTexture3DOES (client_t *client, GLenum target, GLenum attachment,
                                GLenum textarget, GLuint texture,
                                GLint level, GLint zoffset)
{
    egl_state_t *egl_state;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (target != GL_FRAMEBUFFER) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }

        command_t *command = client_get_space_for_command (COMMAND_GLFRAMEBUFFERTEXTURE3DOES);
        command_glframebuffertexture3does_init (command, target,
                                                attachment, textarget,
                                                texture,
                                                level, zoffset);
        client_run_command_async (command);
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
 * we pass them to client.
 */
static void
caching_client_glBindVertexArrayOES (client_t *client, GLuint array)
{
    egl_state_t *egl_state;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        if (egl_state->state.vertex_array_binding == array)
            return;

        command_t *command = client_get_space_for_command (COMMAND_GLBINDVERTEXARRAYOES);
        command_glbindvertexarrayoes_init (command, array);
        client_run_command_async (command);
        egl_state->state.need_get_error = true;
        /* FIXME: should we save this ? */
        egl_state->state.vertex_array_binding = array;
    }
}

static void
caching_client_glDeleteVertexArraysOES (client_t *client, GLsizei n, const GLuint *arrays)
{
    egl_state_t *egl_state;
    int i;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
        
        if (n <= 0) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }
        command_t *command = client_get_space_for_command (COMMAND_GLDELETEVERTEXARRAYSOES);
        command_gldeletevertexarraysoes_init (command, n, arrays);
        client_run_command_async (command);

        /* matching vertex_array_binding ? */
        for (i = 0; i < n; i++) {
            if (arrays[i] == egl_state->state.vertex_array_binding) {
                egl_state->state.vertex_array_binding = 0;
                break;
            }
        }
    }
}

static void
caching_client_glGenVertexArraysOES (client_t *client, GLsizei n, GLuint *arrays)
{
    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        if (n <= 0) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }

        command_t *command = client_get_space_for_command (COMMAND_GLGENVERTEXARRAYSOES);
        command_glgenvertexarraysoes_init (command, n, arrays);
        client_run_command (command);
    }
}

static GLboolean
caching_client_glIsVertexArrayOES (client_t *client, GLuint array)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
        
        command_t *command = client_get_space_for_command (COMMAND_GLISVERTEXARRAYOES);
        command_glisvertexarrayoes_init (command, array);
        client_run_command (command);
        result = ((command_glisvertexarrayoes_t *)command)->result;

        if (result == GL_FALSE && 
            egl_state->state.vertex_array_binding == array)
            egl_state->state.vertex_array_binding = 0;
    }
    return result;
}

static void
caching_client_glDiscardFramebufferEXT (client_t *client,
                             GLenum target, GLsizei numAttachments,
                             const GLenum *attachments)
{
    int i;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        if (target != GL_FRAMEBUFFER) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }

        for (i = 0; i < numAttachments; i++) {
            if (! (attachments[i] == GL_COLOR_ATTACHMENT0  ||
                   attachments[i] == GL_DEPTH_ATTACHMENT   ||
                   attachments[i] == GL_STENCIL_ATTACHMENT ||
                   attachments[i] == GL_COLOR_EXT          ||
                   attachments[i] == GL_DEPTH_EXT          ||
                   attachments[i] == GL_STENCIL_EXT)) {
                caching_client_glSetError (client, GL_INVALID_ENUM);
                return;
            }
        }
        command_t *command = client_get_space_for_command (COMMAND_GLDISCARDFRAMEBUFFEREXT);
        command_gldiscardframebufferext_init (command, target,
                                              numAttachments,
                                              attachments);
        client_run_command_async (command);
    }
}

static void
caching_client_glMultiDrawArraysEXT (client_t *client,
                                     GLenum mode,
                                     GLint *first,
                                     GLsizei *count,
                                     GLsizei primcount)
{
    /* FIXME: things a little complicated here.  We cannot simply
     * turn this into a sync call, because, we have not called 
     * glVertexAttribPointer() yet
     */
    caching_client_glSetError (client, GL_INVALID_OPERATION);

    /* not implemented */
}

void 
caching_client_glMultiDrawElementsEXT (client_t *client, GLenum mode, const GLsizei *count, GLenum type,
                             const GLvoid **indices, GLsizei primcount)
{
    /* FIXME: things a little complicated here.  We cannot simply
     * turn this into a sync call, because, we have not called 
     * glVertexAttribPointer() yet
     */
    caching_client_glSetError (client, GL_INVALID_OPERATION);

    /* not implemented */
}

static void
caching_client_glFramebufferTexture2DMultisampleEXT (client_t *client, GLenum target, 
                                            GLenum attachment,
                                            GLenum textarget, 
                                            GLuint texture,
                                            GLint level, GLsizei samples)
{
    egl_state_t *egl_state;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
        
        if (target != GL_FRAMEBUFFER) {
           caching_client_glSetError (client, GL_INVALID_ENUM);
           return;
        }
        
        command_t *command = client_get_space_for_command (COMMAND_GLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXT);
        command_glframebuffertexture2dmultisampleext_init (command, target,
                                                           attachment,
                                                           textarget,
                                                           texture,
                                                           level,
                                                           samples);
        client_run_command_async (command);

        egl_state->state.need_get_error = true;
    }
}

static void
caching_client_glFramebufferTexture2DMultisampleIMG (client_t *client, GLenum target, 
                                                     GLenum attachment,
                                                     GLenum textarget, 
                                                     GLuint texture,
                                                     GLint level, GLsizei samples)
{
    egl_state_t *egl_state;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
        
        if (target != GL_FRAMEBUFFER) {
           caching_client_glSetError (client, GL_INVALID_ENUM);
           return;
        }

        command_t *command = client_get_space_for_command (COMMAND_GLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMG);
        command_glframebuffertexture2dmultisampleimg_init (command, target,
                                                           attachment,
                                                           textarget,
                                                           texture,
                                                           level,
                                                           samples);
        client_run_command_async (command);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_client_glDeleteFencesNV (client_t *client, GLsizei n, const GLuint *fences)
{
    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        if (n <= 0) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }
    
        command_t *command = client_get_space_for_command (COMMAND_GLDELETEFENCESNV);
        command_gldeletefencesnv_init (command, n, fences);
        client_run_command_async (command);
    }
}

static void
caching_client_glGenFencesNV (client_t *client, GLsizei n, GLuint *fences)
{
    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
    
        if (n <= 0) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }
    
        command_t *command = client_get_space_for_command (COMMAND_GLGENFENCESNV);
        command_glgenfencesnv_init (command, n, fences);
        client_run_command (command);
    }
}

static GLboolean
caching_client_glIsFenceNV (client_t *client, GLuint fence)
{
    GLboolean result = GL_FALSE;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
    
        command_t *command = client_get_space_for_command (COMMAND_GLISFENCENV);
        command_glisfencenv_init (command, fence);
        client_run_command (command);
        result = ((command_glisfencenv_t *)command)->result;
    }
    return result;
}

static GLboolean
caching_client_glTestFenceNV (client_t *client, GLuint fence)
{
    GLboolean result = GL_FALSE;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        command_t *command = client_get_space_for_command (COMMAND_GLTESTFENCENV);
        command_gltestfencenv_init (command, fence);
        client_run_command (command);
        result = ((command_gltestfencenv_t *)command)->result;
    }
    return result;
}

static void 
caching_client_glGetFenceivNV (client_t *client, GLuint fence, GLenum pname, int *params)
{
    egl_state_t *egl_state;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
    
        command_t *command = client_get_space_for_command (COMMAND_GLGETFENCEIVNV);
        command_glgetfenceivnv_init (command, fence, pname, params);
        client_run_command (command);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_client_glSetFenceNV (client_t *client, GLuint fence, GLenum condition)
{
    egl_state_t *egl_state;

    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
    
        command_t *command = client_get_space_for_command (COMMAND_GLSETFENCENV);
        command_glsetfencenv_init (command, fence, condition);
        client_run_command_async (command);
        egl_state->state.need_get_error = true;
    }
}

static void
caching_client_glCoverageOperationNV (client_t *client, GLenum operation)
{
    INSTRUMENT();

    if (caching_client_glIsValidContext (client)) {
    
        if (! (operation == GL_COVERAGE_ALL_FRAGMENTS_NV  ||
               operation == GL_COVERAGE_EDGE_FRAGMENTS_NV ||
               operation == GL_COVERAGE_AUTOMATIC_NV)) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }
        command_t *command = client_get_space_for_command (COMMAND_GLCOVERAGEOPERATIONNV);
        command_glcoverageoperationnv_init (command, operation);
        client_run_command_async (command);
    }
}

static EGLBoolean
caching_client_eglInitialize (client_t *client,
                              EGLDisplay display,
                              EGLint *major,
                              EGLint *minor)
{
    EGLBoolean result = EGL_FALSE;

    INSTRUMENT();

    command_t *command = client_get_space_for_command (COMMAND_EGLINITIALIZE);
    command_eglinitialize_init (command, display, major, minor);
    client_run_command (command);
    result = ((command_eglinitialize_t *)command)->result;

    /* FIXME: Do we need this, probably not */
    /*if (result == EGL_TRUE)
        _caching_client_initialize (display);
    */
    return result;
}

static EGLBoolean
caching_client_eglTerminate (client_t *client,
                             EGLDisplay display)
{
    EGLBoolean result = EGL_FALSE;

    INSTRUMENT();

    command_t *command = client_get_space_for_command (COMMAND_EGLTERMINATE);
    command_eglterminate_init (command, display);
    client_run_command (command);

    result = ((command_eglterminate_t *)command)->result;

    if (result == EGL_TRUE) {
        /* XXX: remove egl_state structure */
        _caching_client_terminate (client, display);
    }

    return result;
}

static EGLBoolean
caching_client_eglDestroySurface (client_t *client,
                                  EGLDisplay display,
                                  EGLSurface surface)
{
    EGLBoolean result = EGL_FALSE;

    INSTRUMENT();

    if (!CACHING_CLIENT(client)->active_state)
        return result;

    command_t *command = client_get_space_for_command (COMMAND_EGLDESTROYSURFACE);
    command_egldestroysurface_init (command, display, surface);
    client_run_command (command);

    result = ((command_egldestroysurface_t *)command)->result;

    if (result == EGL_TRUE) {
        /* update gl states */
        _caching_client_destroy_surface (client, display, surface);
    }

    return result;
}

static EGLBoolean
caching_client_eglReleaseThread (client_t *client)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t *egl_state;

    INSTRUMENT();

    command_t *command = client_get_space_for_command (COMMAND_EGLRELEASETHREAD);
    command_eglreleasethread_init (command);
    client_run_command (command);

    result = ((command_eglreleasethread_t *)command)->result;

    if (result == EGL_TRUE) {
        if (! CACHING_CLIENT(client)->active_state)
            return result;

        egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

        _caching_client_make_current (client,
                                      egl_state->display,
                                      EGL_NO_SURFACE,
                                      EGL_NO_SURFACE,
                                      EGL_NO_CONTEXT);
    }

    return result;
}

static EGLBoolean
caching_client_eglDestroyContext (client_t *client,
                                  EGLDisplay dpy,
                                  EGLContext ctx)
{
    EGLBoolean result = GL_FALSE;

    INSTRUMENT();

    command_t *command = client_get_space_for_command (COMMAND_EGLDESTROYCONTEXT);
    command_egldestroycontext_init (command, dpy, ctx);
    client_run_command (command);

    result = ((command_eglreleasethread_t *)command)->result;

    if (result == GL_TRUE) {
        _caching_client_destroy_context (dpy, ctx, CACHING_CLIENT(client)->active_state);
    }

    return result;
}

static EGLContext
caching_client_eglGetCurrentContext (client_t *client)
{
    egl_state_t *state;

    INSTRUMENT();

    if (!CACHING_CLIENT(client)->active_state)
        return EGL_NO_CONTEXT;

    state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
    return state->context;
}

static EGLDisplay
caching_client_eglGetCurrentDisplay (client_t *client)
{
    egl_state_t *state;
    
    INSTRUMENT();

    if (!CACHING_CLIENT(client)->active_state)
        return EGL_NO_DISPLAY;

    state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
    return state->display;
}

static EGLSurface
caching_client_eglGetCurrentSurface (client_t *client,
                          EGLint readdraw)
{
    egl_state_t *state;
    EGLSurface surface = EGL_NO_SURFACE;
    
    INSTRUMENT();

    if (!CACHING_CLIENT(client)->active_state)
        return EGL_NO_SURFACE;

    state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

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
caching_client_eglSwapBuffers (client_t *client,
                   EGLDisplay display,
                   EGLSurface surface)
{
    EGLBoolean result = EGL_TRUE;
    egl_state_t *state;

    INSTRUMENT();

    if (!CACHING_CLIENT(client)->active_state)
        return EGL_FALSE;
    
    state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;

    if (! (state->display == display &&
           state->drawable == surface)) 
        return EGL_FALSE;
    
    /* XXX: optimization - we don't wait for it to return */
    command_t *command = client_get_space_for_command (COMMAND_EGLSWAPBUFFERS);
    command_eglswapbuffers_init (command, display, surface);
    client_run_command_async (command);

    return result;
}


static EGLBoolean 
caching_client_eglMakeCurrent (client_t *client,
                               EGLDisplay display,
                               EGLSurface draw,
                               EGLSurface read,
                               EGLContext ctx) 
{
    EGLBoolean result = EGL_FALSE;
    link_list_t *exist = NULL;
    bool found = false;
    command_t *command = NULL;

    INSTRUMENT();

    if (!CACHING_CLIENT(client)->active_state) {
        if (display == EGL_NO_DISPLAY || ctx == EGL_NO_CONTEXT)
            return EGL_TRUE;
        else {
            /* we are switch from no context to a context */
            found = _match (display, draw, read, ctx, &exist);
        }
    }
    else {
        egl_state_t *egl_state = (egl_state_t *)CACHING_CLIENT(client)->active_state->data;
        if (display == EGL_NO_DISPLAY || ctx == EGL_NO_CONTEXT) {
            /* we are switching from valid context to no context */
            command_t *command = client_get_space_for_command (COMMAND_EGLMAKECURRENT);
            command_eglmakecurrent_init (command, display, draw, read, ctx);
            client_run_command_async (command);
            /* update cache */
            _caching_client_make_current (client, display, draw, read, ctx);
            return EGL_TRUE;
        }
        else if (egl_state->display == display &&
                 egl_state->context == ctx) {
            if (egl_state->drawable == draw &&
                egl_state->readable == read)
                return EGL_TRUE;
            else {
                found = true;
                exist = CACHING_CLIENT(client)->active_state;
            }
        }
    }
    if (! found) {
        /* look for existing */
        found = _match (display, draw, read, ctx, &exist);
    }
    if (found == true) {
        /* set active to exist, tell client about it */
        if (CACHING_CLIENT(client)->active_state) {
            egl_state_t *egl_state = (egl_state_t *)CACHING_CLIENT(client)->active_state->data;
            egl_state->active = false;
        }
        CACHING_CLIENT(client)->active_state = exist;

        /* call real makeCurrent */
        command_t *command = client_get_space_for_command (COMMAND_EGLMAKECURRENT);
        command_eglmakecurrent_init (command, display, draw, read, ctx);
        client_run_command_async (command);
        /* update cache */
        _caching_client_make_current (client, display, draw, read, ctx);
        return EGL_TRUE;
    }

    /* We could not find in the saved state, we don't know whether
     * parameters are valid or not 
     */
    command = client_get_space_for_command (COMMAND_EGLMAKECURRENT);
    command_eglmakecurrent_init (command, display, draw, read, ctx);
    client_run_command (command);
    result = ((command_eglmakecurrent_t *)command)->result;
    if (result == EGL_TRUE) {
        _caching_client_make_current (client, display, draw, read, ctx);
    }
    return result;
}

/* start of eglext.h */
/* end of eglext.h */
/* we specify those passthrough GL APIs that needs to set need_get_error */
static void caching_client_command_post_hook(client_t *client, command_t *command)
{

    switch (command->type) {
    case COMMAND_GLATTACHSHADER:
    case COMMAND_GLBINDATTRIBLOCATION:
    case COMMAND_GLBUFFERDATA:
    case COMMAND_GLBUFFERSUBDATA:
    case COMMAND_GLCOMPILESHADER:
    case COMMAND_GLCOMPRESSEDTEXIMAGE2D:
    case COMMAND_GLCOMPRESSEDTEXSUBIMAGE2D:
    case COMMAND_GLCOPYTEXIMAGE2D:
    case COMMAND_GLCOPYTEXSUBIMAGE2D:
    case COMMAND_GLDELETEPROGRAM:
    case COMMAND_GLDELETESHADER:
    case COMMAND_GLDETACHSHADER:
    case COMMAND_GLGETBUFFERPARAMETERIV:
    case COMMAND_GLGETPROGRAMINFOLOG:
    case COMMAND_GLGETSHADERIV:
    case COMMAND_GLGETSHADERPRECISIONFORMAT:
    case COMMAND_GLGETSHADERINFOLOG:
    case COMMAND_GLGETSHADERSOURCE:
    case COMMAND_GLGETUNIFORMFV:
    case COMMAND_GLGETUNIFORMIV:
    case COMMAND_GLLINKPROGRAM:
    case COMMAND_GLVALIDATEPROGRAM:
    //case COMMAND_GLGETPROGRAMINFOLOG:
    //case COMMAND_GLGETPROGRAMIV:
    //case COMMAND_GLUNIFORM1F:
    //case COMMAND_GLUNIFORM1I:
    //case COMMAND_GLUNIFORM2F:
    //case COMMAND_GLUNIFORM2I:
    //case COMMAND_GLUNIFORM3F:
    //case COMMAND_GLUNIFORM3I:
    //case COMMAND_GLUNIFORM4F:
    //case COMMAND_GLUNIFORM4I:
    //case COMMAND_GLGETUNIFORMIV:
    //case COMMAND_GLGETUNIFORMFV:
    case COMMAND_GLREADPIXELS:
    case COMMAND_GLRELEASESHADERCOMPILER:
    case COMMAND_GLRENDERBUFFERSTORAGE:
    case COMMAND_GLSHADERBINARY:
    case COMMAND_GLSHADERSOURCE:
    case COMMAND_GLTEXIMAGE2D:
    case COMMAND_GLTEXSUBIMAGE2D:
    //case COMMAND_GLGETPROGRAMBINARYOES:
    case COMMAND_GLPROGRAMBINARYOES:
    case COMMAND_GLTEXIMAGE3DOES:
    case COMMAND_GLTEXSUBIMAGE3DOES:
    case COMMAND_GLCOPYTEXSUBIMAGE3DOES:
    case COMMAND_GLCOMPRESSEDTEXIMAGE3DOES:
    case COMMAND_GLCOMPRESSEDTEXSUBIMAGE3DOES:
    case COMMAND_GLBEGINPERFMONITORAMD:
    case COMMAND_GLGETPERFMONITORGROUPSAMD:
    case COMMAND_GLGETPERFMONITORCOUNTERSAMD:
    case COMMAND_GLGETPERFMONITORGROUPSTRINGAMD:
    case COMMAND_GLGETPERFMONITORCOUNTERSTRINGAMD:
    case COMMAND_GLGETPERFMONITORCOUNTERINFOAMD:
    case COMMAND_GLGENPERFMONITORSAMD:
    case COMMAND_GLENDPERFMONITORAMD:
    case COMMAND_GLDELETEPERFMONITORSAMD:
    case COMMAND_GLSELECTPERFMONITORCOUNTERSAMD:
    case COMMAND_GLGETPERFMONITORCOUNTERDATAAMD:
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
    case COMMAND_GLEXTTEXOBJECTSTATEOVERRIDEIQCOM:
    case COMMAND_GLGETDRIVERCONTROLSTRINGQCOM:
    case COMMAND_GLEXTGETTEXLEVELPARAMETERIVQCOM:
    case COMMAND_GLEXTGETTEXSUBIMAGEQCOM:
    case COMMAND_GLEXTGETBUFFERPOINTERVQCOM:
    case COMMAND_GLEXTISPROGRAMBINARYQCOM:
    case COMMAND_GLEXTGETPROGRAMBINARYSOURCEQCOM:
    case COMMAND_GLSTARTTILINGQCOM:
    case COMMAND_GLENDTILINGQCOM: {
        egl_state_t *egl_state = (egl_state_t *) CACHING_CLIENT(client)->active_state->data;
        egl_state->state.need_get_error = true;
        break;
    }
    default:
        break;
    }
}
