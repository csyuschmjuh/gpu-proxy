#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdlib.h>

#include "config.h"

#include "gpuprocess_command_buffer.h"
#include "gpuprocess_thread_private.h"
#include "gpuprocess_types_private.h"
#include "gpuprocess_egl_server_private.h"

__thread v_link_list_t *active_state
    __attribute__(( tls_model ("initial-exec"))) = NULL;
__thread command_buffer_t* command_buffer
    __attribute__(( tls_model ("initial-exec"))) = NULL;

/* client side cache unpack_alignment */
__thread int unpack_alignment
    __attribute__(( tls_model ("initial-exec"))) = 4;

static inline void
_egl_create_command_buffer ()
{
    if (unlikely(!command_buffer))
        command_buffer = command_buffer_create();
}

static void
_egl_destroy_command_buffer ()
{
    if (command_buffer)
        command_buffer_destroy(command_buffer);
}

EGLAPI EGLint EGLAPIENTRY
eglGetError (void)
{
    EGLint error = EGL_NOT_INITIALIZED;

    _egl_create_command_buffer ();

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

    _egl_create_command_buffer ();

    /* XXX: post eglGetDisplay and wait */
    return display;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglInitialize (EGLDisplay dpy, EGLint *major, EGLint *minor)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

    /* XXX: post eglInitialize and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY eglTerminate (EGLDisplay dpy)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

    /* XXX: post eglTerminate and wait */

    _egl_destroy_command_buffer ();
    return result;
}

EGLAPI const char * EGLAPIENTRY
eglQueryString (EGLDisplay dpy, EGLint name)
{
    const char *result = NULL;

    _egl_create_command_buffer ();

    /* XXX: post eglQueryString and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetConfigs (EGLDisplay dpy, EGLConfig *configs,
               EGLint config_size, EGLint *num_config)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

    /* XXX: post eglGetConfigs and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglChooseConfig (EGLDisplay dpy, const EGLint *attrib_list,
                 EGLConfig *configs, EGLint config_size,
                 EGLint *num_config)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

    /* XXX: post eglChooseConfig and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetConfigAttrib (EGLDisplay dpy, EGLConfig config, EGLint attribute,
                    EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

    /* post eglGetConfigAttrib and wait */
    return result;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreateWindowSurface (EGLDisplay dpy, EGLConfig config,
                        EGLNativeWindowType win,
                        const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    _egl_create_thread ();

    /* XXX: post eglCreateWindowSurface and wait */
    return surface;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreatePbufferSurface (EGLDisplay dpy, EGLConfig config,
                         const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    _egl_create_thread ();

    /* XXX: post eglCreatePbufferSurface and wait */
    return surface;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreatePixmapSurface (EGLDisplay dpy, EGLConfig config,
                        EGLNativePixmapType pixmap,
                        const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    _egl_create_thread ();

    /* XXX: post eglCreatePixmapSurface and wait */
    return surface;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglDestroySurface (EGLDisplay dpy, EGLSurface surface)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_thread ();

    /* XXX: post eglDestroySurface and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglQuerySurface (EGLDisplay dpy, EGLSurface surface,
                 EGLint attribute, EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

    /* XXX: post eglQuerySurface and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglBindAPI (EGLenum api)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

    /* XXX: post eglBindAPI and wait */
    return result;
}

EGLAPI EGLenum EGLAPIENTRY
eglQueryAPI (void)
{
    EGLenum result = EGL_NONE;

    _egl_create_command_buffer ();

    /* XXX: post eglQueryAPI and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglWaitClient (void)
{
    egl_state_t *egl_state;
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

    if (! active_state)
        return EGL_TRUE;

    egl_state = (egl_state_t *) active_state->data;

    if (egl_state->display == EGL_NO_DISPLAY ||
        egl_state->context == EGL_NO_CONTEXT ||
        egl_state->active == FALSE)
        return EGL_TRUE;

    /* XXX: post eglWaitClient and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglReleaseThread (void)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

    /* XXX: post eglReleaseThread and wait */
    return result;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreatePbufferFromClientBuffer (EGLDisplay dpy, EGLenum buftype,
                                  EGLClientBuffer buffer,
                                  EGLConfig config, const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    _egl_create_command_buffer ();

    /* XXX: post eglCreatePbufferFromClientBuffer and wait */
    return surface;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglSurfaceAttrib (EGLDisplay dpy, EGLSurface surface,
                  EGLint attribute, EGLint value)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

    /* XXX: post eglSurfaceAttrib and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglBindTexImage (EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

    /* XXX: post eglBindTexImage and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglReleaseTexImage (EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

    /* XXX: post eglReleaseTexImage and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglSwapInterval (EGLDisplay dpy, EGLint interval)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

    /* XXX: post eglSwapInterval and wait */
    return result;
}

EGLAPI EGLContext EGLAPIENTRY
eglCreateContext (EGLDisplay dpy, EGLConfig config,
                  EGLContext share_context,
                  const EGLint *attrib_list)
{
    EGLContext result = EGL_NO_CONTEXT;

    _egl_create_command_buffer ();

    /* XXX: post eglCreateContext and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglDestroyContext (EGLDisplay dpy, EGLContext ctx)
{
    EGLBoolean result = GL_FALSE;

    _egl_create_command_buffer ();

    /* XXX: post eglDestroyContext and wait */
    return result;
}

EGLAPI EGLContext EGLAPIENTRY
eglGetCurrentContext (void)
{
    egl_state_t *state;

    if (!active_state)
        return EGL_NO_CONTEXT;

    state = (egl_state_t *) active_state->data;
    return state->context;
}

EGLAPI EGLDisplay EGLAPIENTRY
eglGetCurrentDisplay (void)
{
    egl_state_t *state;

    if (!active_state)
        return EGL_NO_DISPLAY;

    state = (egl_state_t *) active_state->data;
    return state->display;
}

EGLAPI EGLSurface EGLAPIENTRY
eglGetCurrentSurface (EGLint readdraw)
{
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

EGLAPI EGLBoolean EGLAPIENTRY
eglQueryContext (EGLDisplay dpy, EGLContext ctx,
                 EGLint attribute, EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

    /* XXX: post eglQueryContext and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglWaitGL (void)
{
    EGLBoolean result = EGL_TRUE;
    egl_state_t *egl_state;

    if (! active_state)
        return result;

    egl_state = (egl_state_t *) active_state->data;

    if (egl_state->display == EGL_NO_DISPLAY ||
        egl_state->context == EGL_NO_CONTEXT ||
        egl_state->active == FALSE)
        return EGL_TRUE;

    _egl_create_command_buffer ();

    /* XXX: post eglWaitGL and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglWaitNative (EGLint engine)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t *state;

    if (! active_state)
        return result;

    _egl_create_command_buffer ();

    /* XXX: post eglWaitNative and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglSwapBuffers (EGLDisplay dpy, EGLSurface surface)
{
    EGLBoolean result = EGL_BAD_DISPLAY;
    egl_state_t *state;

    if (!active_state)
        return result;

    state = (egl_state_t *)active_state->data;
    if (state->display == EGL_NO_DISPLAY)
        return result;

    if (state->display != dpy)
        goto FINISH;

    if (state->readable != surface || state->drawable != surface) {
        result = EGL_BAD_SURFACE;
        goto FINISH;
    }

    _egl_create_command_buffer ();

    /* XXX: post eglSwapBuffers, no wait */

FINISH:
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglCopyBuffers (EGLDisplay dpy, EGLSurface surface,
                EGLNativePixmapType target)
{
    EGLBoolean result = EGL_FALSE;

    egl_create_command_buffer ();

    /* XXX: post eglCopyBuffers and wait */
    return result;
}

EGLAPI __eglMustCastToProperFunctionPointerType EGLAPIENTRY
eglGetProcAddress (const char *procname)
{
    void *address = NULL;

    _egl_create_command_buffer ();

    /* XXX: post eglGetProcAddress and wait */
    return address;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglMakeCurrent (EGLDisplay dpy, EGLSurface draw, EGLSurface read,
                EGLContext ctx)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t        *egl_state;

    _egl_create_command_buffer ();

    /* XXX: we need to pass active_state in command buffer */
    /* we are not in any valid context */
    if (! active_state) {
        if (dpy == EGL_NO_DISPLAY || ctx == EGL_NO_CONTEXT)
            return EGL_TRUE;
        else {
          /* XXX: post eglMakeCurrent and wait */
          /* update active_state */
          return result;
        }
    }
    else {
        if (dpy == EGL_NO_DISPLAY || ctx == EGL_NO_CONTEXT) {
            /* XXX: post eglMakeCurrent and no wait */
            active_state = NULL;
            return EGL_TRUE;
        }
        else {
            egl_state = (egl_state_t *) active_state->data;
            if (egl_state->display == dpy &&
                egl_state->context == ctx &&
                egl_state->drawable == draw &&
                egl_state->readable == read)
                return GL_TRUE;
            else {
                /* XXX: post eglMakeCurrent and wait */
                /* update active_state */
                return result;
            }
        }
    }
    if (active_state) {
        egl_state = (egl_state_t *) active_state->data;
        if (egl_state->active)
            unpack_alignment = egl_state->state.unpack_alignment;
    }
}

/* start of eglext.h */
#ifdef EGL_KHR_lock_surface
EGLAPI EGLBoolean EGLAPIENTRY
eglLockSurfaceKHR (EGLDisplay display, EGLSurface surface,
                   const EGLint *attrib_list)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

    /* XXX: post eglLockSurfaceKHR and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglUnlockSurfaceKHR (EGLDisplay display, EGLSurface surface)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

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

    _egl_create_command_buffer ();

    /* XXX: post eglCreateImageKHR and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglDestroyImageKHR (EGLDisplay dpy, EGLImageKHR image)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

    /* XXX: post eglDestroyImageKHR and wait */
    return result;
}
#endif

#ifdef EGL_KHR_reusable_sync
EGLAPI EGLSyncKHR EGLAPIENTRY
eglCreateSyncKHR (EGLDisplay dpy, EGLenum type, const EGLint *attrib_list)
{
    EGLSyncKHR result = EGL_NO_SYNC_KHR;

    _egl_create_command_buffer ();

    /* XXX: post eglCreateSyncKHR and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglDestroySyncKHR (EGLDisplay dpy, EGLSyncKHR sync)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

    /* XXX: post eglDestroySyncKHR and wait */
    return result;
}

EGLAPI EGLint EGLAPIENTRY
eglClientWaitSyncKHR (EGLDisplay dpy, EGLSyncKHR sync, EGLint flags,
                      EGLTimeKHR timeout)
{
    EGLint result = EGL_FALSE;

    _egl_create_command_buffer ();

    /* XXX: post eglClientWaitSyncKHR and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglSignalSyncKHR (EGLDisplay dpy, EGLSyncKHR sync, EGLenum mode)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

    /* XXX: post eglSignalSyncKHR and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetSyncAttribKHR (EGLDisplay dpy, EGLSyncKHR sync,
                     EGLint attribute, EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

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

    _egl_create_command_buffer ();

    /* XXX: post eglCreateFenceSyncNV and wait */
    return result;
}

EGLBoolean
eglDestroySyncNV (EGLSyncNV sync)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

    /* XXX: post eglDestroyFenceSyncNV and wait */
    return result;
}

EGLBoolean
eglFenceNV (EGLSyncNV sync)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t *egl_state;

    _egl_create_command_buffer ();

    if (! active_state)
        return result;

    egl_state = (egl_state_t *) active_state->data;

    if (egl_state->display == EGL_NO_DISPLAY ||
        egl_state->context == EGL_NO_CONTEXT ||
        egl_state->active == FALSE)
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

    _egl_create_command_buffer ();

    if (! active_state)
        return result;

    egl_state = (egl_state_t *) active_state->data;

    if (egl_state->display == EGL_NO_DISPLAY ||
        egl_state->context == EGL_NO_CONTEXT ||
        egl_state->active == FALSE)
        return result;

    /* XXX: post eglClientWaitSyncNV and wait */
    return result;
}

EGLBoolean
eglSignalSyncNV (EGLSyncNV sync, EGLenum mode)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t *egl_state;

    _egl_create_command_buffer ();

    if (! active_state)
        return result;

    egl_state = (egl_state_t *) active_state->data;

    if (egl_state->display == EGL_NO_DISPLAY ||
        egl_state->context == EGL_NO_CONTEXT ||
        egl_state->active == FALSE)
        return result;

    /* XXX: post eglSignalSyncNV and wait */
    return result;
}

EGLBoolean
eglGetSyncAttribNV (EGLSyncNV sync, EGLint attribute, EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

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

    _egl_create_command_buffer ();

    /* XXX: post eglCreatePixmapSurfaceHI and wait */
    return result;
}
#endif

#ifdef EGL_MESA_drm_image
EGLAPI EGLImageKHR EGLAPIENTRY
eglCreateDRMImageMESA (EGLDisplay dpy, const EGLint *attrib_list)
{
    EGLImageKHR result = EGL_NO_IMAGE_KHR;

    _egl_create_command_buffer ();

    /* XXX: post eglCreateDRMImageMESA and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglExportDRMImageMESA (EGLDisplay dpy, EGLImageKHR image,
                       EGLint *name, EGLint *handle, EGLint *stride)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

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

    _egl_create_command_buffer ();

    /* XXX: post eglPostSubBufferNV and wait */
    return result;
}
#endif

#ifdef EGL_SEC_image_map
EGLAPI void * EGLAPIENTRY
eglMapImageSEC (EGLDisplay dpy, EGLImageKHR image)
{
    void *result = NULL;

    _egl_create_command_buffer ();

    /* XXX: post eglMapImageSEC and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglUnmapImageSEC (EGLDisplay dpy, EGLImageKHR image)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

    /* XXX: post eglUnmapImageSEC and wait */
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetImageAttribSEC (EGLDisplay dpy, EGLImageKHR image, EGLint attribute,
                      EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    _egl_create_command_buffer ();

    /* XXX: post eglgetImageAttribSEC and wait */
    return result;
}
#endif
