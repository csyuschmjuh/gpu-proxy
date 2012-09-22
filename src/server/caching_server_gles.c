/* This file implements the server thread side of GLES2 functions.  Thses
 * functions are called by the command buffer.
 *
 * It references to the server thread side of active_state.
 * if active_state is NULL or there is no symbol for the corresponding
 * gl functions, the cached error state is set to GL_INVALID_OPERATION
 * if the cached error has not been set to one of the errors.
 */

/* This file implements the server thread side of egl functions.
 * After the server thread reads the command buffer, if it is 
 * egl calls, it is routed to here.
 *
 * It keeps two global variables for all server threads:
 * (1) dispatch - a dispatch table to real egl and gl calls. It 
 * is initialized during eglGetDisplay() -> _egl_get_display()
 * (2) server_states - this is a double-linked structure contains
 * all active and inactive egl states.  When a client switches context,
 * the previous egl_state is set to be inactive and thus is subject to
 * be destroyed during _egl_terminate(), _egl_destroy_context() and
 * _egl_release_thread()  The inactive context's surface can also be 
 * destroyed by _egl_destroy_surface().
 * (3) active_state - this is the pointer to the current active state.
 */

#include "config.h"
#include "caching_server_private.h"

#include "dispatch_private.h"
#include "egl_states.h"
#include "server.h"
#include "types_private.h"
#include <EGL/eglext.h>
#include <EGL/egl.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2.h>
#include <stdlib.h>
#include <string.h>

/* server thread local variable */
__thread link_list_t    *active_state
  __attribute__(( tls_model ("initial-exec"))) = NULL;

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

void 
_server_init (server_t *server)
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
static void 
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

static bool
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
static void 
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
static void
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

static void
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
static void 
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

exposed_to_tests inline bool
_gl_is_valid_func (server_t *server, void *func)
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
_gl_is_valid_context (server_t *server)
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
_gl_set_error (server_t *server, GLenum error)
{
    egl_state_t *egl_state;

    if (active_state) {
        egl_state = (egl_state_t *) active_state->data;
 
        if (egl_state->active && egl_state->state.error == GL_NO_ERROR)
            egl_state->state.error = error;
    }
}

/* GLES2 core profile API */
exposed_to_tests void _gl_active_texture (server_t *server, GLenum texture)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glActiveTexture) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (egl_state->state.active_texture == texture)
            return;
        else if (texture > GL_TEXTURE31 || texture < GL_TEXTURE0) {
            _gl_set_error (server, GL_INVALID_ENUM);
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

exposed_to_tests void _gl_attach_shader (server_t *server, GLuint program, GLuint shader)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glAttachShader) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glAttachShader (server, program, shader);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_bind_attrib_location (server_t *server, GLuint program, GLuint index, const GLchar *name)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glBindAttribLocation) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glBindAttribLocation (server, program, index, name);
        egl_state->state.need_get_error = true;
    }
    if (name)
        free ((char *)name);
}

exposed_to_tests void _gl_bind_buffer (server_t *server, GLenum target, GLuint buffer)
{
    egl_state_t *egl_state;
    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int count;
    int i;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glBindBuffer) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        attrib_list = &egl_state->state.vertex_attribs;
        vertex_attrib_t *attribs = attrib_list->attribs;
        count = attrib_list->count;

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
            _gl_set_error (server, GL_INVALID_ENUM);
        }
    }
}

exposed_to_tests void _gl_bind_framebuffer (server_t *server, GLenum target, GLuint framebuffer)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glBindFramebuffer) &&
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

        CACHING_SERVER(server)->super_dispatch.glBindFramebuffer (server, target, framebuffer);
        /* FIXME: should we save it, it will be invalid if the
         * framebuffer is invalid 
         */
        egl_state->state.framebuffer_binding = framebuffer;

        /* egl_state->state.need_get_error = true; */
    }
}

exposed_to_tests void _gl_bind_renderbuffer (server_t *server, GLenum target, GLuint renderbuffer)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glBindRenderbuffer) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (target != GL_RENDERBUFFER) {
            _gl_set_error (server, GL_INVALID_ENUM);
        }

        CACHING_SERVER(server)->super_dispatch.glBindRenderbuffer (server, target, renderbuffer);
        /* egl_state->state.need_get_error = true; */
    }
}

exposed_to_tests void _gl_bind_texture (server_t *server, GLenum target, GLuint texture)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glBindTexture) &&
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

        CACHING_SERVER(server)->super_dispatch.glBindTexture (server, target, texture);
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

exposed_to_tests void _gl_blend_color (server_t *server, GLclampf red, GLclampf green,
                             GLclampf blue, GLclampf alpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glBlendColor) &&
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

        CACHING_SERVER(server)->super_dispatch.glBlendColor (server, red, green, blue, alpha);
    }
}

exposed_to_tests void _gl_blend_equation (server_t *server, GLenum mode)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glBlendEquation) &&
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

        CACHING_SERVER(server)->super_dispatch.glBlendEquation (server, mode);
    }
}

exposed_to_tests void _gl_blend_equation_separate (server_t *server, GLenum modeRGB, GLenum modeAlpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glBlendEquationSeparate) &&
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

        CACHING_SERVER(server)->super_dispatch.glBlendEquationSeparate (server, modeRGB, modeAlpha);
    }
}

exposed_to_tests void _gl_blend_func (server_t *server, GLenum sfactor, GLenum dfactor)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glBlendFunc) &&
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

        CACHING_SERVER(server)->super_dispatch.glBlendFunc (server, sfactor, dfactor);
    }
}

exposed_to_tests void _gl_blend_func_separate (server_t *server, GLenum srcRGB, GLenum dstRGB,
                                     GLenum srcAlpha, GLenum dstAlpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glBlendFuncSeparate) &&
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

        CACHING_SERVER(server)->super_dispatch.glBlendFuncSeparate (server, srcRGB, dstRGB, srcAlpha, dstAlpha);
    }
}

exposed_to_tests void _gl_buffer_data (server_t *server, GLenum target, GLsizeiptr size,
                             const GLvoid *data, GLenum usage)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glBufferData) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        /* XXX: we skip rest of check, because driver
         * can generate GL_OUT_OF_MEMORY, which cannot check
         */
        CACHING_SERVER(server)->super_dispatch.glBufferData (server, target, size, data, usage);
        egl_state = (egl_state_t *) active_state->data;
        egl_state->state.need_get_error = true;
    }

    if (data)
        free ((void *)data);
}

exposed_to_tests void _gl_buffer_sub_data (server_t *server, GLenum target, GLintptr offset,
                                 GLsizeiptr size, const GLvoid *data)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glBufferSubData) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

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

exposed_to_tests GLenum _gl_check_framebuffer_status (server_t *server, GLenum target)
{
    GLenum result = GL_INVALID_ENUM;
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glCheckFramebufferStatus) &&
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

        return CACHING_SERVER(server)->super_dispatch.glCheckFramebufferStatus (server, target);
    }

    return result;
}

exposed_to_tests void _gl_clear (server_t *server, GLbitfield mask)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glClear) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (! (mask & GL_COLOR_BUFFER_BIT ||
               mask & GL_DEPTH_BUFFER_BIT ||
               mask & GL_STENCIL_BUFFER_BIT)) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glClear (server, mask);
    }
}

exposed_to_tests void _gl_clear_color (server_t *server, GLclampf red, GLclampf green,
                             GLclampf blue, GLclampf alpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glClearColor) &&
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

        CACHING_SERVER(server)->super_dispatch.glClearColor (server, red, green, blue, alpha);
    }
}

exposed_to_tests void _gl_clear_depthf (server_t *server, GLclampf depth)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glClearDepthf) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        state = &egl_state->state;

        if (state->depth_clear_value == depth)
            return;

        state->depth_clear_value = depth;

        CACHING_SERVER(server)->super_dispatch.glClearDepthf (server, depth);
    }
}

exposed_to_tests void _gl_clear_stencil (server_t *server, GLint s)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glClearStencil) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        state = &egl_state->state;

        if (state->stencil_clear_value == s)
            return;

        state->stencil_clear_value = s;

        CACHING_SERVER(server)->super_dispatch.glClearStencil (server, s);
    }
}

exposed_to_tests void _gl_color_mask (server_t *server, GLboolean red, GLboolean green,
                            GLboolean blue, GLboolean alpha)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glColorMask) &&
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

        CACHING_SERVER(server)->super_dispatch.glColorMask (server, red, green, blue, alpha);
    }
}

exposed_to_tests void _gl_compressed_tex_image_2d (server_t *server, GLenum target, GLint level,
                                         GLenum internalformat,
                                         GLsizei width, GLsizei height,
                                         GLint border, GLsizei imageSize,
                                         const GLvoid *data)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glCompressedTexImage2D) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glCompressedTexImage2D (server, target, level, internalformat,
                                       width, height, border, imageSize,
                                       data);

        egl_state->state.need_get_error = true;
    }

    if (data)
        free ((void *)data);
}

exposed_to_tests void _gl_compressed_tex_sub_image_2d (server_t *server, GLenum target, GLint level,
                                             GLint xoffset, GLint yoffset,
                                             GLsizei width, GLsizei height,
                                             GLenum format, 
                                             GLsizei imageSize,
                                             const GLvoid *data)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glCompressedTexSubImage2D) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glCompressedTexSubImage2D (server, target, level, xoffset, yoffset,
                                          width, height, format, imageSize,
                                          data);

        egl_state->state.need_get_error = true;
    }

    if (data)
        free ((void *)data);
}

exposed_to_tests void _gl_copy_tex_image_2d (server_t *server, GLenum target, GLint level,
                                   GLenum internalformat,
                                   GLint x, GLint y,
                                   GLsizei width, GLsizei height,
                                   GLint border)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glCopyTexImage2D) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glCopyTexImage2D (server, target, level, internalformat,
                                 x, y, width, height, border);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_copy_tex_sub_image_2d (server_t *server, GLenum target, GLint level,
                                       GLint xoffset, GLint yoffset,
                                       GLint x, GLint y,
                                       GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glCopyTexSubImage2D) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glCopyTexSubImage2D (server, target, level, xoffset, yoffset,
                                    x, y, width, height);

        egl_state->state.need_get_error = true;
    }
}

/* This is a sync call */
exposed_to_tests GLuint _gl_create_program  (server_t *server)
{
    GLuint result = 0;
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glCreateProgram) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        result = CACHING_SERVER(server)->super_dispatch.glCreateProgram (server);
    }

    return result;
}

/* sync call */
exposed_to_tests GLuint _gl_create_shader (server_t *server, GLenum shaderType)
{
    GLuint result = 0;
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glCreateShader) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (! (shaderType == GL_VERTEX_SHADER ||
               shaderType == GL_FRAGMENT_SHADER)) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return result;
        }

        result = CACHING_SERVER(server)->super_dispatch.glCreateShader (server, shaderType);
    }

    return result;
}

exposed_to_tests void _gl_cull_face (server_t *server, GLenum mode)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glCullFace) &&
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

        CACHING_SERVER(server)->super_dispatch.glCullFace (server, mode);
    }
}

exposed_to_tests void _gl_delete_buffers (server_t *server, GLsizei n, const GLuint *buffers)
{
    egl_state_t *egl_state;
    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int count;
    int i, j;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glDeleteBuffers) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        attrib_list = &egl_state->state.vertex_attribs;
        vertex_attrib_t *attribs = attrib_list->attribs;
        count = attrib_list->count;

        if (n < 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
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

exposed_to_tests void _gl_delete_framebuffers (server_t *server, GLsizei n, const GLuint *framebuffers)
{
    egl_state_t *egl_state;
    int i;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glDeleteFramebuffers) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (n < 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
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

exposed_to_tests void _gl_delete_program (server_t *server, GLuint program)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glDeleteProgram) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glDeleteProgram (server, program);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_delete_renderbuffers (server_t *server, GLsizei n, const GLuint *renderbuffers)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glDeleteRenderbuffers) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (n < 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            goto FINISH;
        }

        CACHING_SERVER(server)->super_dispatch.glDeleteRenderbuffers (server, n, renderbuffers);
    }

FINISH:
    if (renderbuffers)
        free ((void *)renderbuffers);
}

exposed_to_tests void _gl_delete_shader (server_t *server, GLuint shader)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glDeleteShader) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glDeleteShader (server, shader);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_delete_textures (server_t *server, GLsizei n, const GLuint *textures)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glDeleteTextures) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (n < 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            goto FINISH;
        }

        CACHING_SERVER(server)->super_dispatch.glDeleteTextures (server, n, textures);
    }

FINISH:
    if (textures)
        free ((void *)textures);
}

exposed_to_tests void _gl_depth_func (server_t *server, GLenum func)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glDepthFunc) &&
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

        CACHING_SERVER(server)->super_dispatch.glDepthFunc (server, func);
    }
}

exposed_to_tests void _gl_depth_mask (server_t *server, GLboolean flag)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glDepthMask) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (egl_state->state.depth_writemask == flag)
            return;

        egl_state->state.depth_writemask = flag;

        CACHING_SERVER(server)->super_dispatch.glDepthMask (server, flag);
    }
}

exposed_to_tests void _gl_depth_rangef (server_t *server, GLclampf nearVal, GLclampf farVal)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glDepthRangef) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (egl_state->state.depth_range[0] == nearVal &&
            egl_state->state.depth_range[1] == farVal)
            return;

        egl_state->state.depth_range[0] = nearVal;
        egl_state->state.depth_range[1] = farVal;

        CACHING_SERVER(server)->super_dispatch.glDepthRangef (server, nearVal, farVal);
    }
}

exposed_to_tests void _gl_detach_shader (server_t *server, GLuint program, GLuint shader)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glDetachShader) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        /* XXX: command buffer, error code generated */
        CACHING_SERVER(server)->super_dispatch.glDetachShader (server, program, shader);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void
_gl_set_cap (server_t *server, GLenum cap, GLboolean enable)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    bool needs_call = false;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glDisable) &&
        _gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glEnable) &&
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
                CACHING_SERVER(server)->super_dispatch.glEnable (server, cap);
            else
                CACHING_SERVER(server)->super_dispatch.glDisable (server, cap);
        }
    }
}

exposed_to_tests void _gl_disable (server_t *server, GLenum cap)
{
    _gl_set_cap (server, cap, GL_FALSE);
}

exposed_to_tests void _gl_enable (server_t *server, GLenum cap)
{
    _gl_set_cap (server, cap, GL_TRUE);
}

exposed_to_tests bool
_gl_index_is_too_large (server_t *server, gles2_state_t *state, GLuint index)
{
    if (index >= state->max_vertex_attribs) {
        if (! state->max_vertex_attribs_queried) {
            /* XXX: command buffer */
            CACHING_SERVER(server)->super_dispatch.glGetIntegerv (server, GL_MAX_VERTEX_ATTRIBS,
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
_gl_set_vertex_attrib_array (server_t *server, GLuint index, gles2_state_t *state, 
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

exposed_to_tests void _gl_disable_vertex_attrib_array (server_t *server, GLuint index)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glDisableVertexAttribArray) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        state = &egl_state->state;

        _gl_set_vertex_attrib_array (server, index, state, GL_FALSE);
    }
}

exposed_to_tests void _gl_enable_vertex_attrib_array (server_t *server, GLuint index)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glEnableVertexAttribArray) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        state = &egl_state->state;

        _gl_set_vertex_attrib_array (server, index, state, GL_TRUE);
    }
}

/* FIXME: we should use pre-allocated buffer if possible */
exposed_to_tests char *
_gl_create_data_array (server_t *server, vertex_attrib_t *attrib, int count)
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

exposed_to_tests void _gl_draw_arrays (server_t *server, GLenum mode, GLint first, GLsizei count)
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

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glDrawArrays) &&
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
            CACHING_SERVER(server)->super_dispatch.glDrawArrays (server, mode, first, count);
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
                CACHING_SERVER(server)->super_dispatch.glVertexAttribPointer (server, attribs[i].index,
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
        CACHING_SERVER(server)->super_dispatch.glDrawArrays (server, mode, first, count);

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
_gl_create_indices_array (server_t *server, GLenum mode, GLenum type, int count,
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

exposed_to_tests void _gl_draw_elements (server_t *server, GLenum mode, GLsizei count, GLenum type,
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

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glDrawElements) &&
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
            CACHING_SERVER(server)->super_dispatch.glDrawElements (server, mode, count, type, indices);
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
                CACHING_SERVER(server)->super_dispatch.glVertexAttribPointer (server, attribs[i].index,
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

        CACHING_SERVER(server)->super_dispatch.glDrawElements (server, mode, type, count, indices);

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

exposed_to_tests void _gl_finish (server_t *server)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glFinish) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glFinish (server);
    }
}

exposed_to_tests void _gl_flush (server_t *server)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glFlush) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glFlush (server);
    }
}

exposed_to_tests void _gl_framebuffer_renderbuffer (server_t *server, GLenum target, GLenum attachment,
                                          GLenum renderbuffertarget,
                                          GLenum renderbuffer)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glFramebufferRenderbuffer) &&
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

        CACHING_SERVER(server)->super_dispatch.glFramebufferRenderbuffer (server, target, attachment,
                                          renderbuffertarget, 
                                          renderbuffer);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_framebuffer_texture_2d (server_t *server, GLenum target, GLenum attachment,
                                        GLenum textarget, GLuint texture, 
                                        GLint level)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glFramebufferTexture2D) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (target != GL_FRAMEBUFFER) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glFramebufferTexture2D (server, target, attachment, textarget,
                                       texture, level);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_front_face (server_t *server, GLenum mode)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glFramebufferTexture2D) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (egl_state->state.front_face == mode)
            return;

        if (! (mode == GL_CCW || mode == GL_CW)) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        egl_state->state.front_face = mode;
        CACHING_SERVER(server)->super_dispatch.glFrontFace (server, mode);
    }
}

exposed_to_tests void _gl_gen_buffers (server_t *server, GLsizei n, GLuint *buffers)
{
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGenBuffers) &&
        _gl_is_valid_context (server)) {

        if (n < 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            return;
        }
    
        CACHING_SERVER(server)->super_dispatch.glGenBuffers (server, n, buffers);
    }
}

exposed_to_tests void _gl_gen_framebuffers (server_t *server, GLsizei n, GLuint *framebuffers)
{
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGenFramebuffers) &&
        _gl_is_valid_context (server)) {

        if (n < 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            return;
        }
        
        CACHING_SERVER(server)->super_dispatch.glGenFramebuffers (server, n, framebuffers);
    }
}

exposed_to_tests void _gl_gen_renderbuffers (server_t *server, GLsizei n, GLuint *renderbuffers)
{
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGenRenderbuffers) &&
        _gl_is_valid_context (server)) {

        if (n < 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            return;
        }
        
        CACHING_SERVER(server)->super_dispatch.glGenRenderbuffers (server, n, renderbuffers);
    }
}

exposed_to_tests void _gl_gen_textures (server_t *server, GLsizei n, GLuint *textures)
{
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGenTextures) &&
        _gl_is_valid_context (server)) {

        if (n < 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            return;
        }
        
        CACHING_SERVER(server)->super_dispatch.glGenTextures (server, n, textures);
    }
}

exposed_to_tests void _gl_generate_mipmap (server_t *server, GLenum target)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGenerateMipmap) &&
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

        CACHING_SERVER(server)->super_dispatch.glGenerateMipmap (server, target);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_get_booleanv (server_t *server, GLenum pname, GLboolean *params)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetBooleanv) &&
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
            CACHING_SERVER(server)->super_dispatch.glGetBooleanv (server, pname, params);
            break;
        }
    }
}

exposed_to_tests void _gl_get_floatv (server_t *server, GLenum pname, GLfloat *params)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetFloatv) &&
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
            CACHING_SERVER(server)->super_dispatch.glGetFloatv (server, pname, params);
            break;
        }
    }
}

exposed_to_tests void _gl_get_integerv (server_t *server, GLenum pname, GLint *params)
{
    egl_state_t *egl_state;
    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int count;
    int i;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetIntegerv) &&
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
#ifdef GL_OES_vertex_array_object
        else if (pname == GL_VERTEX_ARRAY_BINDING_OES)
            egl_state->state.vertex_array_binding = *params;
#endif
    }
}

exposed_to_tests void _gl_get_active_attrib (server_t *server, GLuint program, GLuint index,
                                   GLsizei bufsize, GLsizei *length,
                                   GLint *size, GLenum *type, GLchar *name)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetActiveAttrib) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glGetActiveAttrib (server, program, index, bufsize, length,
                                  size, type, name);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_get_active_uniform (server_t *server, GLuint program, GLuint index, 
                                    GLsizei bufsize, GLsizei *length, 
                                    GLint *size, GLenum *type, 
                                    GLchar *name)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetActiveUniform) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glGetActiveUniform (server, program, index, bufsize, length,
                                   size, type, name);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_get_attached_shaders (server_t *server, GLuint program, GLsizei maxCount,
                                      GLsizei *count, GLuint *shaders)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetAttachedShaders) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glGetAttachedShaders (server, program, maxCount, count, shaders);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests GLint _gl_get_attrib_location (server_t *server, GLuint program, const GLchar *name)
{
    egl_state_t *egl_state;
    GLint result = -1;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetAttribLocation) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        result = CACHING_SERVER(server)->super_dispatch.glGetAttribLocation (server, program, name);

        egl_state->state.need_get_error = true;
    }
    return result;
}

exposed_to_tests void _gl_get_buffer_parameteriv (server_t *server, GLenum target, GLenum value, GLint *data)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetBufferParameteriv) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glGetBufferParameteriv (server, target, value, data);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests GLenum _gl_get_error (server_t *server)
{
    GLenum error = GL_INVALID_OPERATION;
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetError) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
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

exposed_to_tests void _gl_get_framebuffer_attachment_parameteriv (server_t *server, GLenum target,
                                                        GLenum attachment,
                                                        GLenum pname,
                                                        GLint *params)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetFramebufferAttachmentParameteriv) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (target != GL_FRAMEBUFFER) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glGetFramebufferAttachmentParameteriv (server, target, attachment,
                                                      pname, params);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_get_program_info_log (server_t *server, GLuint program, GLsizei maxLength,
                                      GLsizei *length, GLchar *infoLog)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetProgramInfoLog) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glGetProgramInfoLog (server, program, maxLength, length, infoLog);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_get_programiv (server_t *server, GLuint program, GLenum pname, GLint *params)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetProgramiv) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glGetProgramiv (server, program, pname, params);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_get_renderbuffer_parameteriv (server_t *server, GLenum target,
                                              GLenum pname,
                                              GLint *params)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetRenderbufferParameteriv) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (target != GL_RENDERBUFFER) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glGetRenderbufferParameteriv (server, target, pname, params);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_get_shader_info_log (server_t *server, GLuint program, GLsizei maxLength,
                                     GLsizei *length, GLchar *infoLog)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetShaderInfoLog) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glGetShaderInfoLog (server, program, maxLength, length, infoLog);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_get_shader_precision_format (server_t *server, GLenum shaderType, 
                                             GLenum precisionType,
                                             GLint *range, 
                                             GLint *precision)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetShaderPrecisionFormat) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glGetShaderPrecisionFormat (server, shaderType, precisionType,
                                           range, precision);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_get_shader_source  (server_t *server, GLuint shader, GLsizei bufSize, 
                                    GLsizei *length, GLchar *source)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetShaderSource) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glGetShaderSource (server, shader, bufSize, length, source);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_get_shaderiv (server_t *server, GLuint shader, GLenum pname, GLint *params)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetShaderiv) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glGetShaderiv (server, shader, pname, params);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests const GLubyte *_gl_get_string (server_t *server, GLenum name)
{
    GLubyte *result = NULL;
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetString) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (! (name == GL_VENDOR                   || 
               name == GL_RENDERER                 ||
               name == GL_SHADING_LANGUAGE_VERSION ||
               name == GL_EXTENSIONS)) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return NULL;
        }

        result = (GLubyte *)CACHING_SERVER(server)->super_dispatch.glGetString (server, name);

        egl_state->state.need_get_error = true;
    }
    return (const GLubyte *)result;
}

exposed_to_tests void _gl_get_tex_parameteriv (server_t *server, GLenum target, GLenum pname, 
                                     GLint *params)
{
    egl_state_t *egl_state;
    int active_texture_index;
    int target_index;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetTexParameteriv) &&
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

exposed_to_tests void _gl_get_tex_parameterfv (server_t *server, GLenum target, GLenum pname, GLfloat *params)
{
    GLint paramsi;

    _gl_get_tex_parameteriv (server, target, pname, &paramsi);
    *params = paramsi;
}

exposed_to_tests void _gl_get_uniformiv (server_t *server, GLuint program, GLint location, 
                               GLint *params)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetUniformiv) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glGetUniformiv (server, program, location, params);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_get_uniformfv (server_t *server, GLuint program, GLint location, 
                               GLfloat *params)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetUniformfv) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glGetUniformfv (server, program, location, params);

        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests GLint _gl_get_uniform_location (server_t *server, GLuint program, const GLchar *name)
{
    GLint result = -1;
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetUniformLocation) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        result = CACHING_SERVER(server)->super_dispatch.glGetUniformLocation (server, program, name);

        egl_state->state.need_get_error = true;
    }
    return result;
}

exposed_to_tests void _gl_get_vertex_attribfv (server_t *server, GLuint index, GLenum pname, 
                                     GLfloat *params)
{
    egl_state_t *egl_state;

    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int count;
    int i, found_index = -1;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetVertexAttribfv) &&
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
            CACHING_SERVER(server)->super_dispatch.glGetVertexAttribfv (server, index, pname, params);
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

exposed_to_tests void _gl_get_vertex_attribiv (server_t *server, GLuint index, GLenum pname, GLint *params)
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

exposed_to_tests void _gl_get_vertex_attrib_pointerv (server_t *server, GLuint index, GLenum pname, 
                                            GLvoid **pointer)
{
    egl_state_t *egl_state;

    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int count;
    int i, found_index = -1;
    
    *pointer = 0;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetVertexAttribPointerv) &&
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
            CACHING_SERVER(server)->super_dispatch.glGetVertexAttribPointerv (server, index, pname, pointer);
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

exposed_to_tests void _gl_hint (server_t *server, GLenum target, GLenum mode)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glHint) &&
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

        CACHING_SERVER(server)->super_dispatch.glHint (server, target, mode);

        if (target != GL_GENERATE_MIPMAP_HINT)
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests GLboolean _gl_is_buffer (server_t *server, GLuint buffer)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glIsBuffer) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        result = CACHING_SERVER(server)->super_dispatch.glIsBuffer (server, buffer);
    }
    return result;
}

exposed_to_tests GLboolean _gl_is_enabled (server_t *server, GLenum cap)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glIsEnabled) &&
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

exposed_to_tests GLboolean _gl_is_framebuffer (server_t *server, GLuint framebuffer)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glIsFramebuffer) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        result = CACHING_SERVER(server)->super_dispatch.glIsFramebuffer (server, framebuffer);
    }
    return result;
}

exposed_to_tests GLboolean _gl_is_program (server_t *server, GLuint program)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glIsProgram) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        result = CACHING_SERVER(server)->super_dispatch.glIsProgram (server, program);
    }
    return result;
}

exposed_to_tests GLboolean _gl_is_renderbuffer (server_t *server, GLuint renderbuffer)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glIsRenderbuffer) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        result = CACHING_SERVER(server)->super_dispatch.glIsRenderbuffer (server, renderbuffer);
    }
    return result;
}

exposed_to_tests GLboolean _gl_is_shader (server_t *server, GLuint shader)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glIsShader) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        result = CACHING_SERVER(server)->super_dispatch.glIsShader (server, shader);
    }
    return result;
}

exposed_to_tests GLboolean _gl_is_texture (server_t *server, GLuint texture)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glIsTexture) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        result = CACHING_SERVER(server)->super_dispatch.glIsTexture (server, texture);
    }
    return result;
}

exposed_to_tests void _gl_line_width (server_t *server, GLfloat width)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glLineWidth) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (egl_state->state.line_width == width)
            return;

        if (width < 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            return;
        }

        egl_state->state.line_width = width;
        CACHING_SERVER(server)->super_dispatch.glLineWidth (server, width);
    }
}

exposed_to_tests void _gl_link_program (server_t *server, GLuint program)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glLinkProgram) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glLinkProgram (server, program);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_pixel_storei (server_t *server, GLenum pname, GLint param)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glPixelStorei) &&
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

        CACHING_SERVER(server)->super_dispatch.glPixelStorei (server, pname, param);
    }
}

exposed_to_tests void _gl_polygon_offset (server_t *server, GLfloat factor, GLfloat units)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glPolygonOffset) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (egl_state->state.polygon_offset_factor == factor &&
            egl_state->state.polygon_offset_units == units)
            return;

        egl_state->state.polygon_offset_factor = factor;
        egl_state->state.polygon_offset_units = units;

        CACHING_SERVER(server)->super_dispatch.glPolygonOffset (server, factor, units);
    }
}

/* sync call */
exposed_to_tests void _gl_read_pixels (server_t *server, GLint x, GLint y,
                             GLsizei width, GLsizei height,
                             GLenum format, GLenum type, GLvoid *data)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glReadPixels) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glReadPixels (server, x, y, width, height, format, type, data);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_compile_shader (server_t *server, GLuint shader)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glCompileShader) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glCompileShader (server, shader);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_release_shader_compiler (server_t *server)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glReleaseShaderCompiler) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glReleaseShaderCompiler (server);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_renderbuffer_storage (server_t *server, GLenum target, GLenum internalformat,
                                      GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glRenderbufferStorage) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glRenderbufferStorage (server, target, internalformat, width, height);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_sample_coverage (server_t *server, GLclampf value, GLboolean invert)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glSampleCoverage) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (value == egl_state->state.sample_coverage_value &&
            invert == egl_state->state.sample_coverage_invert)
            return;

        egl_state->state.sample_coverage_invert = invert;
        egl_state->state.sample_coverage_value = value;

        CACHING_SERVER(server)->super_dispatch.glSampleCoverage (server, value, invert);
    }
}

exposed_to_tests void _gl_scissor (server_t *server, GLint x, GLint y, GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glScissor) &&
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

        CACHING_SERVER(server)->super_dispatch.glScissor (server, x, y, width, height);
    }
}

exposed_to_tests void _gl_shader_binary (server_t *server, GLsizei n, const GLuint *shaders,
                               GLenum binaryformat, const void *binary,
                               GLsizei length)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glShaderBinary) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glShaderBinary (server, n, shaders, binaryformat, binary, length);
        egl_state->state.need_get_error = true;
    }
    if (binary)
        free ((void *)binary);
}

exposed_to_tests void _gl_shader_source (server_t *server, GLuint shader, GLsizei count,
                               const GLchar **string, const GLint *length)
{
    egl_state_t *egl_state;
    int i;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glShaderSource) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

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

exposed_to_tests void _gl_stencil_func_separate (server_t *server, GLenum face, GLenum func,
                                       GLint ref, GLuint mask)
{
    egl_state_t *egl_state;
    bool needs_call = false;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glStencilFuncSeparate) &&
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
        CACHING_SERVER(server)->super_dispatch.glStencilFuncSeparate (server, face, func, ref, mask);
}

exposed_to_tests void _gl_stencil_func (server_t *server, GLenum func, GLint ref, GLuint mask)
{
    _gl_stencil_func_separate (server, GL_FRONT_AND_BACK, func, ref, mask);
}

exposed_to_tests void _gl_stencil_mask_separate (server_t *server, GLenum face, GLuint mask)
{
    egl_state_t *egl_state;
    bool needs_call = false;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glStencilMaskSeparate) &&
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
        CACHING_SERVER(server)->super_dispatch.glStencilMaskSeparate (server, face, mask);
}

exposed_to_tests void _gl_stencil_mask (server_t *server, GLuint mask)
{
    _gl_stencil_mask_separate (server, GL_FRONT_AND_BACK, mask);
}

exposed_to_tests void _gl_stencil_op_separate (server_t *server, GLenum face, GLenum sfail, 
                                     GLenum dpfail, GLenum dppass)
{
    egl_state_t *egl_state;
    bool needs_call = false;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glStencilOpSeparate) &&
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
        CACHING_SERVER(server)->super_dispatch.glStencilOpSeparate (server, face, sfail, dpfail, dppass);
}

exposed_to_tests void _gl_stencil_op (server_t *server, GLenum sfail, GLenum dpfail, GLenum dppass)
{
    _gl_stencil_op_separate (server, GL_FRONT_AND_BACK, sfail, dpfail, dppass);
}

exposed_to_tests void _gl_tex_image_2d (server_t *server, GLenum target, GLint level, 
                              GLint internalformat,
                              GLsizei width, GLsizei height, GLint border,
                              GLenum format, GLenum type, 
                              const GLvoid *data)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glTexImage2D) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glTexImage2D (server, target, level, internalformat, width, height,
                             border, format, type, data);
        egl_state->state.need_get_error = true;
    }
    if (data)
      free ((void *)data);
}

exposed_to_tests void _gl_tex_parameteri (server_t *server, GLenum target, GLenum pname, GLint param)
{
    egl_state_t *egl_state;
    gles2_state_t *state;
    int active_texture_index;
    int target_index;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glTexParameteri) &&
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

        CACHING_SERVER(server)->super_dispatch.glTexParameteri (server, target, pname, param);
    }
}

exposed_to_tests void _gl_tex_parameterf (server_t *server, GLenum target, GLenum pname, GLfloat param)
{
    GLint parami = param;
    _gl_tex_parameteri (server, target, pname, parami);
}

exposed_to_tests void _gl_tex_sub_image_2d (server_t *server, GLenum target, GLint level,
                                  GLint xoffset, GLint yoffset,
                                  GLsizei width, GLsizei height,
                                  GLenum format, GLenum type, 
                                  const GLvoid *data)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glTexSubImage2D) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glTexSubImage2D (server, target, level, xoffset, yoffset,
                                width, height, format, type, data);
        egl_state->state.need_get_error = true;
    }
    if (data)
        free ((void *)data);
}

exposed_to_tests void _gl_uniform1f (server_t *server, GLint location, GLfloat v0)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glUniform1f) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glUniform1f (server, location, v0);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_uniform2f (server_t *server, GLint location, GLfloat v0, GLfloat v1)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glUniform2f) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glUniform2f (server, location, v0, v1);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_uniform3f (server_t *server, GLint location, GLfloat v0, GLfloat v1, 
                           GLfloat v2)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glUniform3f) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glUniform3f (server, location, v0, v1, v2);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_uniform4f (server_t *server, GLint location, GLfloat v0, GLfloat v1, 
                           GLfloat v2, GLfloat v3)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glUniform4f) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glUniform4f (server, location, v0, v1, v2, v3);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_uniform1i (server_t *server, GLint location, GLint v0)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glUniform1i) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glUniform1i (server, location, v0);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_uniform_2i (server_t *server, GLint location, GLint v0, GLint v1)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glUniform2i) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glUniform2i (server, location, v0, v1);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_uniform3i (server_t *server, GLint location, GLint v0, GLint v1, GLint v2)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glUniform3i) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glUniform3i (server, location, v0, v1, v2);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_uniform4i (server_t *server, GLint location, GLint v0, GLint v1, 
                           GLint v2, GLint v3)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glUniform4i) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glUniform4i (server, location, v0, v1, v2, v3);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void
_gl_uniform_fv (server_t *server, int i, GLint location,
                GLsizei count, const GLfloat *value)
{
    egl_state_t *egl_state;

    if (! _gl_is_valid_context (server))
        goto FINISH;

    switch (i) {
    case 1:
        if(! _gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glUniform1fv))
            goto FINISH;
        CACHING_SERVER(server)->super_dispatch.glUniform1fv (server, location, count, value);
        break;
    case 2:
        if(! _gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glUniform2fv))
            goto FINISH;
        CACHING_SERVER(server)->super_dispatch.glUniform2fv (server, location, count, value);
        break;
    case 3:
        if(! _gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glUniform3fv))
            goto FINISH;
        CACHING_SERVER(server)->super_dispatch.glUniform3fv (server, location, count, value);
        break;
    default:
        if(! _gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glUniform4fv))
            goto FINISH;
        CACHING_SERVER(server)->super_dispatch.glUniform4fv (server, location, count, value);
        break;
    }

    egl_state = (egl_state_t *) active_state->data;
    egl_state->state.need_get_error = true;

FINISH:
    if (value)
        free ((GLfloat *)value);
}

exposed_to_tests void _gl_uniform1fv (server_t *server, GLint location, GLsizei count, 
                            const GLfloat *value)
{
    _gl_uniform_fv (server, 1, location, count, value);
}

exposed_to_tests void _gl_uniform2fv (server_t *server, GLint location, GLsizei count, 
                            const GLfloat *value)
{
    _gl_uniform_fv (server, 2, location, count, value);
}

exposed_to_tests void _gl_uniform3fv (server_t *server, GLint location, GLsizei count, 
                            const GLfloat *value)
{
    _gl_uniform_fv (server, 3, location, count, value);
}

exposed_to_tests void _gl_uniform4fv (server_t *server, GLint location, GLsizei count, 
                            const GLfloat *value)
{
    _gl_uniform_fv (server, 4, location, count, value);
}

exposed_to_tests void
_gl_uniform_iv (server_t *server,
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
        if(! _gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glUniform1iv))
            goto FINISH;
        CACHING_SERVER(server)->super_dispatch.glUniform1iv (server, location, count, value);
        break;
    case 2:
        if(! _gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glUniform2iv))
            goto FINISH;
        CACHING_SERVER(server)->super_dispatch.glUniform2iv (server, location, count, value);
        break;
    case 3:
        if(! _gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glUniform3iv))
            goto FINISH;
        CACHING_SERVER(server)->super_dispatch.glUniform3iv (server, location, count, value);
        break;
    default:
        if(! _gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glUniform4iv))
            goto FINISH;
        CACHING_SERVER(server)->super_dispatch.glUniform4iv (server, location, count, value);
        break;
    }

    egl_state = (egl_state_t *) active_state->data;
    egl_state->state.need_get_error = true;

FINISH:
    if (value)
        free ((GLint *)value);
}

exposed_to_tests void _gl_uniform1iv (server_t *server, GLint location, GLsizei count, 
                            const GLint *value)
{
    _gl_uniform_iv (server, 1, location, count, value);
}

exposed_to_tests void _gl_uniform2iv (server_t *server, GLint location, GLsizei count, 
                            const GLint *value)
{
    _gl_uniform_iv (server, 2, location, count, value);
}

exposed_to_tests void _gl_uniform3iv (server_t *server, GLint location, GLsizei count, 
                            const GLint *value)
{
    _gl_uniform_iv (server, 3, location, count, value);
}

exposed_to_tests void _gl_uniform4iv (server_t *server, GLint location, GLsizei count, 
                            const GLint *value)
{
    _gl_uniform_iv (server, 4, location, count, value);
}

exposed_to_tests void _gl_use_program (server_t *server, GLuint program)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glUseProgram) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
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

exposed_to_tests void _gl_validate_program (server_t *server, GLuint program)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glValidateProgram) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glValidateProgram (server, program);
        egl_state->state.need_get_error = true;
    }
}

exposed_to_tests void _gl_vertex_attrib1f (server_t *server, GLuint index, GLfloat v0)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glVertexAttrib1f) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (_gl_index_is_too_large (server, &egl_state->state, index)) {
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glVertexAttrib1f (server, index, v0);
    }
}

exposed_to_tests void _gl_vertex_attrib2f (server_t *server, GLuint index, GLfloat v0, GLfloat v1)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glVertexAttrib2f) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (_gl_index_is_too_large (server, &egl_state->state, index)) {
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glVertexAttrib2f (server, index, v0, v1);
    }
}

exposed_to_tests void _gl_vertex_attrib3f (server_t *server, GLuint index, GLfloat v0, 
                                 GLfloat v1, GLfloat v2)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glVertexAttrib3f) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (_gl_index_is_too_large (server, &egl_state->state, index)) {
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glVertexAttrib3f (server, index, v0, v1, v2);
    }
}

exposed_to_tests void _gl_vertex_attrib4f (server_t *server, GLuint index, GLfloat v0, GLfloat v1, 
                                 GLfloat v2, GLfloat v3)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glVertexAttrib3f) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (_gl_index_is_too_large (server, &egl_state->state, index))
            return;

        CACHING_SERVER(server)->super_dispatch.glVertexAttrib4f (server, index, v0, v1, v2, v3);
    }
}

exposed_to_tests void
_gl_vertex_attrib_fv (server_t *server, int i, GLuint index, const GLfloat *v)
{
    egl_state_t *egl_state;
    
    if (! _gl_is_valid_context (server))
        goto FINISH;
    
    egl_state = (egl_state_t *) active_state->data;

    if (_gl_index_is_too_large (server, &egl_state->state, index))
        goto FINISH;

    switch (i) {
        case 1:
            if(! _gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glVertexAttrib1fv))
                goto FINISH;
            CACHING_SERVER(server)->super_dispatch.glVertexAttrib1fv (server, index, v);
            break;
        case 2:
            if(! _gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glVertexAttrib2fv))
                goto FINISH;
            CACHING_SERVER(server)->super_dispatch.glVertexAttrib2fv (server, index, v);
            break;
        case 3:
            if(! _gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glVertexAttrib3fv))
                goto FINISH;
            CACHING_SERVER(server)->super_dispatch.glVertexAttrib3fv (server, index, v);
            break;
        default:
            if(! _gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glVertexAttrib4fv))
                goto FINISH;
            CACHING_SERVER(server)->super_dispatch.glVertexAttrib4fv (server, index, v);
            break;
    }

FINISH:
    if (v)
        free ((GLfloat *)v);
}

exposed_to_tests void _gl_vertex_attrib1fv (server_t *server, GLuint index, const GLfloat *v)
{
    _gl_vertex_attrib_fv (server, 1, index, v);
}

exposed_to_tests void _gl_vertex_attrib2fv (server_t *server, GLuint index, const GLfloat *v)
{
    _gl_vertex_attrib_fv (server, 2, index, v);
}

exposed_to_tests void _gl_vertex_attrib3fv (server_t *server, GLuint index, const GLfloat *v)
{
    _gl_vertex_attrib_fv (server, 3, index, v);
}

exposed_to_tests void _gl_vertex_attrib4fv (server_t *server, GLuint index, const GLfloat *v)
{
    _gl_vertex_attrib_fv (server, 4, index, v);
}

exposed_to_tests void _gl_vertex_attrib_pointer (server_t *server, GLuint index, GLint size, 
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

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glVertexAttribPointer) &&
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
            CACHING_SERVER(server)->super_dispatch.glVertexAttribPointer (server, index, size, type, normalized,
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

exposed_to_tests void _gl_viewport (server_t *server, GLint x, GLint y, GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glViewport) &&
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

        CACHING_SERVER(server)->super_dispatch.glViewport (server, x, y, width, height);
    }
}
/* end of GLES2 core profile */

#ifdef GL_OES_EGL_image
static void
_gl_egl_image_target_texture_2d_oes (server_t *server, GLenum target, GLeglImageOES image)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glEGLImageTargetTexture2DOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (target != GL_TEXTURE_2D) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glEGLImageTargetTexture2DOES (server, target, image);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_egl_image_target_renderbuffer_storage_oes (server_t *server, GLenum target, 
                                               GLeglImageOES image)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glEGLImageTargetRenderbufferStorageOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
#ifndef HAS_GLES2
        if (target != GL_RENDERBUFFER_OES) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }
#endif

        CACHING_SERVER(server)->super_dispatch.glEGLImageTargetRenderbufferStorageOES (server, target, image);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_OES_get_program_binary
static void
_gl_get_program_binary_oes (server_t *server, GLuint program, GLsizei bufSize, 
                            GLsizei *length, GLenum *binaryFormat, 
                            GLvoid *binary)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetProgramBinaryOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glGetProgramBinaryOES (server, program, bufSize, length, 
                                      binaryFormat, binary);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_program_binary_oes (server_t *server, GLuint program, GLenum binaryFormat,
                        const GLvoid *binary, GLint length)
{
    egl_state_t *egl_state;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glProgramBinaryOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glProgramBinaryOES (server, program, binaryFormat, binary, length);
        egl_state->state.need_get_error = true;
    }

    if (binary)
        free ((void *)binary);
}
#endif

#ifdef GL_OES_mapbuffer
static void* 
_gl_map_buffer_oes (server_t *server, GLenum target, GLenum access)
{
    egl_state_t *egl_state;
    void *result = NULL;
    
    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glMapBufferOES) &&
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

        result = CACHING_SERVER(server)->super_dispatch.glMapBufferOES (server, target, access);
        egl_state->state.need_get_error = true;
    }
    return result;
}

static GLboolean
_gl_unmap_buffer_oes (server_t *server, GLenum target)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glUnmapBufferOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (! (target == GL_ARRAY_BUFFER ||
               target == GL_ELEMENT_ARRAY_BUFFER)) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return result;
        }

        result = CACHING_SERVER(server)->super_dispatch.glUnmapBufferOES (server, target);
        egl_state->state.need_get_error = true;
    }
    return result;
}

static void
_gl_get_buffer_pointerv_oes (server_t *server, GLenum target, GLenum pname, GLvoid **params)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetBufferPointervOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (! (target == GL_ARRAY_BUFFER ||
               target == GL_ELEMENT_ARRAY_BUFFER)) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glGetBufferPointervOES (server, target, pname, params);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_OES_texture_3D
static void
_gl_tex_image_3d_oes (server_t *server, GLenum target, GLint level, GLenum internalformat,
                      GLsizei width, GLsizei height, GLsizei depth,
                      GLint border, GLenum format, GLenum type,
                      const GLvoid *pixels)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glTexImage3DOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glTexImage3DOES (server, target, level, internalformat, 
                                width, height, depth, 
                                border, format, type, pixels);
        egl_state->state.need_get_error = true;
    }
    if (pixels)
        free ((void *)pixels);
}

static void
_gl_tex_sub_image_3d_oes (server_t *server, GLenum target, GLint level,
                          GLint xoffset, GLint yoffset, GLint zoffset,
                          GLsizei width, GLsizei height, GLsizei depth,
                          GLenum format, GLenum type, const GLvoid *data)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glTexSubImage3DOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

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
_gl_copy_tex_sub_image_3d_oes (server_t *server, GLenum target, GLint level,
                               GLint xoffset, GLint yoffset, GLint zoffset,
                               GLint x, GLint y,
                               GLint width, GLint height)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glCopyTexSubImage3DOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glCopyTexSubImage3DOES (server, target, level,
                                       xoffset, yoffset, zoffset,
                                       x, y, width, height);

        egl_state->state.need_get_error = true;
    }
}

static void
_gl_compressed_tex_image_3d_oes (server_t *server, GLenum target, GLint level,
                                 GLenum internalformat,
                                 GLsizei width, GLsizei height, 
                                 GLsizei depth,
                                 GLint border, GLsizei imageSize,
                                 const GLvoid *data)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glCompressedTexImage3DOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        CACHING_SERVER(server)->super_dispatch.glCompressedTexImage3DOES (server, target, level, internalformat,
                                          width, height, depth,
                                          border, imageSize, data);

        egl_state->state.need_get_error = true;
    }
    if (data)
        free ((void *)data);
}

static void
_gl_compressed_tex_sub_image_3d_oes (server_t *server, GLenum target, GLint level,
                                     GLint xoffset, GLint yoffset, 
                                     GLint zoffset,
                                     GLsizei width, GLsizei height, 
                                     GLsizei depth,
                                     GLenum format, GLsizei imageSize,
                                     const GLvoid *data)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glCompressedTexSubImage3DOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

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
_gl_framebuffer_texture_3d_oes (server_t *server, GLenum target, GLenum attachment,
                                GLenum textarget, GLuint texture,
                                GLint level, GLint zoffset)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glFramebufferTexture3DOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (target != GL_FRAMEBUFFER) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glFramebufferTexture3DOES (server, target, attachment, textarget,
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
_gl_bind_vertex_array_oes (server_t *server, GLuint array)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glBindVertexArrayOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;

        if (egl_state->state.vertex_array_binding == array)
            return;

        CACHING_SERVER(server)->super_dispatch.glBindVertexArrayOES (server, array);
        egl_state->state.need_get_error = true;
        /* FIXME: should we save this ? */
        egl_state->state.vertex_array_binding = array;
    }
}

static void
_gl_delete_vertex_arrays_oes (server_t *server, GLsizei n, const GLuint *arrays)
{
    egl_state_t *egl_state;
    int i;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glDeleteVertexArraysOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        if (n <= 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
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
_gl_gen_vertex_arrays_oes (server_t *server, GLsizei n, GLuint *arrays)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGenVertexArraysOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        if (n <= 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glGenVertexArraysOES (server, n, arrays);
    }
}

static GLboolean
_gl_is_vertex_array_oes (server_t *server, GLuint array)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glIsVertexArrayOES) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        result = CACHING_SERVER(server)->super_dispatch.glIsVertexArrayOES (server, array);

        if (result == GL_FALSE && 
            egl_state->state.vertex_array_binding == array)
            egl_state->state.vertex_array_binding = 0;
    }
    return result;
}
#endif

#ifdef GL_AMD_performance_monitor
static void
_gl_get_perf_monitor_groups_amd (server_t *server, GLint *numGroups, GLsizei groupSize, 
                                 GLuint *groups)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetPerfMonitorGroupsAMD) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glGetPerfMonitorGroupsAMD (server, numGroups, groupSize, groups);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_get_perf_monitor_counters_amd (server_t *server, GLuint group, GLint *numCounters, 
                                   GLint *maxActiveCounters, 
                                   GLsizei counterSize,
                                   GLuint *counters)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetPerfMonitorCountersAMD) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glGetPerfMonitorCountersAMD (server, group, numCounters,
                                            maxActiveCounters,
                                            counterSize, counters);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_get_perf_monitor_group_string_amd (server_t *server, GLuint group, GLsizei bufSize, 
                                       GLsizei *length, 
                                       GLchar *groupString)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetPerfMonitorGroupStringAMD) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glGetPerfMonitorGroupStringAMD (server, group, bufSize, length,
                                               groupString);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_get_perf_monitor_counter_string_amd (server_t *server, GLuint group, GLuint counter, 
                                         GLsizei bufSize, 
                                         GLsizei *length, 
                                         GLchar *counterString)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetPerfMonitorCounterStringAMD) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glGetPerfMonitorCounterStringAMD (server, group, counter, bufSize, 
                                                 length, counterString);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_get_perf_monitor_counter_info_amd (server_t *server, GLuint group, GLuint counter, 
                                       GLenum pname, GLvoid *data)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetPerfMonitorCounterInfoAMD) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glGetPerfMonitorCounterInfoAMD (server, group, counter, 
                                               pname, data); 
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_gen_perf_monitors_amd (server_t *server, GLsizei n, GLuint *monitors) 
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGenPerfMonitorsAMD) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glGenPerfMonitorsAMD (server, n, monitors); 
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_delete_perf_monitors_amd (server_t *server, GLsizei n, GLuint *monitors) 
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glDeletePerfMonitorsAMD) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glDeletePerfMonitorsAMD (server, n, monitors); 
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_select_perf_monitor_counters_amd (server_t *server, GLuint monitor, GLboolean enable,
                                      GLuint group, GLint numCounters,
                                      GLuint *countersList) 
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glSelectPerfMonitorCountersAMD) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glSelectPerfMonitorCountersAMD (server, monitor, enable, group,
                                               numCounters, countersList); 
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_begin_perf_monitor_amd (server_t *server, GLuint monitor)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glBeginPerfMonitorAMD) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glBeginPerfMonitorAMD (server, monitor); 
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_end_perf_monitor_amd (server_t *server, GLuint monitor)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glEndPerfMonitorAMD) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glEndPerfMonitorAMD (server, monitor); 
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_get_perf_monitor_counter_data_amd (server_t *server, GLuint monitor, GLenum pname,
                                       GLsizei dataSize, GLuint *data,
                                       GLint *bytesWritten)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetPerfMonitorCounterDataAMD) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glGetPerfMonitorCounterDataAMD (server, monitor, pname, dataSize,
                                               data, bytesWritten); 
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_ANGLE_framebuffer_blit
static void
_gl_blit_framebuffer_angle (server_t *server, GLint srcX0, GLint srcY0, 
                            GLint srcX1, GLint srcY1,
                            GLint dstX0, GLint dstY0, 
                            GLint dstX1, GLint dstY1,
                            GLbitfield mask, GLenum filter)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glBlitFramebufferANGLE) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glBlitFramebufferANGLE (server, srcX0, srcY0, srcX1, srcY1,
                                       dstX0, dstY0, dstX1, dstY1,
                                       mask, filter);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_ANGLE_framebuffer_multisample
static void
_gl_renderbuffer_storage_multisample_angle (server_t *server, GLenum target, GLsizei samples,
                                            GLenum internalformat,
                                            GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glRenderbufferStorageMultisampleANGLE) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glRenderbufferStorageMultisampleANGLE (server, target, samples,
                                                      internalformat,
                                                      width, height);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_APPLE_framebuffer_multisample
static void
_gl_renderbuffer_storage_multisample_apple (server_t *server, GLenum target, GLsizei samples,
                                            GLenum internalformat, 
                                            GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glRenderbufferStorageMultisampleAPPLE) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glRenderbufferStorageMultisampleAPPLE (server, target, samples,
                                                      internalformat,
                                                      width, height);
        egl_state->state.need_get_error = true;
    }
}

static void
gl_resolve_multisample_framebuffer_apple (void)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glResolveMultisampleFramebufferAPPLE) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glResolveMultisampleFramebufferAPPLE (server);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_EXT_discard_framebuffer
static void
gl_discard_framebuffer_ext (server_t *server,
                            GLenum target, GLsizei numAttachments,
                            const GLenum *attachments)
{
    egl_state_t *egl_state;
    int i;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glDiscardFramebufferEXT) &&
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
        CACHING_SERVER(server)->super_dispatch.glDiscardFramebufferEXT (server, target, numAttachments, 
                                        attachments);
    }
FINISH:
    if (attachments)
        free ((void *)attachments);
}
#endif

#ifdef GL_EXT_multi_draw_arrays
static void
_gl_multi_draw_arrays_ext (server_t *server, GLenum mode, const GLint *first, 
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
_gl_renderbuffer_storage_multisample_ext (server_t *server, GLenum target, GLsizei samples,
                                          GLenum internalformat,
                                          GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glRenderbufferStorageMultisampleEXT) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glRenderbufferStorageMultisampleEXT (server, target, samples, 
                                                    internalformat, 
                                                    width, height);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_framebuffer_texture_2d_multisample_ext (server_t *server, GLenum target, 
                                            GLenum attachment,
                                            GLenum textarget, 
                                            GLuint texture,
                                            GLint level, GLsizei samples)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glFramebufferTexture2DMultisampleEXT) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        if (target != GL_FRAMEBUFFER) {
           _gl_set_error (server, GL_INVALID_ENUM);
           return;
        }

        CACHING_SERVER(server)->super_dispatch.glFramebufferTexture2DMultisampleEXT (server, target, attachment, 
                                                     textarget, texture, 
                                                     level, samples);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_IMG_multisampled_render_to_texture
static void
_gl_renderbuffer_storage_multisample_img (server_t *server, GLenum target, GLsizei samples,
                                          GLenum internalformat,
                                          GLsizei width, GLsizei height)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glRenderbufferStorageMultisampleIMG) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        CACHING_SERVER(server)->super_dispatch.glRenderbufferStorageMultisampleIMG (server, target, samples, 
                                                    internalformat, 
                                                    width, height);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_framebuffer_texture_2d_multisample_img (server_t *server, GLenum target, 
                                            GLenum attachment,
                                            GLenum textarget, 
                                            GLuint texture,
                                            GLint level, GLsizei samples)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glFramebufferTexture2DMultisampleIMG) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
        
        if (target != GL_FRAMEBUFFER) {
           _gl_set_error (server, GL_INVALID_ENUM);
           return;
        }

        CACHING_SERVER(server)->super_dispatch.glFramebufferTexture2DMultisampleIMG (server, target, attachment,
                                                     textarget, texture,
                                                     level, samples);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_NV_fence
static void
_gl_delete_fences_nv (server_t *server, GLsizei n, const GLuint *fences)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glDeleteFencesNV) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (n <= 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
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
_gl_gen_fences_nv (server_t *server, GLsizei n, GLuint *fences)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGenFencesNV) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (n <= 0) {
            _gl_set_error (server, GL_INVALID_VALUE);
            return;
        }
    
        CACHING_SERVER(server)->super_dispatch.glGenFencesNV (server, n, fences); 
        egl_state->state.need_get_error = true;
    }
}

static GLboolean
_gl_is_fence_nv (server_t *server, GLuint fence)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glIsFenceNV) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        result = CACHING_SERVER(server)->super_dispatch.glIsFenceNV (server, fence);
        egl_state->state.need_get_error = true;
    }
    return result;
}

static GLboolean
_gl_test_fence_nv (server_t *server, GLuint fence)
{
    egl_state_t *egl_state;
    GLboolean result = GL_FALSE;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glTestFenceNV) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        result = CACHING_SERVER(server)->super_dispatch.glTestFenceNV (server, fence);
        egl_state->state.need_get_error = true;
    }
    return result;
}

static void 
_gl_get_fenceiv_nv (server_t *server, GLuint fence, GLenum pname, int *params)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetFenceivNV) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glGetFenceivNV (server, fence, pname, params);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_finish_fence_nv (server_t *server, GLuint fence)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glFinishFenceNV) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glFinishFenceNV (server, fence);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_set_fence_nv (server_t *server, GLuint fence, GLenum condition)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glSetFenceNV) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glSetFenceNV (server, fence, condition);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_NV_coverage_sample
static void
_gl_coverage_mask_nv (server_t *server, GLboolean mask)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glCoverageMaskNV) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glCoverageMaskNV (server, mask);
    }
}

static void
_gl_coverage_operation_nv (server_t *server, GLenum operation)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glCoverageOperationNV) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        if (! (operation == GL_COVERAGE_ALL_FRAGMENTS_NV  ||
               operation == GL_COVERAGE_EDGE_FRAGMENTS_NV ||
               operation == GL_COVERAGE_AUTOMATIC_NV)) {
            _gl_set_error (server, GL_INVALID_ENUM);
            return;
        }

        CACHING_SERVER(server)->super_dispatch.glCoverageOperationNV (server, operation);
    }
}
#endif

#ifdef GL_QCOM_driver_control
static void
_gl_get_driver_controls_qcom (server_t *server, GLint *num, GLsizei size, 
                              GLuint *driverControls)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetDriverControlsQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glGetDriverControlsQCOM (server, num, size, driverControls);
    }
}

static void
_gl_get_driver_control_string_qcom (server_t *server, GLuint driverControl, GLsizei bufSize,
                                    GLsizei *length, 
                                    GLchar *driverControlString)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glGetDriverControlStringQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glGetDriverControlStringQCOM (server, driverControl, bufSize,
                                             length, driverControlString);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_enable_driver_control_qcom (server_t *server, GLuint driverControl)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glEnableDriverControlQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glEnableDriverControlQCOM (server, driverControl);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_disable_driver_control_qcom (server_t *server, GLuint driverControl)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glDisableDriverControlQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glDisableDriverControlQCOM (server, driverControl);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_QCOM_extended_get
static void
_gl_ext_get_textures_qcom (server_t *server, GLuint *textures, GLint maxTextures, 
                           GLint *numTextures)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glExtGetTexturesQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glExtGetTexturesQCOM (server, textures, maxTextures, numTextures);
    }
}

static void
_gl_ext_get_buffers_qcom (server_t *server, GLuint *buffers, GLint maxBuffers, 
                          GLint *numBuffers) 
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glExtGetBuffersQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glExtGetBuffersQCOM (server, buffers, maxBuffers, numBuffers);
    }
}

static void
_gl_ext_get_renderbuffers_qcom (server_t *server, GLuint *renderbuffers, 
                                GLint maxRenderbuffers,
                                GLint *numRenderbuffers)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glExtGetRenderbuffersQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glExtGetRenderbuffersQCOM (server, renderbuffers, maxRenderbuffers,
                                          numRenderbuffers);
    }
}

static void
_gl_ext_get_framebuffers_qcom (server_t *server, GLuint *framebuffers, GLint maxFramebuffers,
                               GLint *numFramebuffers)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glExtGetFramebuffersQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glExtGetFramebuffersQCOM (server, framebuffers, maxFramebuffers,
                                         numFramebuffers);
    }
}

static void
_gl_ext_get_tex_level_parameteriv_qcom (server_t *server, GLuint texture, GLenum face, 
                                        GLint level, GLenum pname, 
                                        GLint *params)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glExtGetTexLevelParameterivQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glExtGetTexLevelParameterivQCOM (server, texture, face, level,
                                                pname, params);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_ext_tex_object_state_overridei_qcom (server_t *server, GLenum target, GLenum pname, 
                                         GLint param)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glExtTexObjectStateOverrideiQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glExtTexObjectStateOverrideiQCOM (server, target, pname, param);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_ext_get_tex_sub_image_qcom (server_t *server, GLenum target, GLint level,
                                GLint xoffset, GLint yoffset, 
                                GLint zoffset,
                                GLsizei width, GLsizei height, 
                                GLsizei depth,
                                GLenum format, GLenum type, GLvoid *texels)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glExtGetTexSubImageQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glExtGetTexSubImageQCOM (server, target, level,
                                        xoffset, yoffset, zoffset,
                                        width, height, depth,
                                        format, type, texels);
    egl_state->state.need_get_error = true;
    }
}

static void
_gl_ext_get_buffer_pointerv_qcom (server_t *server, GLenum target, GLvoid **params)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glExtGetBufferPointervQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glExtGetBufferPointervQCOM (server, target, params);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_QCOM_extended_get2
static void
_gl_ext_get_shaders_qcom (server_t *server, GLuint *shaders, GLint maxShaders, 
                          GLint *numShaders)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glExtGetShadersQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glExtGetShadersQCOM (server, shaders, maxShaders, numShaders);
    }
}

static void
_gl_ext_get_programs_qcom (server_t *server, GLuint *programs, GLint maxPrograms,
                           GLint *numPrograms)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glExtGetProgramsQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glExtGetProgramsQCOM (server, programs, maxPrograms, numPrograms);
    }
}

static GLboolean
_gl_ext_is_program_binary_qcom (server_t *server, GLuint program)
{
    egl_state_t *egl_state;
    bool result = false;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glExtIsProgramBinaryQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        result = CACHING_SERVER(server)->super_dispatch.glExtIsProgramBinaryQCOM (server, program);
        egl_state->state.need_get_error = true;
    }
    return result;
}

static void
_gl_ext_get_program_binary_source_qcom (server_t *server, GLuint program, GLenum shadertype,
                                        GLchar *source, GLint *length)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glExtGetProgramBinarySourceQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glExtGetProgramBinarySourceQCOM (server, program, shadertype,
                                                source, length);
        egl_state->state.need_get_error = true;
    }
}
#endif

#ifdef GL_QCOM_tiled_rendering
static void
_gl_start_tiling_qcom (server_t *server, GLuint x, GLuint y, GLuint width, GLuint height,
                       GLbitfield preserveMask)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glStartTilingQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glStartTilingQCOM (server, x, y, width, height, preserveMask);
        egl_state->state.need_get_error = true;
    }
}

static void
_gl_end_tiling_qcom (server_t *server, GLbitfield preserveMask)
{
    egl_state_t *egl_state;

    if (_gl_is_valid_func (server, CACHING_SERVER(server)->super_dispatch.glEndTilingQCOM) &&
        _gl_is_valid_context (server)) {
        egl_state = (egl_state_t *) active_state->data;
    
        CACHING_SERVER(server)->super_dispatch.glEndTilingQCOM (server, preserveMask);
        egl_state->state.need_get_error = true;
    }
}
#endif

exposed_to_tests EGLint
_egl_get_error (server_t *server)
{
    EGLint error = EGL_NOT_INITIALIZED;

    if (! CACHING_SERVER(server)->super_dispatch.eglGetError)
        return error;

    error = CACHING_SERVER(server)->super_dispatch.eglGetError(server);

    return error;
}

exposed_to_tests EGLDisplay
_egl_get_display (server_t *server,
                  EGLNativeDisplayType display_id)
{
    EGLDisplay display = EGL_NO_DISPLAY;

    display = CACHING_SERVER(server)->super_dispatch.eglGetDisplay (server, display_id);

    return display;
}

exposed_to_tests EGLBoolean
_egl_initialize (server_t *server,
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

exposed_to_tests EGLBoolean
_egl_terminate (server_t *server,
                EGLDisplay display)
{
    EGLBoolean result = EGL_FALSE;

    if (CACHING_SERVER(server)->super_dispatch.eglTerminate) {
        result = CACHING_SERVER(server)->super_dispatch.eglTerminate (server, display);

        if (result == EGL_TRUE) {
            /* XXX: remove srv structure */
            _server_terminate (display, active_state);
        }
    }

    return result;
}

static const char *
_egl_query_string (server_t *server,
                   EGLDisplay display,
                   EGLint name)
{
    const char *result = NULL;

    if (CACHING_SERVER(server)->super_dispatch.eglQueryString)
        result = CACHING_SERVER(server)->super_dispatch.eglQueryString (server, display, name);

    return result;
}

exposed_to_tests EGLBoolean
_egl_get_configs (server_t *server,
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

exposed_to_tests EGLBoolean
_egl_choose_config (server_t *server,
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

exposed_to_tests EGLBoolean
_egl_get_config_attrib (server_t *server,
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

exposed_to_tests EGLSurface
_egl_create_window_surface (server_t *server,
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

exposed_to_tests EGLSurface
_egl_create_pbuffer_surface (server_t *server,
                             EGLDisplay display,
                             EGLConfig config,
                             const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    if (CACHING_SERVER(server)->super_dispatch.eglCreatePbufferSurface)
        surface = CACHING_SERVER(server)->super_dispatch.eglCreatePbufferSurface (server, display, config, attrib_list);

    return surface;
}

exposed_to_tests EGLSurface
_egl_create_pixmap_surface (server_t *server,
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

exposed_to_tests EGLBoolean
_egl_destroy_surface (server_t *server,
                      EGLDisplay display,
                      EGLSurface surface)
{
    EGLBoolean result = EGL_FALSE;

    if (!active_state)
        return result;

    if (CACHING_SERVER(server)->super_dispatch.eglDestroySurface) { 
        result = CACHING_SERVER(server)->super_dispatch.eglDestroySurface (server, display, surface);

        if (result == EGL_TRUE) {
            /* update srv states */
            _server_destroy_surface (display, surface, active_state);
        }
    }

    return result;
}

exposed_to_tests EGLBoolean
_egl_query_surface (server_t *server,
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

exposed_to_tests EGLBoolean
_egl_bind_api (server_t *server,
               EGLenum api)
{
    EGLBoolean result = EGL_FALSE;

    if (CACHING_SERVER(server)->super_dispatch.eglBindAPI) 
        result = CACHING_SERVER(server)->super_dispatch.eglBindAPI (server, api);

    return result;
}

static EGLenum 
_egl_query_api (server_t *server)
{
    EGLenum result = EGL_NONE;

    if (CACHING_SERVER(server)->super_dispatch.eglQueryAPI) 
        result = CACHING_SERVER(server)->super_dispatch.eglQueryAPI (server);

    return result;
}

static EGLBoolean
_egl_wait_client (server_t *server)
{
    EGLBoolean result = EGL_FALSE;

    if (CACHING_SERVER(server)->super_dispatch.eglWaitClient)
        result = CACHING_SERVER(server)->super_dispatch.eglWaitClient (server);

    return result;
}

exposed_to_tests EGLBoolean
_egl_release_thread (server_t *server)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t *egl_state;
    link_list_t *active_state_out = NULL;

    if (CACHING_SERVER(server)->super_dispatch.eglReleaseThread) {
        result = CACHING_SERVER(server)->super_dispatch.eglReleaseThread (server);

        if (result == EGL_TRUE) {
            if (! active_state)
                return result;
            
            egl_state = (egl_state_t *) active_state->data;

            _server_make_current (egl_state->display,
                                             EGL_NO_SURFACE,
                                             EGL_NO_SURFACE,
                                             EGL_NO_CONTEXT,
                                             active_state,
                                             &active_state_out);
	    active_state = active_state_out;

        }
    }

    return result;
}

static EGLSurface
_egl_create_pbuffer_from_client_buffer (server_t *server,
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
_egl_surface_attrib (server_t *server,
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
_egl_bind_tex_image (server_t *server,
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
_egl_release_tex_image (server_t *server,
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
_egl_swap_interval (server_t *server,
                    EGLDisplay display,
                    EGLint interval)
{
    EGLBoolean result = EGL_FALSE;
    
    if (CACHING_SERVER(server)->super_dispatch.eglSwapInterval)
        result = CACHING_SERVER(server)->super_dispatch.eglSwapInterval (server, display, interval);

    return result;
}

exposed_to_tests EGLContext
_egl_create_context (server_t *server,
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

exposed_to_tests EGLBoolean
_egl_destroy_context (server_t *server,
                      EGLDisplay dpy,
                      EGLContext ctx)
{
    EGLBoolean result = GL_FALSE;

    if (CACHING_SERVER(server)->super_dispatch.eglDestroyContext) {
        result = CACHING_SERVER(server)->super_dispatch.eglDestroyContext (server, dpy, ctx); 

        if (result == GL_TRUE) {
            _server_destroy_context (dpy, ctx, active_state);
        }
    }

    return result;
}

exposed_to_tests EGLContext
_egl_get_current_context (server_t *server)
{
    return CACHING_SERVER(server)->super_dispatch.eglGetCurrentContext (server);
    egl_state_t *state;

    if (!active_state)
        return EGL_NO_CONTEXT;

    state = (egl_state_t *) active_state->data;
    return state->context;
}

exposed_to_tests EGLDisplay
_egl_get_current_display (server_t *server)
{
    return CACHING_SERVER(server)->super_dispatch.eglGetCurrentDisplay (server);
    egl_state_t *state;

    if (!active_state)
        return EGL_NO_DISPLAY;

    state = (egl_state_t *) active_state->data;
    return state->display;
}

exposed_to_tests EGLSurface
_egl_get_current_surface (server_t *server,
                          EGLint readdraw)
{
    return CACHING_SERVER(server)->super_dispatch.eglGetCurrentSurface (server, readdraw);
    egl_state_t *state;
    EGLSurface surface = EGL_NO_SURFACE;

    if (!active_state)
        return EGL_NO_SURFACE;

    state = (egl_state_t *) active_state->data;

    if (state->display == EGL_NO_DISPLAY || state->context == EGL_NO_CONTEXT)
        goto FINISH;

    if (readdraw == EGL_DRAW)
        surface = state->drawable;
    else
        surface = state->readable;
 FINISH:
    return surface;
}


exposed_to_tests EGLBoolean
_egl_query_context (server_t *server,
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
_egl_wait_gl (server_t *server)
{
    EGLBoolean result = EGL_FALSE;
    
    if (! CACHING_SERVER(server)->super_dispatch.eglWaitGL)
        return result;

    result = CACHING_SERVER(server)->super_dispatch.eglWaitGL (server);

    return result;
}

static EGLBoolean
_egl_wait_native (server_t *server,
                  EGLint engine)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglWaitNative)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglWaitNative (server, engine);

    return result;
}

static EGLBoolean
_egl_swap_buffers (server_t *server,
                   EGLDisplay display,
                   EGLSurface surface)
{
    EGLBoolean result = EGL_BAD_DISPLAY;

    result = CACHING_SERVER(server)->super_dispatch.eglSwapBuffers (server, display, surface);

    return result;
}

static EGLBoolean
_egl_copy_buffers (server_t *server,
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
_egl_get_proc_address (server_t *server,
                       const char *procname)
{
    return CACHING_SERVER(server)->super_dispatch.eglGetProcAddress (server, procname);
}

exposed_to_tests EGLBoolean 
_egl_make_current (server_t *server,
                   EGLDisplay display,
                   EGLSurface draw,
                   EGLSurface read,
                   EGLContext ctx) 
{
    EGLBoolean result = EGL_FALSE;
    link_list_t *exist = NULL;
    link_list_t *active_state_out = NULL;
    bool found = false;

    if (! CACHING_SERVER(server)->super_dispatch.eglMakeCurrent)
        return result;

    /* look for existing */
    found = _match (display, draw, read, ctx, &exist);
    if (found == true) {
        /* set active to exist, tell client about it */
        active_state = exist;

        /* call real makeCurrent */
        return CACHING_SERVER(server)->super_dispatch.eglMakeCurrent (server, display, draw, read, ctx);
    }

    /* We could not find in the saved state, we don't know whether
     * parameters are valid or not 
     */
    result = CACHING_SERVER(server)->super_dispatch.eglMakeCurrent (server, display, draw, read, ctx);
    if (result == EGL_TRUE) {
        _server_make_current (display, draw, read, ctx,
                                         active_state, 
                                         &active_state_out);
        active_state = active_state_out;
    }
    return result;
}

/* start of eglext.h */
#ifdef EGL_KHR_lock_surface
static EGLBoolean
_egl_lock_surface_khr (server_t *server,
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
_egl_unlock_surface_khr (server_t *server,
                         EGLDisplay display,
                         EGLSurface surface)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglUnlockSurfaceKHR)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglUnlockSurfaceKHR (server, display, surface);

    return result;
}
#endif

#ifdef EGL_KHR_image
static EGLImageKHR
_egl_create_image_khr (server_t *server,
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
_egl_destroy_image_khr (server_t *server,
                        EGLDisplay display,
                        EGLImageKHR image)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglDestroyImageKHR)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglDestroyImageKHR (server, display, image);

    return result;
}
#endif

#ifdef EGL_KHR_reusable_sync
static EGLSyncKHR
_egl_create_sync_khr (server_t *server,
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
_egl_destroy_sync_khr (server_t *server,
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
_egl_client_wait_sync_khr (server_t *server,
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
_egl_signal_sync_khr (server_t *server,
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
_egl_get_sync_attrib_khr (server_t *server,
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
#endif

#ifdef EGL_NV_sync
static EGLSyncNV 
_egl_create_fence_sync_nv (server_t *server,
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
_egl_destroy_sync_nv (server_t *server,
                      EGLSyncNV sync)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglDestroySyncNV)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglDestroySyncNV (server, sync);

    return result;
}

static EGLBoolean
_egl_fence_nv (server_t *server,
               EGLSyncNV sync)
{
    EGLBoolean result = EGL_FALSE;
    
    if (! CACHING_SERVER(server)->super_dispatch.eglFenceNV)
        return result;

    result = CACHING_SERVER(server)->super_dispatch.eglFenceNV (server, sync);

    return result;
}

static EGLint
_egl_client_wait_sync_nv (server_t *server,
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
_egl_signal_sync_nv (server_t *server,
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
_egl_get_sync_attrib_nv (server_t *server,
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
#endif

#ifdef EGL_HI_clientpixmap
static EGLSurface
_egl_create_pixmap_surface_hi (server_t *server,
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
#endif

#ifdef EGL_MESA_drm_image
static EGLImageKHR
_egl_create_drm_image_mesa (server_t *server,
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
_egl_export_drm_image_mesa (server_t *server,
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
#endif

#ifdef EGL_NV_post_sub_buffer
static EGLBoolean 
_egl_post_subbuffer_nv (server_t *server,
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
#endif

#ifdef EGL_SEC_image_map
static void *
_egl_map_image_sec (server_t *server,
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
_egl_unmap_image_sec (server_t *server,
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
_egl_get_image_attrib_sec (server_t *server,
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
#endif

exposed_to_tests void
caching_server_init (caching_server_t *server, buffer_t *buffer)
{
    server_init (&server->super, buffer, false);
    server->super_dispatch = server->super.dispatch;

    /* TODO: Override the methods in the super dispatch table. */
}

exposed_to_tests caching_server_t *
caching_server_new (buffer_t *buffer)
{
    caching_server_t *server = malloc (sizeof(caching_server_t));
    caching_server_init (server, buffer);
    return server;
}
