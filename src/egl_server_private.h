#ifndef GPUPROCESS_EGL_SRV_PRIVATE_H
#define GPUPROCESS_EGL_SRV_PRIVATE_H

#include "compiler_private.h"
#include "egl_states.h"
#include "types_private.h"
#include <EGL/egl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_CONTEXTS 4

typedef struct gl_server_states
{
    bool             initialized;
    int              num_contexts;
    link_list_t      *states;
} gl_server_states_t;

/* global state variable */
extern gl_server_states_t        srv_states;

/* called within eglGetDisplay () */
private void
_server_init ();

private void
_server_terminate (EGLDisplay display, link_list_t *active_state);

/* called within eglMakeCurrent () */
private void
_server_make_current (EGLDisplay display, 
                                 EGLSurface drawable, 
                                 EGLSurface readable,
                                 EGLContext context,
                                 link_list_t *active_state,
                                 link_list_t **active_state_out);

private void
_server_destroy_context (EGLDisplay display, EGLContext context,
                                    link_list_t *active_state);

private void
_server_destroy_surface (EGLDisplay display, EGLSurface surface,
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

private EGLSurface
_egl_create_window_surface (EGLDisplay display,
                            EGLConfig config,
                            EGLNativeWindowType window,
                            const EGLint *attrib_list);

private EGLSurface
_egl_create_pbuffer_surface (EGLDisplay display,
                             EGLConfig config,
                             const EGLint *attrib_list);;

private EGLSurface
_egl_create_pbuffer_surface (EGLDisplay display,
                             EGLConfig config,
                             const EGLint *attrib_list);

private EGLSurface
_egl_create_pixmap_surface (EGLDisplay display,
                            EGLConfig config,
                            EGLNativePixmapType pixmap,
                            const EGLint *attrib_list);

private EGLContext
_egl_get_current_context_for_testing (void);

private EGLDisplay
_egl_get_current_display_for_testing (void);

private EGLSurface
_egl_get_current_surface_for_testing (EGLint readdraw);

EGLBoolean
_egl_destroy_context (EGLDisplay dpy,
                      EGLContext ctx);

private EGLBoolean
_egl_initialize (EGLDisplay display,
                 EGLint *major,
                 EGLint *minor);

private EGLBoolean
_egl_terminate (EGLDisplay display);

private EGLContext
_egl_create_context (EGLDisplay display,
                     EGLConfig config,
                     EGLContext share_context,
                     const EGLint *attrib_list);

private EGLDisplay
_egl_get_display (EGLNativeDisplayType display_id);

EGLint
_egl_get_error (void);

/*private void
_server_remove_context (EGLDisplay display,
                                EGLSurface draw,
                                EGLSurface read,
                                EGLContext context);

*/
#ifdef __cplusplus
}
#endif


#endif /* GPUPROCESS_EGL_SRV_PRIVATE_H */
