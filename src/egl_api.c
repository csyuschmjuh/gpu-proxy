/* Implemented egl and eglext.h functions.
 * This is the egl functions entry point in the client thread.
 * 
 * The server thread is initialized and be ready during eglGetDisplay()
 * and is terminated during eglTerminate() when the current context is 
 * None. If a client calls egl functions before eglGetDisplay(), an error 
 * is returned.
 * We also keep three thread local variables:
 * (1) active_context - a pointer to the current state that is in 
 * global server_states.  If it is NULL, the current client is not in
 * any valid egl context.
 * (2) command_buffer - a pointer to the command buffer that is shared
 * between client thread and server thread.  The client posts command to
 * command buffer, while the server thread reads from the command from
 * the command buffer
 * (3) unpack_alignment - this the client cache of server side state. It
 * is used by gles2_server.c to compute padding for image during
 * image uploading (glTexImage2D(), glTexSubImage2D(), glTexImage3DOES(),
 * and glTexSubImage3DOES() 
 */
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdlib.h>

#include "config.h"

#include "command_buffer.h"
#include "thread_private.h"
#include "types_private.h"
#include "egl_server_private.h"

/* thread local variable for client thread */
__thread v_link_list_t *active_context
    __attribute__(( tls_model ("initial-exec"))) = NULL;

/* client side cache unpack_alignment */
__thread int unpack_alignment
    __attribute__(( tls_model ("initial-exec"))) = 4;

EGLAPI EGLint EGLAPIENTRY
eglGetError (void)
{
    EGLint error = EGL_NOT_INITIALIZED;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return error;

    /* XXX: post eglGetError() and wait */

    return error;
}

/* This is the first call to egl system, we initialize dispatch here,
 * we also initialize gl client states
 */
EGLAPI EGLDisplay EGLAPIENTRY
eglGetDisplay (EGLNativeDisplayType display_id)
{
    EGLDisplay display = EGL_NO_DISPLAY;

    command_buffer_get_thread_local ();

    /* XXX: post eglGetDisplay and wait */
    return display;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglInitialize (EGLDisplay dpy, EGLint *major, EGLint *minor)
{
    EGLBoolean result = EGL_FALSE;
    
    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglInitialize and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY eglTerminate (EGLDisplay dpy)
{
    EGLBoolean result = EGL_FALSE;
   
    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglTerminate and wait */

    /* FIXME: only if we are in None Context, we will destroy the server 
     * according to egl spec.  What happens if we are in valid context
     * and application exit?  Obviously, there are still valid context
     * on the driver side, what about our server thread ? 
     */
    if (! active_context)
        command_buffer_destroy_thread_local ();

    return result;
}

EGLAPI const char * EGLAPIENTRY
eglQueryString (EGLDisplay dpy, EGLint name)
{
    const char *result = NULL;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglQueryString and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetConfigs (EGLDisplay dpy, EGLConfig *configs,
               EGLint config_size, EGLint *num_config)
{
    EGLBoolean result = EGL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglGetConfigs and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglChooseConfig (EGLDisplay dpy, const EGLint *attrib_list,
                 EGLConfig *configs, EGLint config_size,
                 EGLint *num_config)
{
    EGLBoolean result = EGL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglChooseConfig and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetConfigAttrib (EGLDisplay dpy, EGLConfig config, EGLint attribute,
                    EGLint *value)
{
    EGLBoolean result = EGL_FALSE;
    
    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* post eglGetConfigAttrib and wait */
    return result;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreateWindowSurface (EGLDisplay dpy, EGLConfig config,
                        EGLNativeWindowType win,
                        const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return surface;

    /* XXX: post eglCreateWindowSurface and wait */
    return surface;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreatePbufferSurface (EGLDisplay dpy, EGLConfig config,
                         const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return surface;

    /* XXX: post eglCreatePbufferSurface and wait */
    return surface;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreatePixmapSurface (EGLDisplay dpy, EGLConfig config,
                        EGLNativePixmapType pixmap,
                        const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return surface;

    /* XXX: post eglCreatePixmapSurface and wait */
    return surface;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglDestroySurface (EGLDisplay dpy, EGLSurface surface)
{
    EGLBoolean result = EGL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglDestroySurface and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglQuerySurface (EGLDisplay dpy, EGLSurface surface,
                 EGLint attribute, EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglQuerySurface and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglBindAPI (EGLenum api)
{
    EGLBoolean result = EGL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglBindAPI and wait */
    return result;
}

EGLAPI EGLenum EGLAPIENTRY
eglQueryAPI (void)
{
    EGLenum result = EGL_NONE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglQueryAPI and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglWaitClient (void)
{
    egl_state_t *egl_state;
    EGLBoolean result = EGL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    if (! active_context)
        return EGL_TRUE;

    egl_state = (egl_state_t *) active_context->data;

    if (egl_state->display == EGL_NO_DISPLAY ||
        egl_state->context == EGL_NO_CONTEXT ||
        egl_state->active == false)
        return EGL_TRUE;

    /* XXX: post eglWaitClient and wait */
    return result;
}

static void
command_buffer_set_active_context (command_buffer_t* command_buffer, v_link_list_t* new_context)
{
    active_context = new_context;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglReleaseThread (void)
{
    EGLBoolean result = EGL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* current context to None */
    /* XXX: post eglReleaseThread and wait, then set active state to NULL */
    active_context = NULL;
    command_buffer_set_active_context (command_buffer, NULL);
    return result;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreatePbufferFromClientBuffer (EGLDisplay dpy, EGLenum buftype,
                                  EGLClientBuffer buffer,
                                  EGLConfig config, const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return surface;

    /* XXX: post eglCreatePbufferFromClientBuffer and wait */
    return surface;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglSurfaceAttrib (EGLDisplay dpy, EGLSurface surface,
                  EGLint attribute, EGLint value)
{
    EGLBoolean result = EGL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglSurfaceAttrib and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglBindTexImage (EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    EGLBoolean result = EGL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;


    /* XXX: post eglBindTexImage and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglReleaseTexImage (EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    EGLBoolean result = EGL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglReleaseTexImage and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglSwapInterval (EGLDisplay dpy, EGLint interval)
{
    EGLBoolean result = EGL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglSwapInterval and wait */
    return result;
}

EGLAPI EGLContext EGLAPIENTRY
eglCreateContext (EGLDisplay dpy, EGLConfig config,
                  EGLContext share_context,
                  const EGLint *attrib_list)
{
    EGLContext result = EGL_NO_CONTEXT;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglCreateContext and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglDestroyContext (EGLDisplay dpy, EGLContext ctx)
{
    EGLBoolean result = GL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglDestroyContext and wait */
    return result;
}

EGLAPI EGLContext EGLAPIENTRY
eglGetCurrentContext (void)
{
    egl_state_t *state;

    if (! active_context)
        return EGL_NO_CONTEXT;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return EGL_NO_CONTEXT;

    state = (egl_state_t *) active_context->data;
    return state->context;
}

EGLAPI EGLDisplay EGLAPIENTRY
eglGetCurrentDisplay (void)
{
    egl_state_t *state;

    if (! active_context)
        return EGL_NO_DISPLAY;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return EGL_NO_DISPLAY;

    state = (egl_state_t *) active_context->data;
    return state->display;
}

EGLAPI EGLSurface EGLAPIENTRY
eglGetCurrentSurface (EGLint readdraw)
{
    egl_state_t *state;
    EGLSurface surface = EGL_NO_SURFACE;

    if (! active_context)
        return EGL_NO_SURFACE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return EGL_NO_SURFACE;

    state = (egl_state_t *) active_context->data;

    if (state->display == EGL_NO_DISPLAY || state->context == EGL_NO_CONTEXT)
        goto FINISH;

    if (readdraw == EGL_DRAW)
        surface = state->drawable;
    else
        surface = state->readable;
 FINISH:
    return surface;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglQueryContext (EGLDisplay dpy, EGLContext ctx,
                 EGLint attribute, EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglQueryContext and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglWaitGL (void)
{
    EGLBoolean result = EGL_TRUE;
    egl_state_t *egl_state;

    if (! active_context)
        return result;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    egl_state = (egl_state_t *) active_context->data;

    if (egl_state->display == EGL_NO_DISPLAY ||
        egl_state->context == EGL_NO_CONTEXT ||
        egl_state->active == false)
        return EGL_TRUE;

    /* XXX: post eglWaitGL and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglWaitNative (EGLint engine)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t *state;

    if (! active_context)
        return result;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglWaitNative and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglSwapBuffers (EGLDisplay dpy, EGLSurface surface)
{
    EGLBoolean result = EGL_BAD_DISPLAY;
    egl_state_t *state;

    if (! active_context)
        return result;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    state = (egl_state_t *)active_context->data;
    if (state->display == EGL_NO_DISPLAY)
        return result;

    if (state->display != dpy)
        goto FINISH;

    if (state->readable != surface || state->drawable != surface) {
        result = EGL_BAD_SURFACE;
        goto FINISH;
    }

    /* XXX: post eglSwapBuffers, no wait */

FINISH:
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglCopyBuffers (EGLDisplay dpy, EGLSurface surface,
                EGLNativePixmapType target)
{
    EGLBoolean result = EGL_FALSE;

    if (! active_context)
        return result;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglCopyBuffers and wait */
    return result;
}

EGLAPI __eglMustCastToProperFunctionPointerType EGLAPIENTRY
eglGetProcAddress (const char *procname)
{
    void *address = NULL;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return address;

    /* XXX: post eglGetProcAddress and wait */
    return address;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglMakeCurrent (EGLDisplay dpy, EGLSurface draw, EGLSurface read,
                EGLContext ctx)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t        *egl_state;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: we need to pass active_context in command buffer */
    /* we are not in any valid context */
    if (! active_context) {
        if (dpy == EGL_NO_DISPLAY || ctx == EGL_NO_CONTEXT) {
            result = EGL_TRUE;
            goto FINISH;
        }
        else {
          /* XXX: post eglMakeCurrent and wait */
          /* update active_context */
          goto FINISH;
        }
    }
    else {
        if (dpy == EGL_NO_DISPLAY || ctx == EGL_NO_CONTEXT) {
            /* XXX: post eglMakeCurrent and no wait */
            active_context = NULL;
            result = EGL_TRUE;
            goto FINISH;
        }
        else {
            egl_state = (egl_state_t *) active_context->data;
            if (egl_state->display == dpy &&
                egl_state->context == ctx &&
                egl_state->drawable == draw &&
                egl_state->readable == read) {
                result = GL_TRUE;
               goto FINISH;
            }
            else {
                /* XXX: post eglMakeCurrent and wait */
                /* update active_context */
                goto FINISH;
            }
        }
    }
FINISH:

    if (active_context) {
        egl_state = (egl_state_t *) active_context->data;
        if (egl_state->active) {
            unpack_alignment = egl_state->state.unpack_alignment;
        }
    }

    command_buffer_set_active_context (command_buffer, active_context);
    
}

/* start of eglext.h */
#ifdef EGL_KHR_lock_surface
EGLAPI EGLBoolean EGLAPIENTRY
eglLockSurfaceKHR (EGLDisplay display, EGLSurface surface,
                   const EGLint *attrib_list)
{
    EGLBoolean result = EGL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglLockSurfaceKHR and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglUnlockSurfaceKHR (EGLDisplay display, EGLSurface surface)
{
    EGLBoolean result = EGL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* post eglUnlockSurfaceKHR and wait */
    return result;
}
#endif

#ifdef EGL_KHR_image
EGLAPI EGLImageKHR EGLAPIENTRY
eglCreateImageKHR (EGLDisplay dpy, EGLContext ctx, EGLenum target,
                   EGLClientBuffer buffer, const EGLint *attrib_list)
{
    EGLImageKHR result = EGL_NO_IMAGE_KHR;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglCreateImageKHR and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglDestroyImageKHR (EGLDisplay dpy, EGLImageKHR image)
{
    EGLBoolean result = EGL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglDestroyImageKHR and wait */
    return result;
}
#endif

#ifdef EGL_KHR_reusable_sync
EGLAPI EGLSyncKHR EGLAPIENTRY
eglCreateSyncKHR (EGLDisplay dpy, EGLenum type, const EGLint *attrib_list)
{
    EGLSyncKHR result = EGL_NO_SYNC_KHR;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglCreateSyncKHR and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglDestroySyncKHR (EGLDisplay dpy, EGLSyncKHR sync)
{
    EGLBoolean result = EGL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglDestroySyncKHR and wait */
    return result;
}

EGLAPI EGLint EGLAPIENTRY
eglClientWaitSyncKHR (EGLDisplay dpy, EGLSyncKHR sync, EGLint flags,
                      EGLTimeKHR timeout)
{
    EGLint result = EGL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglClientWaitSyncKHR and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglSignalSyncKHR (EGLDisplay dpy, EGLSyncKHR sync, EGLenum mode)
{
    EGLBoolean result = EGL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglSignalSyncKHR and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetSyncAttribKHR (EGLDisplay dpy, EGLSyncKHR sync,
                     EGLint attribute, EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglGetSyncAttribKHR and wait */
    return result;
}
#endif

#ifdef EGL_NV_sync
EGLSyncNV
eglCreateFenceSyncNV (EGLDisplay dpy, EGLenum condition,
                      const EGLint *attrib_list)
{
    EGLSyncNV result = EGL_NO_SYNC_NV;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglCreateFenceSyncNV and wait */
    return result;
}

EGLBoolean
eglDestroySyncNV (EGLSyncNV sync)
{
    EGLBoolean result = EGL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglDestroyFenceSyncNV and wait */
    return result;
}

EGLBoolean
eglFenceNV (EGLSyncNV sync)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t *egl_state;

    if (! active_context)
        return result;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    egl_state = (egl_state_t *) active_context->data;

    if (egl_state->display == EGL_NO_DISPLAY ||
        egl_state->context == EGL_NO_CONTEXT ||
        egl_state->active == false)
        return result;

    /* XXX: post eglFenceNV and wait */
    return result;
}

EGLint
eglClientWaitSyncNV (EGLSyncNV sync, EGLint flags, EGLTimeNV timeout)
{
    /* XXX: is this supposed to be default value ? */
    EGLint result = EGL_TIMEOUT_EXPIRED_NV;
    egl_state_t *egl_state;

    if (! active_context)
        return result;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    egl_state = (egl_state_t *) active_context->data;

    if (egl_state->display == EGL_NO_DISPLAY ||
        egl_state->context == EGL_NO_CONTEXT ||
        egl_state->active == false)
        return result;

    /* XXX: post eglClientWaitSyncNV and wait */
    return result;
}

EGLBoolean
eglSignalSyncNV (EGLSyncNV sync, EGLenum mode)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t *egl_state;

    if (! active_context)
        return result;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    egl_state = (egl_state_t *) active_context->data;

    if (egl_state->display == EGL_NO_DISPLAY ||
        egl_state->context == EGL_NO_CONTEXT ||
        egl_state->active == false)
        return result;

    /* XXX: post eglSignalSyncNV and wait */
    return result;
}

EGLBoolean
eglGetSyncAttribNV (EGLSyncNV sync, EGLint attribute, EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! active_context)
        return result;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglGetSyncAttribNV and wait */
    return result;
}
#endif

#ifdef EGL_HI_clientpixmap
EGLAPI EGLSurface EGLAPIENTRY
eglCreatePixmapSurfaceHI (EGLDisplay dpy, EGLConfig config,
                          struct EGLClientPixmapHI *pixmap)
{
    EGLSurface result = EGL_NO_SURFACE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglCreatePixmapSurfaceHI and wait */
    return result;
}
#endif

#ifdef EGL_MESA_drm_image
EGLAPI EGLImageKHR EGLAPIENTRY
eglCreateDRMImageMESA (EGLDisplay dpy, const EGLint *attrib_list)
{
    EGLImageKHR result = EGL_NO_IMAGE_KHR;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglCreateDRMImageMESA and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglExportDRMImageMESA (EGLDisplay dpy, EGLImageKHR image,
                       EGLint *name, EGLint *handle, EGLint *stride)
{
    EGLBoolean result = EGL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglExportDRMImageMESA and wait */
    return result;
}
#endif

#ifdef EGL_NV_post_sub_buffer
EGLAPI EGLBoolean EGLAPIENTRY
eglPostSubBufferNV (EGLDisplay dpy, EGLSurface surface,
                    EGLint x, EGLint y,
                    EGLint width, EGLint height)
{
    EGLBoolean result = EGL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglPostSubBufferNV and wait */
    return result;
}
#endif

#ifdef EGL_SEC_image_map
EGLAPI void * EGLAPIENTRY
eglMapImageSEC (EGLDisplay dpy, EGLImageKHR image)
{
    void *result = NULL;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglMapImageSEC and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglUnmapImageSEC (EGLDisplay dpy, EGLImageKHR image)
{
    EGLBoolean result = EGL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglUnmapImageSEC and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetImageAttribSEC (EGLDisplay dpy, EGLImageKHR image, EGLint attribute,
                      EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    command_buffer_t *command_buffer = command_buffer_get_thread_local ();
    if (! command_buffer)
        return result;

    /* XXX: post eglgetImageAttribSEC and wait */
    return result;
}
#endif
