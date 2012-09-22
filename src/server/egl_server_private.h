#ifndef GPUPROCESS_EGL_SERVER_PRIVATE_H
#define GPUPROCESS_EGL_SERVER_PRIVATE_H

#include "command_buffer_server.h"
#include "compiler_private.h"
#include "egl_states.h"
#include "types_private.h"
#include <EGL/egl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_CONTEXTS 4

#define CACHING_SERVER(object) ((caching_server_t *) (object))
typedef struct _caching_server {
    command_buffer_server_t super;

    /* We maintain a copy of the superclass' dispatch table here, so
     * that we can chain up to the superclass. The process of subclassing
     * overrides the original dispatch table. */
    server_dispatch_table_t super_dispatch;

} caching_server_t;

typedef struct gl_server_states
{
    bool             initialized;
    int              num_contexts;
    link_list_t      *states;
} gl_server_states_t;

/* global state variable */
extern gl_server_states_t        server_states;

/* called within eglGetDisplay () */
private void
_server_init ();

private void
_server_terminate (EGLDisplay display,
                   link_list_t *active_state);

private void
_server_initialize (EGLDisplay display);

/* called within eglMakeCurrent () */
private void
_server_make_current (EGLDisplay display, 
                      EGLSurface drawable, 
                      EGLSurface readable,
                      EGLContext context,
                      link_list_t *active_state,
                      link_list_t **active_state_out);

private void
_server_destroy_context (EGLDisplay display,
                         EGLContext context,
                         link_list_t *active_state);

private void
_server_destroy_surface (EGLDisplay display,
                         EGLSurface surface,
                         link_list_t *active_state);

private bool
_server_is_equal (egl_state_t *state,
                  EGLDisplay  display,
                  EGLSurface  drawable,
                  EGLSurface  readable,
                  EGLContext  context);

private bool
_match (EGLDisplay display,
        EGLSurface drawable,
        EGLSurface readable,
        EGLContext context,
        link_list_t **state);

private void 
_server_remove_state (link_list_t **state);

#ifdef __cplusplus
}
#endif


#endif /* GPUPROCESS_EGL_SERVER_PRIVATE_H */
