/* Implemented egl and eglext.h functions.
 * This is the egl functions entry point in the client thread.
 *
 * The server thread is initialized and be ready during eglGetDisplay()
 * and is terminated during eglTerminate() when the current context is
 * None. If a client calls egl functions before eglGetDisplay(), an error
 * is returned.
 * Variables used in the code:
 * (1) active_context - a pointer to the current state that is in
 * global server_states.  If it is NULL, the current client is not in
 * any valid egl context.
 * (2) command_buffer - a pointer to the command buffer that is shared
 * between client thread and server thread.  The client posts command to
 * command buffer, while the server thread reads from the command from
 * the command buffer
 */

#include "config.h"

#include "client.h"
#include "thread_private.h"
#include "types_private.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdlib.h>

EGLAPI EGLint EGLAPIENTRY
eglGetError (void)
{
    EGLint error = EGL_NOT_INITIALIZED;

    if (! client_get_thread_local ())
        return error;

    /* XXX: post eglGetError() and wait */

    return error;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglInitialize (EGLDisplay dpy,
               EGLint *major,
               EGLint *minor)
{
    EGLBoolean result = EGL_FALSE;

    if (! client_get_thread_local ())
        return result;

    /* XXX: post eglInitialize and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY eglTerminate (EGLDisplay dpy)
{
    EGLBoolean result = EGL_FALSE;

    if (! client_get_thread_local ())
        return result;

    /* XXX: post eglTerminate and wait */

    /* FIXME: only if we are in None Context, we will destroy the server
     * according to egl spec.  What happens if we are in valid context
     * and application exit?  Obviously, there are still valid context
     * on the driver side, what about our server thread ?
     */
    if (! client_active_egl_state_available ())
        client_destroy_thread_local ();

    return result;
}

EGLAPI const char * EGLAPIENTRY
eglQueryString (EGLDisplay dpy, EGLint name)
{
    const char *result = NULL;

    if (! client_get_thread_local ())
        return result;

    /* XXX: post eglQueryString and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetConfigs (EGLDisplay dpy,
               EGLConfig *configs,
               EGLint config_size,
               EGLint *num_config)
{
    EGLBoolean result = EGL_FALSE;

    if (! client_get_thread_local ())
        return result;

    /* XXX: post eglGetConfigs and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglChooseConfig (EGLDisplay dpy,
                 const EGLint *attrib_list,
                 EGLConfig *configs,
                 EGLint config_size,
                 EGLint *num_config)
{
    EGLBoolean result = EGL_FALSE;

    if (! client_get_thread_local ())
        return result;

    /* XXX: post eglChooseConfig and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetConfigAttrib (EGLDisplay dpy,
                    EGLConfig config,
                    EGLint attribute,
                    EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! client_get_thread_local ())
        return result;

    /* post eglGetConfigAttrib and wait */
    return result;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreateWindowSurface (EGLDisplay dpy,
                        EGLConfig config,
                        EGLNativeWindowType win,
                        const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    if (! client_get_thread_local ())
        return surface;

    /* XXX: post eglCreateWindowSurface and wait */
    return surface;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreatePbufferSurface (EGLDisplay dpy,
                         EGLConfig config,
                         const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    if (! client_get_thread_local ())
        return surface;

    /* XXX: post eglCreatePbufferSurface and wait */
    return surface;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreatePixmapSurface (EGLDisplay dpy,
                        EGLConfig config,
                        EGLNativePixmapType pixmap,
                        const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    if (! client_get_thread_local ())
        return surface;

    /* XXX: post eglCreatePixmapSurface and wait */
    return surface;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglQuerySurface (EGLDisplay dpy,
                 EGLSurface surface,
                 EGLint attribute,
                 EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! client_get_thread_local ())
        return result;

    /* XXX: post eglQuerySurface and wait */
    return result;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreatePbufferFromClientBuffer (EGLDisplay dpy,
                                  EGLenum buftype,
                                  EGLClientBuffer buffer,
                                  EGLConfig config,
                                  const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    if (! client_get_thread_local ())
        return surface;

    /* XXX: post eglCreatePbufferFromClientBuffer and wait */
    return surface;
}

EGLAPI EGLContext EGLAPIENTRY
eglCreateContext (EGLDisplay dpy,
                  EGLConfig config,
                  EGLContext share_context,
                  const EGLint *attrib_list)
{
    EGLContext result = EGL_NO_CONTEXT;

    if (! client_get_thread_local ())
        return result;

    /* XXX: post eglCreateContext and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglQueryContext (EGLDisplay dpy,
                 EGLContext ctx,
                 EGLint attribute,
                 EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! client_get_thread_local ())
        return result;

    /* XXX: post eglQueryContext and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglSwapBuffers (EGLDisplay dpy,
                EGLSurface surface)
{
    EGLBoolean result = EGL_BAD_DISPLAY;
    egl_state_t *egl_state;

    if (! client_active_egl_state_available ())
        return result;

    egl_state = client_get_active_egl_state ();
    if (egl_state->display == EGL_NO_DISPLAY)
        return result;

    if (egl_state->display != dpy)
        goto FINISH;

    if (egl_state->readable != surface || egl_state->drawable != surface) {
        result = EGL_BAD_SURFACE;
        goto FINISH;
    }

    /* XXX: post eglSwapBuffers, no wait */

FINISH:
    return result;
}

EGLAPI __eglMustCastToProperFunctionPointerType EGLAPIENTRY
eglGetProcAddress (const char *procname)
{
    void *address = NULL;

    if (! client_get_thread_local ())
        return address;

    /* XXX: post eglGetProcAddress and wait */
    return address;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglMakeCurrent (EGLDisplay dpy,
                EGLSurface draw,
                EGLSurface read,
                EGLContext ctx)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t *egl_state = NULL;

    if (! client_get_thread_local ())
        return result;

    /* XXX: we need to pass active_context in command buffer */
    /* we are not in any valid context */
    if (! client_active_egl_state_available ()) {
        if (dpy == EGL_NO_DISPLAY || ctx == EGL_NO_CONTEXT) {
            result = EGL_TRUE;
            return result;
            
        }
        else {
          /* XXX: post eglMakeCurrent and wait */
          /* update active_context */
          return EGL_TRUE;
        }
    } else {
        if (dpy == EGL_NO_DISPLAY || ctx == EGL_NO_CONTEXT) {
            /* XXX: post eglMakeCurrent and no wait */
            return EGL_TRUE;
        }
        else {
            egl_state = client_get_active_egl_state ();
            if (egl_state->display == dpy &&
                egl_state->context == ctx &&
                egl_state->drawable == draw &&
                egl_state->readable == read) {
                if (egl_state->destroy_dpy)
                    return EGL_FALSE;
                else
                    return EGL_TRUE;
            }
            else {
                /* XXX: post eglMakeCurrent and wait */
                /* update active_context */
                return EGL_TRUE;
            }
        }
    }
    return result;
}

/* start of eglext.h */
#ifdef EGL_KHR_lock_surface
EGLAPI EGLBoolean EGLAPIENTRY
eglLockSurfaceKHR (EGLDisplay display,
                   EGLSurface surface,
                   const EGLint *attrib_list)
{
    EGLBoolean result = EGL_FALSE;

    if (! client_get_thread_local ())
        return result;

    /* XXX: post eglLockSurfaceKHR and wait */
    return result;
}
#endif

#ifdef EGL_KHR_image
EGLAPI EGLImageKHR EGLAPIENTRY
eglCreateImageKHR (EGLDisplay dpy,
                   EGLContext ctx,
                   EGLenum target,
                   EGLClientBuffer buffer,
                   const EGLint *attrib_list)
{
    EGLImageKHR result = EGL_NO_IMAGE_KHR;

    if (! client_get_thread_local ())
        return result;

    /* XXX: post eglCreateImageKHR and wait */
    return result;
}
#endif

#ifdef EGL_KHR_reusable_sync
EGLAPI EGLSyncKHR EGLAPIENTRY
eglCreateSyncKHR (EGLDisplay dpy,
                  EGLenum type,
                  const EGLint *attrib_list)
{
    EGLSyncKHR result = EGL_NO_SYNC_KHR;

    if (! client_get_thread_local ())
        return result;

    /* XXX: post eglCreateSyncKHR and wait */
    return result;
}

EGLAPI EGLint EGLAPIENTRY
eglClientWaitSyncKHR (EGLDisplay dpy,
                      EGLSyncKHR sync,
                      EGLint flags,
                      EGLTimeKHR timeout)
{
    EGLint result = EGL_FALSE;

    if (! client_get_thread_local ())
        return result;

    /* XXX: post eglClientWaitSyncKHR and wait */
    return result;
}
#endif

#ifdef EGL_NV_sync
EGLSyncNV
eglCreateFenceSyncNV (EGLDisplay dpy,
                      EGLenum condition,
                      const EGLint *attrib_list)
{
    EGLSyncNV result = EGL_NO_SYNC_NV;

    if (! client_get_thread_local ())
        return result;

    /* XXX: post eglCreateFenceSyncNV and wait */
    return result;
}

EGLint
eglClientWaitSyncNV (EGLSyncNV sync,
                     EGLint flags,
                     EGLTimeNV timeout)
{
    /* XXX: is this supposed to be default value ? */
    EGLint result = EGL_TIMEOUT_EXPIRED_NV;
    egl_state_t *egl_state;

    if (! client_active_egl_state_available  ())
        return result;

    egl_state = client_get_active_egl_state ();

    if (egl_state->display == EGL_NO_DISPLAY ||
        egl_state->context == EGL_NO_CONTEXT ||
        egl_state->active == false)
        return result;

    /* XXX: post eglClientWaitSyncNV and wait */
    return result;
}

EGLBoolean
eglGetSyncAttribNV (EGLSyncNV sync,
                    EGLint attribute,
                    EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! client_active_egl_state_available  ())
        return result;

    /* XXX: post eglGetSyncAttribNV and wait */
    return result;
}
#endif

#ifdef EGL_HI_clientpixmap
EGLAPI EGLSurface EGLAPIENTRY
eglCreatePixmapSurfaceHI (EGLDisplay dpy,
                          EGLConfig config,
                          struct EGLClientPixmapHI *pixmap)
{
    EGLSurface result = EGL_NO_SURFACE;

    if (! client_get_thread_local ())
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

    if (! client_get_thread_local ())
        return result;

    /* XXX: post eglCreateDRMImageMESA and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglExportDRMImageMESA (EGLDisplay dpy, EGLImageKHR image,
                       EGLint *name, EGLint *handle, EGLint *stride)
{
    EGLBoolean result = EGL_FALSE;

    if (! client_get_thread_local ())
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

    if (! client_get_thread_local ())
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

    if (! client_get_thread_local ())
        return result;

    /* XXX: post eglMapImageSEC and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglUnmapImageSEC (EGLDisplay dpy, EGLImageKHR image)
{
    EGLBoolean result = EGL_FALSE;

    if (! client_get_thread_local ())
        return result;

    /* XXX: post eglUnmapImageSEC and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetImageAttribSEC (EGLDisplay dpy,
                      EGLImageKHR image,
                      EGLint attribute,
                      EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! client_get_thread_local ())
        return result;

    /* XXX: post eglgetImageAttribSEC and wait */
    return result;
}
#endif
