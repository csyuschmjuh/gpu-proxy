#ifndef GPUPROCESS_GLES2_SRV_PRIVATE_H
#define GPUPROCESS_GLES2_SRV_PRIVATE_H

#include "gpuprocess_compiler_private.h"
#include "gpuprocess_egl_states.h"
#include "gpuprocess_types_private.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_CONTEXTS 4

typedef struct gl_srv_states
{
    int num_contexts;

    /* when glxMakeCurrent() or eglMakeCurrent(), if current context
     * matches the request, we return quickly.  Otherwise, we save
     * the request context/display/drawable to pending_xxxx, 
     */
    v_bool_t 		make_current_called;
    EGLContext		pending_context;
    EGLDisplay		pending_display;
    EGLSurface		pending_drawable;
    EGLSurface		pending_readable;

    egl_state_t		*active_state;
    
    v_link_list_t	*states;
} gl_srv_states_t;

/* global state variable */
extern gl_srv_states_t	srv_states;

/* called within eglGetDisplay () */
gpuprocess_private void
_gpuprocess_srv_init ();

gpuprocess_private void
_gpuprocess_srv_terminate (EGLDisplay display);

/* called within eglMakeCurrent () */
gpuprocess_private void
_gpuprocess_srv_make_current (EGLDisplay display, 
			      EGLSurface drawable, 
			      EGLSurface readable,
			      EGLContext context);

gpuprocess_private void
_gpuprocess_srv_destroy_context (EGLDisplay display, EGLContext context);

gpuprocess_private v_bool_t
_gpuprocess_srv_is_equal (egl_state_t *state,
			  EGLDisplay  display,
			  EGLSurface  drawable,
			  EGLSurface  readable,
			  EGLContext  context);

gpuprocess_private void
_gpuprocess_srv_destroy_context (EGLDisplay display, EGLContext context);

#ifdef __cplusplus
}
#endif


#endif /* GPUPROCESS_SRV_PRIVATE_H */
