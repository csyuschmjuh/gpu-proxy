#ifndef GPUPROCESS_SRV_PRIVATE_H
#define GPUPROCESS_SRV_PRIVATE_H

#ifdef HAS_GL
#include "glx_states.h"
#elif HAS_GLES2
#include "egl_states.h"
#else
#error "Could not find appropriate backend"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gl_states
{
    int num_contexts;

    /* when glxMakeCurrent() or eglMakeCurrent(), if current context
     * matches the request, we return quickly.  Otherwise, we save
     * the request context/display/drawable to pending_xxxx, 
     */
#ifdef HAS_GL
    GLXContext		pending_context;	
    Display 		*pending_display;
    GLXDrawable 	pending_drawable;
    GLXDrawable 	pending_readable;

    glx_state_t		*active_state;

    glx_state_t		embedded_states[4];
    glx_state_t		*states;

#else /* EGL states */
    EGLContext		pending_context;
    EGLDisplay		pending_display;
    EGLSurface		pending_drawable;
    EGLSurface		pending_readable;

    egl_state_t		*active_state;
    
    egl_state_t		embedded_states[4];
    egl_state_t		*states;
#endif
} gl_states_t;

#ifdef __cplusplus
}
#endif

void _virtual_server_init ();

#endif /* GPUPROCESS_SRV_PRIVATE_H */
