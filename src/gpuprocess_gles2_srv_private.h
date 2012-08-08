#ifndef GPUPROCESS_GLES2_SRV_PRIVATE_H
#define GPUPROCESS_GLES2_SRV_PRIVATE_H

#include "egl_states.h"

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
    EGLContext		pending_context;
    EGLDisplay		pending_display;
    EGLSurface		pending_drawable;
    EGLSurface		pending_readable;

    egl_state_t		*active_state;
    
    egl_state_t		embedded_states[NUM_CONTEXTS];
    egl_state_t		*states;
} gl_srv_states_t;

/* global state variable */
gl_srv_states_t	srv_states;

/* called within eglInitialize () */
void 
_gpuprocess_srv_init ();

void 
_gpuprocess_srv_destroy ();

/* called within eglMakeCurrent () */
void
_gpuprocess_srv_make_current (EGLDisplay display, 
			      EGLSurface drawable, 
			      EGLSurface readable,
			      EGLContext context);

void
_gpuprocess_srv_destroy_context (EGLDisplay display, EGLContext context);

int
_gpuprocess_srv_is_equal (egl_state_t *state,
			  EGLDisplay  display,
			  EGLSurface  drawable,
			  EGLSurface  readable,
			  EGLContext  context);

void 
_gpuprocess_srv_destroy_context (EGLDisplay display, EGLContext context);

#ifdef __cplusplus
}
#endif


#endif /* GPUPROCESS_SRV_PRIVATE_H */
