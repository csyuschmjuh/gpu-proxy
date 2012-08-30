#ifndef GPUPROCESS_GL_SRV_PRIVATE_H
#define GPUPROCESS_GL_SRV_PRIVATE_H

#include "gpuprocess_compiler_private.h"
#include "gpuprocess_glx_states.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_CONTEXTS

typedef struct gl_server_states
{
    int num_contexts;

    /* when glxMakeCurrent() or eglMakeCurrent(), if current context
     * matches the request, we return quickly.  Otherwise, we save
     * the request context/display/drawable to pending_xxxx,
     */
    GLXContext          pending_context;
    Display            *pending_display;
    GLXDrawable         pending_drawable;
    GLXDrawable         pending_readable;

    glx_state_t        *active_state;

    glx_state_t         embedded_states[NUM_CONTEXTS];
    glx_state_t        *states;
} gl_server_states_t;

/* global state variable */
gl_server_states_t        srv_states;

/* called within eglInitialize () */
gpuprocess_private void 
_gpuprocess_server_init ();

gpuprocess_private void 
_gpuprocess_server_destroy ();

/* called within eglMakeCurrent () */
gpuprocess_private void
_gpuprocess_server_make_current (EGLDisplay display,
                                 EGLSurface drawable,
                                 EGLSurface readable,
                                 EGLContext context);

#ifdef __cplusplus
}
#endif


#endif /* GPUPROCESS_SRV_PRIVATE_H */
