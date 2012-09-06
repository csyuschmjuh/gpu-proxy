#ifndef GPUPROCESS_EGL_STATE_H
#define GPUPROCESS_EGL_STATE_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "gles2_states.h"
#include "thread_private.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct egl_state {
    EGLContext           context;        /* active context, initial EGL_NO_CONTEXT */
    EGLDisplay           display;        /* active display, initial EGL_NO_SURFACE */
    gles2_state_t        state;                /* the cached states associated
                                         * with this context
                                         */
    EGLSurface           drawable;        /* active draw drawable, initial EGL_NO_SURFACE */
    EGLSurface           readable;        /* active read drawable, initial EGL_NO_SURFACE */

    bool             active;
    bool             destroy_dpy;
    bool             destroy_ctx;
    bool             destroy_read;
    bool             destroy_draw;
} egl_state_t;

#ifdef __cplusplus
}
#endif

#endif /* GPUPROCESS_EGL_STATE_H */
