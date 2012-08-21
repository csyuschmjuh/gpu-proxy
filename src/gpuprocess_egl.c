#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdlib.h>

#include "config.h"

/* XXX: we should move it to the srv */
#include "gpuprocess_dispatch_private.h"
#include "gpuprocess_egl_states.h"
#include "gpuprocess_gles2_srv_private.h"
gpuprocess_dispatch_t        dispatch;

#include "gpuprocess_thread_private.h"
#include "gpuprocess_types_private.h"
#include "gpuprocess_gles2_cli_private.h"

/* XXX: initialize static mutex on srv */
extern gl_srv_states_t        srv_states;
extern __thread v_link_list_t *active_state;
gpu_mutex_static_init (global_mutex);

EGLAPI EGLint EGLAPIENTRY
eglGetError (void)
{
    EGLint error = EGL_NOT_INITIALIZED;

    if (! dispatch.eglGetError)
        return error;

    error = dispatch.eglGetError();

    return error;
}

/* This is the first call to egl system, we initialize dispatch here,
 * we also initialize gl client states
 */
EGLAPI EGLDisplay EGLAPIENTRY
eglGetDisplay (EGLNativeDisplayType display_id)
{
    EGLDisplay display = EGL_NO_DISPLAY;
    egl_state_t *state;
    
    if (!active_state)
        return display;

    state = (egl_state_t *) active_state->data;

    gpu_mutex_lock (global_mutex);

    /* XXX: we should initialize on srv */
    gpuprocess_dispatch_init (&dispatch);
    /* XXX: this should be initialized in srv */
    _gpuprocess_srv_init ();
    gpu_mutex_unlock (global_mutex);

    display = dispatch.eglGetDisplay (display_id);

    return display;
}
    
EGLAPI EGLBoolean EGLAPIENTRY
eglInitialize (EGLDisplay dpy, EGLint *major, EGLint *minor)
{
    EGLBoolean result = EGL_FALSE;

    if (dispatch.eglInitialize) 
        result = dispatch.eglInitialize (dpy, major, minor);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY eglTerminate (EGLDisplay dpy)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t *state;

    gpu_mutex_lock (global_mutex);

    if (dispatch.eglTerminate) {
        result = dispatch.eglTerminate (dpy);

        if (result == EGL_TRUE) {
              gpu_mutex_lock (global_mutex);

            /* XXX: remove srv structure */
            _gpuprocess_srv_terminate (dpy);

            gpu_mutex_unlock (global_mutex);

        }
    }

    return result;
}

EGLAPI const char * EGLAPIENTRY
eglQueryString (EGLDisplay dpy, EGLint name)
{
    const char *result = NULL;

    if (dispatch.eglQueryString)
        result = dispatch.eglQueryString (dpy, name);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetConfigs (EGLDisplay dpy, EGLConfig *configs, 
               EGLint config_size, EGLint *num_config)
{
    EGLBoolean result = EGL_FALSE;

    if (dispatch.eglGetConfigs)
        result = dispatch.eglGetConfigs (dpy, configs, config_size, num_config);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglChooseConfig (EGLDisplay dpy, const EGLint *attrib_list,
                 EGLConfig *configs, EGLint config_size, 
                 EGLint *num_config)
{
    EGLBoolean result = EGL_FALSE;

    if (dispatch.eglChooseConfig) 
        result = dispatch.eglChooseConfig (dpy, attrib_list, configs,
                                           config_size, num_config);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetConfigAttrib (EGLDisplay dpy, EGLConfig config, EGLint attribute, 
                    EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (dispatch.eglGetConfigAttrib)
        result = dispatch.eglGetConfigAttrib (dpy, config, attribute, value);

    return result;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreateWindowSurface (EGLDisplay dpy, EGLConfig config,
                        EGLNativeWindowType win, 
                        const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    if (dispatch.eglCreateWindowSurface)
        surface = dispatch.eglCreateWindowSurface (dpy, config, win,
                                                   attrib_list);

    return surface;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreatePbufferSurface (EGLDisplay dpy, EGLConfig config,
                         const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;
    
    if (dispatch.eglCreatePbufferSurface)
        surface = dispatch.eglCreatePbufferSurface (dpy, config, attrib_list);

    return surface;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreatePixmapSurface (EGLDisplay dpy, EGLConfig config,
                        EGLNativePixmapType pixmap, 
                        const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;
    
    if (dispatch.eglCreatePixmapSurface)
        surface = dispatch.eglCreatePixmapSurface (dpy, config, pixmap,
                                                   attrib_list);

    return surface;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglDestroySurface (EGLDisplay dpy, EGLSurface surface)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t *state;

    if (!active_state)
        return result;

    if (dispatch.eglDestroySurface) { 
        result = dispatch.eglDestroySurface (dpy, surface);
        
        if (result == EGL_TRUE) {
            /* update srv states */
            gpu_mutex_lock (global_mutex);
            _gpuprocess_srv_destroy_surface (dpy, surface);
            gpu_mutex_unlock (global_mutex);
        }
    }

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglQuerySurface (EGLDisplay dpy, EGLSurface surface,
                 EGLint attribute, EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (dispatch.eglQuerySurface) 
        result = dispatch.eglQuerySurface (dpy, surface, attribute, value);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglBindAPI (EGLenum api)
{
    EGLBoolean result = EGL_FALSE;

    if (dispatch.eglBindAPI) 
        result = dispatch.eglBindAPI (api);

    return result;
}

EGLAPI EGLenum EGLAPIENTRY 
eglQueryAPI (void)
{
    EGLenum result = EGL_NONE;

    if (dispatch.eglQueryAPI) 
        result = dispatch.eglQueryAPI ();

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglWaitClient (void)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t *state;

    if (!active_state)
        return result;

    state = (egl_state_t *) active_state->data;
    if (state->context == EGL_NO_CONTEXT || 
        state->display == EGL_NO_DISPLAY)
        return result;
    
    gpu_mutex_lock (state->mutex);
    /* XXX: we should create a command buffer for it, and wait for signal */
    if (dispatch.eglWaitClient)
        result = dispatch.eglWaitClient ();

    gpu_mutex_unlock (state->mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglReleaseThread (void)
{
    EGLBoolean result = EGL_FALSE;
    v_bool_t success;
    egl_state_t *state;

    if (dispatch.eglReleaseThread) {
        result = dispatch.eglReleaseThread ();

        if (result = EGL_TRUE) {
            if (! active_state)
                return result;

            gpu_mutex_lock (global_mutex);

            success = _gpuprocess_srv_make_current (state->display,
                                                    EGL_NO_SURFACE,
                                                    EGL_NO_SURFACE,
                                                    EGL_NO_CONTEXT,
                                                    state->display,
                                                    state->drawable,
                                                    state->readable,
                                                    state->context,
                                                    &active_state);

            gpu_mutex_unlock (global_mutex);
        }
    }

    return result;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreatePbufferFromClientBuffer (EGLDisplay dpy, EGLenum buftype,
                                  EGLClientBuffer buffer, 
                                  EGLConfig config, const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;
    
    if (dispatch.eglCreatePbufferFromClientBuffer)
        surface = dispatch.eglCreatePbufferFromClientBuffer (dpy, buftype,
                                                             buffer, config,
                                                             attrib_list);

    return surface;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglSurfaceAttrib (EGLDisplay dpy, EGLSurface surface, 
                  EGLint attribute, EGLint value)
{
    EGLBoolean result = EGL_FALSE;
    
    if (dispatch.eglSurfaceAttrib)
        result = dispatch.eglSurfaceAttrib (dpy, surface, attribute, value);

    return result;
}
    
EGLAPI EGLBoolean EGLAPIENTRY
eglBindTexImage (EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    EGLBoolean result = EGL_FALSE;
    
    if (dispatch.eglBindTexImage)
        result = dispatch.eglBindTexImage (dpy, surface, buffer);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglReleaseTexImage (EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    EGLBoolean result = EGL_FALSE;
    
    if (dispatch.eglReleaseTexImage)
        result = dispatch.eglReleaseTexImage (dpy, surface, buffer);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglSwapInterval (EGLDisplay dpy, EGLint interval)
{
    EGLBoolean result = EGL_FALSE;
    
    if (dispatch.eglSwapInterval)
        result = dispatch.eglSwapInterval (dpy, interval);

    return result;
}

EGLAPI EGLContext EGLAPIENTRY
eglCreateContext (EGLDisplay dpy, EGLConfig config,
                  EGLContext share_context,
                  const EGLint *attrib_list)
{
    EGLContext result = EGL_NO_CONTEXT;
    
    if (dispatch.eglCreateContext)
        result = dispatch.eglCreateContext (dpy, config, share_context, 
                                            attrib_list);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglDestroyContext (EGLDisplay dpy, EGLContext ctx)
{
    EGLBoolean result = GL_FALSE;
    egl_state_t *state;

    gpu_mutex_lock (global_mutex);

    if (dispatch.eglDestroyContext) {
        result = dispatch.eglDestroyContext (dpy, ctx); 

        if (result == GL_TRUE) {
            gpu_mutex_lock (global_mutex);
            _gpuprocess_srv_destroy_context (dpy, ctx);
            gpu_mutex_unlock (global_mutex);
        }
    }

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

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglQueryContext)
        return result;
    
    result = dispatch.eglQueryContext (dpy, ctx, attribute, value);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglWaitGL (void)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t *state;

    if (! active_state)
        return result;

    if (! dispatch.eglWaitGL)
        return result;

    state = (egl_state_t *) active_state->data;

    gpu_mutex_lock (state->mutex);

    /* XXX: We should put a command buffer instead calling here */
    result = dispatch.eglWaitGL ();

    gpu_mutex_unlock (state->mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglWaitNative (EGLint engine)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t *state;

    if (! active_state)
        return result;

    if (! dispatch.eglWaitNative)
        return result;
    
    state = (egl_state_t *) active_state->data;

    gpu_mutex_lock (state->mutex);

    /* XXX: We should put a command buffer instead calling here */
    result = dispatch.eglWaitNative (engine);

    gpu_mutex_unlock (state->mutex);

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

    gpu_mutex_lock (state->mutex);

    /* put in a command buffer */
    dispatch.eglSwapBuffers (dpy, surface);
    result = EGL_TRUE;
    gpu_mutex_unlock (state->mutex);

FINISH:
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY 
eglCopyBuffers (EGLDisplay dpy, EGLSurface surface, 
                EGLNativePixmapType target)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglCopyBuffers)
        return result;

    result = dispatch.eglCopyBuffers (dpy, surface, target);

    return result;
}

EGLAPI __eglMustCastToProperFunctionPointerType EGLAPIENTRY
eglGetProcAddress (const char *procname)
{
    return dispatch.GetProcAddress;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglMakeCurrent (EGLDisplay dpy, EGLSurface draw, EGLSurface read,
                EGLContext ctx)
{
    EGLBoolean result = EGL_FALSE;
    v_bool_t needs_call = FALSE;
    egl_state_t        *state;
    EGLDisplay current_dpy;
    EGLContext current_ctx;
    EGLSurface current_read;
    EGLSurface current_draw;
    v_bool_t success = TRUE;

    if (! dispatch.eglMakeCurrent)
        return result;

    /* we are not in any valid context */
    if (! active_state) {
        if (dpy == EGL_NO_DISPLAY || ctx == EGL_NO_CONTEXT)
            return EGL_TRUE;
    }

    if (! active_state && ! 
        (dpy == EGL_NO_DISPLAY ||
         ctx == EGL_NO_CONTEXT)) {
        /* we switch to one of the valid context */
        gpu_mutex_lock (global_mutex);
        success = _gpuprocess_srv_make_current (dpy, draw,
                                                read, ctx,
                                                EGL_NO_DISPLAY,
                                                EGL_NO_SURFACE,
                                                EGL_NO_SURFACE,
                                                EGL_NO_CONTEXT,
                                                &active_state);
        gpu_mutex_unlock (global_mutex);
    }
    /* we are in a valid context */
    else {
        gpu_mutex_lock (global_mutex);
        state = (egl_state_t *) active_state->data;

        /* we are switch to an invalid context */
        if (dpy == EGL_NO_SURFACE || EGL_NO_CONTEXT) {
            state->ref_count --;
            current_dpy = state->display;
            current_ctx = state->context;
            current_read = state->readable;
            current_draw = state->drawable;

            if (state->ref_count == 0)
                state->active = FALSE;
            if (state->destroy_dpy && ! state->active) {
                if (active_state->next)
                    active_state->next->prev = active_state->prev;
                if (active_state->prev)
                    active_state->prev->next = active_state->next;

                /* make current to none on the srv thread, and terminate
                 * the srv thread
                 */
                free (state);
                free (active_state);
            }
            /* look for other states that have same dpy */
            success = _gpuprocess_srv_make_current (dpy, draw,
                                                    read, ctx,
                                                    current_dpy,
                                                    current_draw,
                                                    current_read,
                                                    current_ctx,
                                                    &active_state);
        }
        /* we are switch to a same context */
        else if (dpy == state->display &&
                 ctx == state->context) {
            /* previous DestroySurface called on drawable  */
            if (state->drawable != draw && state->destroy_draw)
                _gpuprocess_srv_destroy_surface (dpy, state->drawable);
            state->drawable = draw;

            /* previous DestroySurface called on readable */
            if (state->readable != read && state->destroy_read)
                _gpuprocess_srv_destroy_surface (dpy, state->readable);
            state->readable = read;
        }
        /* we are switch to another valid context */
        else {
            success = _gpuprocess_srv_make_current (dpy, draw,
                                                    read, ctx,
                                                    current_dpy,
                                                    current_draw,
                                                    current_read,
                                                    current_ctx,
                                                    &active_state);
        }
        gpu_mutex_unlock (global_mutex);
    }


    if (success == TRUE)
        result = GL_TRUE;

    return result;
}

/* start of eglext.h */
#ifdef EGL_KHR_lock_surface
EGLAPI EGLBoolean EGLAPIENTRY
eglLockSurfaceKHR (EGLDisplay display, EGLSurface surface,
                   const EGLint *attrib_list)
{
    EGLBoolean result = EGL_FALSE;
    
    if (! dispatch.eglLockSurfaceKHR)
        return result;

    result = dispatch.eglLockSurfaceKHR (display, surface, attrib_list);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglUnlockSurfaceKHR (EGLDisplay display, EGLSurface surface)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglUnlockSurfaceKHR)
        return result;
    
    result = dispatch.eglUnlockSurfaceKHR (display, surface);

    return result;
}
#endif

#ifdef EGL_KHR_image
EGLAPI EGLImageKHR EGLAPIENTRY
eglCreateImageKHR (EGLDisplay dpy, EGLContext ctx, EGLenum target,
                   EGLClientBuffer buffer, const EGLint *attrib_list)
{
    EGLImageKHR result = EGL_NO_IMAGE_KHR;

    if (! dispatch.eglCreateImageKHR)
        return result;

    if (dpy == EGL_NO_DISPLAY)
        return result;
    
    result = dispatch.eglCreateImageKHR (dpy, ctx, target,
                                         buffer, attrib_list);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglDestroyImageKHR (EGLDisplay dpy, EGLImageKHR image)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglDestroyImageKHR)
        return result;
    
    result = dispatch.eglDestroyImageKHR (dpy, image);

    return result;
}
#endif

#ifdef EGL_KHR_reusable_sync
EGLAPI EGLSyncKHR EGLAPIENTRY
eglCreateSyncKHR (EGLDisplay dpy, EGLenum type, const EGLint *attrib_list)
{
    EGLSyncKHR result = EGL_NO_SYNC_KHR;

    if (! dispatch.eglCreateSyncKHR)
        return result;
    
    result = dispatch.eglCreateSyncKHR (dpy, type, attrib_list);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglDestroySyncKHR (EGLDisplay dpy, EGLSyncKHR sync)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglDestroySyncKHR)
        return result;

    result = dispatch.eglDestroySyncKHR (dpy, sync);

    return result;
}

EGLAPI EGLint EGLAPIENTRY
eglClientWaitSyncKHR (EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, 
                      EGLTimeKHR timeout)
{
    EGLint result = EGL_FALSE;

    if (! dispatch.eglClientWaitSyncKHR)
        return result;
    
    state = active_state->data;
    gpu_mutex_lock (state->mutex);
    
    result = dispatch.eglClientWaitSyncKHR (dpy, sync, flags, timeout);
    gpu_mutex_unlock (state->mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglSignalSyncKHR (EGLDisplay dpy, EGLSyncKHR sync, EGLenum mode)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglSignalSyncKHR)
        return result;
    
    result = dispatch.eglSignalSyncKHR (dpy, sync, mode);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetSyncAttribKHR (EGLDisplay dpy, EGLSyncKHR sync,
                     EGLint attribute, EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglGetSyncAttribKHR)
        return result;
    
    result = dispatch.eglGetSyncAttribKHR (dpy, sync, attribute, value);

    return result;
}
#endif

#ifdef EGL_NV_sync
EGLSyncNV 
eglCreateFenceSyncNV (EGLDisplay dpy, EGLenum condition, 
                      const EGLint *attrib_list)
{
    EGLSyncNV result = EGL_NO_SYNC_NV;

    if (! dispatch.eglCreateFenceSyncNV)
        return result;
    
    result = dispatch.eglCreateFenceSyncNV (dpy, condition, attrib_list);

    return result;
}

EGLBoolean 
eglDestroySyncNV (EGLSyncNV sync) 
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglDestroySyncNV)
        return result;
    
    result = dispatch.eglDestroySyncNV (sync);

    return result;
}

EGLBoolean
eglFenceNV (EGLSyncNV sync)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t *state;

    if (! active_state || ! dispatch.eglFenceNV)
        return result;

    state = active_state->data;
    gpu_mutex_lock (state->mutex);

    /* XXX: We should put a command buffer instead calling here */
    result = dispatch.eglFenceNV (sync);

    gpu_mutex_unlock (state->mutex);

    return result;
}

EGLint
eglClientWaitSyncNV (EGLSyncNV sync, EGLint flags, EGLTimeNV timeout)
{
    /* XXX: is this supposed to be default value ? */
    EGLint result = EGL_TIMEOUT_EXPIRED_NV;
    egl_state_t *state;

    if (! active_state || ! dispatch.eglClientWaitSyncNV)
        return result;

    state = active_state->data;

    gpu_mutex_lock (state->mutex);

    /* XXX: We should put a command buffer instead calling here */
    result = dispatch.eglClientWaitSyncNV (sync, flags, timeout);

    gpu_mutex_unlock (state->mutex);

    return result;
}

EGLBoolean
eglSignalSyncNV (EGLSyncNV sync, EGLenum mode)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t *state;

    if (! active_state || ! dispatch.eglSignalSyncNV)
        return result;


    /* XXX: We should put a command buffer instead calling here */
    result = dispatch.eglSignalSyncNV (sync, mode);

    return result;
}

EGLBoolean
eglGetSyncAttribNV (EGLSyncNV sync, EGLint attribute, EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglGetSyncAttribNV)
        return result;

    result = dispatch.eglGetSyncAttribNV (sync, attribute, value);

    return result;
}
#endif

#ifdef EGL_HI_clientpixmap
EGLAPI EGLSurface EGLAPIENTRY
eglCreatePixmapSurfaceHI (EGLDisplay dpy, EGLConfig config,
                          struct EGLClientPixmapHI *pixmap)
{
    EGLSurface result = EGL_NO_SURFACE;

    if (! dispatch.eglCreatePixmapSurfaceHI)
        return result;
    
    result = dispatch.eglCreatePixmapSurfaceHI (dpy, config, pixmap);

    return result;
}
#endif

#ifdef EGL_MESA_drm_image
EGLAPI EGLImageKHR EGLAPIENTRY
eglCreateDRMImageMESA (EGLDisplay dpy, const EGLint *attrib_list)
{
    EGLImageKHR result = EGL_NO_IMAGE_KHR;

    if (! dispatch.eglCreateDRMImageMESA)
        return result;
    
    result = dispatch.eglCreateDRMImageMESA (dpy, attrib_list);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglExportDRMImageMESA (EGLDisplay dpy, EGLImageKHR image,
                       EGLint *name, EGLint *handle, EGLint *stride)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglExportDRMImageMESA)
        return result;
    
    result = dispatch.eglExportDRMImageMESA (dpy, image, name, handle, stride);

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

    if (! dispatch.eglExportDRMImageMESA)
        return result;

    result = dispatch.eglPostSubBufferNV (dpy, surface, x, y, width, height);

    return result;
}
#endif

#ifdef EGL_SEC_image_map
EGLAPI void * EGLAPIENTRY
eglMapImageSEC (EGLDisplay dpy, EGLImageKHR image)
{
    void *result = NULL;

    if (! dispatch.eglMapImageSEC)
        return result;
    
    result = dispatch.eglMapImageSEC (dpy, image);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY 
eglUnmapImageSEC (EGLDisplay dpy, EGLImageKHR image)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglUnmapImageSEC)
        return result;
    
    result = dispatch.eglUnmapImageSEC (dpy, image);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetImageAttribSEC (EGLDisplay dpy, EGLImageKHR image, EGLint attribute,
                      EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglGetImageAttribSEC)
        return result;
    
    result = dispatch.eglGetImageAttribSEC (dpy, image, attribute, value);

    return result;
}
#endif
