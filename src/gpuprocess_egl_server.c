#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdlib.h>

#include "config.h"

/* XXX: we should move it to the srv */
#include "gpuprocess_dispatch_private.h"
#include "gpuprocess_egl_states.h"
#include "gpuprocess_egl_server_private.h"

gpuprocess_dispatch_t        dispatch;
extern gpu_mutex_t           mutex;
extern __thread v_link_list_t *active_state;

#include "gpuprocess_types_private.h"

/* XXX: initialize static mutex on srv */
extern gl_server_states_t        srv_states;

static EGLint
_egl_get_error (void)
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
static EGLDisplay
_egl_get_display (EGLNativeDisplayType display_id)
{
    EGLDisplay display = EGL_NO_DISPLAY;
   
    /* XXX: we should initialize once for both dispatch and srv structure */
    gpu_mutex_lock (mutex);
    gpuprocess_dispatch_init (&dispatch);
    _gpuprocess_server_init ();
    gpu_mutex_unlock (mutex);

    display = dispatch.eglGetDisplay (display_id);

    return display;
}
    
static EGLBoolean
_egl_initialize (EGLDisplay dpy, EGLint *major, EGLint *minor)
{
    EGLBoolean result = EGL_FALSE;

    if (dispatch.eglInitialize) 
        result = dispatch.eglInitialize (dpy, major, minor);

    return result;
}

static EGLBoolean
_egl_terminate (EGLDisplay dpy)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t *state;

    if (dispatch.eglTerminate) {
        result = dispatch.eglTerminate (dpy);

        if (result == EGL_TRUE) {
            /* XXX: remove srv structure */
            _gpuprocess_server_terminate (dpy, active_state);
        }
    }

    return result;
}

static const char *
_egl_query_string (EGLDisplay dpy, EGLint name)
{
    const char *result = NULL;

    if (dispatch.eglQueryString)
        result = dispatch.eglQueryString (dpy, name);

    return result;
}

static EGLBoolean
_egl_get_configs (EGLDisplay dpy, EGLConfig *configs, 
                  EGLint config_size, EGLint *num_config)
{
    EGLBoolean result = EGL_FALSE;

    if (dispatch.eglGetConfigs)
        result = dispatch.eglGetConfigs (dpy, configs, config_size, num_config);

    return result;
}

static EGLBoolean
_egl_choose_config (EGLDisplay dpy, const EGLint *attrib_list,
                    EGLConfig *configs, EGLint config_size, 
                    EGLint *num_config)
{
    EGLBoolean result = EGL_FALSE;

    if (dispatch.eglChooseConfig) 
        result = dispatch.eglChooseConfig (dpy, attrib_list, configs,
                                           config_size, num_config);

    return result;
}

static EGLBoolean
_egl_get_config_attrib (EGLDisplay dpy, EGLConfig config, EGLint attribute, 
                        EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (dispatch.eglGetConfigAttrib)
        result = dispatch.eglGetConfigAttrib (dpy, config, attribute, value);

    return result;
}

static EGLSurface
_egl_create_window_surface (EGLDisplay dpy, EGLConfig config,
                            EGLNativeWindowType win, 
                            const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    if (dispatch.eglCreateWindowSurface)
        surface = dispatch.eglCreateWindowSurface (dpy, config, win,
                                                   attrib_list);

    return surface;
}

static EGLSurface
_egl_create_pbuffer_surface (EGLDisplay dpy, EGLConfig config,
                             const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;
    
    if (dispatch.eglCreatePbufferSurface)
        surface = dispatch.eglCreatePbufferSurface (dpy, config, attrib_list);

    return surface;
}

static EGLSurface 
_egl_create_pixman_surface (EGLDisplay dpy, EGLConfig config,
                            EGLNativePixmapType pixmap, 
                            const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;
    
    if (dispatch.eglCreatePixmapSurface)
        surface = dispatch.eglCreatePixmapSurface (dpy, config, pixmap,
                                                   attrib_list);

    return surface;
}

static EGLBoolean
_egl_destroy_surface (EGLDisplay dpy, EGLSurface surface)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t *state;

    if (!active_state)
        return result;

    if (dispatch.eglDestroySurface) { 
        result = dispatch.eglDestroySurface (dpy, surface);
        
        if (result == EGL_TRUE) {
            /* update srv states */
            _gpuprocess_server_destroy_surface (dpy, surface, active_state);
        }
    }

    return result;
}

static EGLBoolean
_egl_query_surface (EGLDisplay dpy, EGLSurface surface,
                    EGLint attribute, EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (dispatch.eglQuerySurface) 
        result = dispatch.eglQuerySurface (dpy, surface, attribute, value);

    return result;
}

static EGLBoolean
_egl_bind_api (EGLenum api)
{
    EGLBoolean result = EGL_FALSE;

    if (dispatch.eglBindAPI) 
        result = dispatch.eglBindAPI (api);

    return result;
}

static EGLenum 
_egl_query_api (void)
{
    EGLenum result = EGL_NONE;

    if (dispatch.eglQueryAPI) 
        result = dispatch.eglQueryAPI ();

    return result;
}

static EGLBoolean
_egl_wait_client (void)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t *state;

    if (dispatch.eglWaitClient)
        result = dispatch.eglWaitClient ();

    return result;
}

static EGLBoolean
_egl_release_thread (void)
{
    EGLBoolean result = EGL_FALSE;
    v_bool_t success;
    egl_state_t *egl_state;
    v_link_list_t *active_state_out = NULL;

    if (dispatch.eglReleaseThread) {
        result = dispatch.eglReleaseThread ();

        if (result = EGL_TRUE) {
            if (! active_state)
                return result;
            
            egl_state = (egl_state_t *) active_state->data;

            _gpuprocess_server_make_current (egl_state->display,
                                             EGL_NO_SURFACE,
                                             EGL_NO_SURFACE,
                                             EGL_NO_CONTEXT,
                                             active_state,
                                             &active_state_out);
	    active_state = active_state_out;

        }
    }

    return result;
}

static EGLSurface
_egl_create_pbuffer_from_client_buffer (EGLDisplay dpy, EGLenum buftype,
                                        EGLClientBuffer buffer, 
                                        EGLConfig config, 
                                        const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;
    
    if (dispatch.eglCreatePbufferFromClientBuffer)
        surface = dispatch.eglCreatePbufferFromClientBuffer (dpy, buftype,
                                                             buffer, config,
                                                             attrib_list);

    return surface;
}

static EGLBoolean
_egl_surface_attrib (EGLDisplay dpy, EGLSurface surface, 
                     EGLint attribute, EGLint value)
{
    EGLBoolean result = EGL_FALSE;
    
    if (dispatch.eglSurfaceAttrib)
        result = dispatch.eglSurfaceAttrib (dpy, surface, attribute, value);

    return result;
}
    
static EGLBoolean
_egl_bind_tex_image (EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    EGLBoolean result = EGL_FALSE;
    
    if (dispatch.eglBindTexImage)
        result = dispatch.eglBindTexImage (dpy, surface, buffer);

    return result;
}

static EGLBoolean
_egl_release_tex_image (EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    EGLBoolean result = EGL_FALSE;
    
    if (dispatch.eglReleaseTexImage)
        result = dispatch.eglReleaseTexImage (dpy, surface, buffer);

    return result;
}

static EGLBoolean
_egl_swap_interval (EGLDisplay dpy, EGLint interval)
{
    EGLBoolean result = EGL_FALSE;
    
    if (dispatch.eglSwapInterval)
        result = dispatch.eglSwapInterval (dpy, interval);

    return result;
}

static EGLContext
_egl_create_contextegl (EGLDisplay dpy, EGLConfig config,
                        EGLContext share_context,
                        const EGLint *attrib_list)
{
    EGLContext result = EGL_NO_CONTEXT;
    
    if (dispatch.eglCreateContext)
        result = dispatch.eglCreateContext (dpy, config, share_context, 
                                            attrib_list);

    return result;
}

static EGLBoolean
_egl_destroy_context (EGLDisplay dpy, EGLContext ctx)
{
    EGLBoolean result = GL_FALSE;
    egl_state_t *state;

    if (dispatch.eglDestroyContext) {
        result = dispatch.eglDestroyContext (dpy, ctx); 

        if (result == GL_TRUE) {
            _gpuprocess_server_destroy_context (dpy, ctx, active_state);
        }
    }

    return result;
}

/*
static EGLContext
_egl_get_current_context (void)
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
*/

static EGLBoolean
_egl_query_context (EGLDisplay dpy, EGLContext ctx,
                    EGLint attribute, EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglQueryContext)
        return result;
    
    result = dispatch.eglQueryContext (dpy, ctx, attribute, value);

    return result;
}

static EGLBoolean
_egl_wait_gl (void)
{
    EGLBoolean result = EGL_FALSE;
    
    if (! dispatch.eglWaitGL)
        return result;

    result = dispatch.eglWaitGL ();

    return result;
}

static EGLBoolean
_egl_wait_native (EGLint engine)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglWaitNative)
        return result;
    
    result = dispatch.eglWaitNative (engine);

    return result;
}

static EGLBoolean
_egl_swap_buffers (EGLDisplay dpy, EGLSurface surface)
{
    EGLBoolean result = EGL_BAD_DISPLAY;

    result = dispatch.eglSwapBuffers (dpy, surface);

    return result;
}

static EGLBoolean
_egl_copy_buffers (EGLDisplay dpy, EGLSurface surface, 
                   EGLNativePixmapType target)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglCopyBuffers)
        return result;

    result = dispatch.eglCopyBuffers (dpy, surface, target);

    return result;
}

static __eglMustCastToProperFunctionPointerType
_egl_get_proc_address (const char *procname)
{
    return dispatch.GetProcAddress (procname);
}

static EGLBoolean 
_egl_make_current (EGLDisplay dpy, EGLSurface draw, EGLSurface read,
                   EGLContext ctx, v_link_list_t *active_state, 
                   v_link_list_t **active_state_out)
{
    EGLBoolean result = EGL_FALSE;
    v_link_list_t *exist = NULL;
    v_bool_t found = FALSE;

    if (! dispatch.eglMakeCurrent)
        return result;

    /* look for existing */
    found = _gpuprocess_match (dpy, draw, read, ctx, &exist);
    if (found == TRUE) {
        /* set active to exist, tell client about it */
        active_state = exist;

        /* call real makeCurrent */
        return dispatch.eglMakeCurrent (dpy, draw, read, ctx);
    }

    /* We could not find in the saved state, we don't know whether
     * parameters are valid or not 
     */
    result = dispatch.eglMakeCurrent (dpy, draw, read, ctx);
    if (result == EGL_TRUE) {
        _gpuprocess_server_make_current (dpy, draw, read, ctx,
                                         active_state, 
                                         active_state_out);
        active_state = *active_state_out;
    }
    return result;
}

/* start of eglext.h */
#ifdef EGL_KHR_lock_surface
static EGLBoolean
_egl_lock_surface_khr (EGLDisplay display, EGLSurface surface,
                       const EGLint *attrib_list)
{
    EGLBoolean result = EGL_FALSE;
    
    if (! dispatch.eglLockSurfaceKHR)
        return result;

    result = dispatch.eglLockSurfaceKHR (display, surface, attrib_list);

    return result;
}

static EGLBoolean
_egl_unlock_surface_khr (EGLDisplay display, EGLSurface surface)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglUnlockSurfaceKHR)
        return result;
    
    result = dispatch.eglUnlockSurfaceKHR (display, surface);

    return result;
}
#endif

#ifdef EGL_KHR_image
static EGLImageKHR
_egl_create_image_khr (EGLDisplay dpy, EGLContext ctx, EGLenum target,
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

static EGLBoolean
_egl_destroy_image_khr (EGLDisplay dpy, EGLImageKHR image)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglDestroyImageKHR)
        return result;
    
    result = dispatch.eglDestroyImageKHR (dpy, image);

    return result;
}
#endif

#ifdef EGL_KHR_reusable_sync
static EGLSyncKHR
_egl_create_sync_khr (EGLDisplay dpy, EGLenum type, const EGLint *attrib_list)
{
    EGLSyncKHR result = EGL_NO_SYNC_KHR;

    if (! dispatch.eglCreateSyncKHR)
        return result;
    
    result = dispatch.eglCreateSyncKHR (dpy, type, attrib_list);

    return result;
}

static EGLBoolean
_egl_destroy_sync_khr (EGLDisplay dpy, EGLSyncKHR sync)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglDestroySyncKHR)
        return result;

    result = dispatch.eglDestroySyncKHR (dpy, sync);

    return result;
}

static EGLint
_egl_client_wait_sync_khr (EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, 
                           EGLTimeKHR timeout)
{
    EGLint result = EGL_FALSE;

    if (! dispatch.eglClientWaitSyncKHR)
        return result;
    
    result = dispatch.eglClientWaitSyncKHR (dpy, sync, flags, timeout);

    return result;
}

static EGLBoolean
_egl_signal_sync_khr (EGLDisplay dpy, EGLSyncKHR sync, EGLenum mode)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglSignalSyncKHR)
        return result;
    
    result = dispatch.eglSignalSyncKHR (dpy, sync, mode);

    return result;
}

static EGLBoolean
_egl_get_sync_attrib_khr (EGLDisplay dpy, EGLSyncKHR sync,
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
static EGLSyncNV 
_egl_create_fence_sync_nv (EGLDisplay dpy, EGLenum condition, 
                           const EGLint *attrib_list)
{
    EGLSyncNV result = EGL_NO_SYNC_NV;

    if (! dispatch.eglCreateFenceSyncNV)
        return result;
    
    result = dispatch.eglCreateFenceSyncNV (dpy, condition, attrib_list);

    return result;
}

static EGLBoolean 
_egl_destroy_sync_nv (EGLSyncNV sync) 
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglDestroySyncNV)
        return result;
    
    result = dispatch.eglDestroySyncNV (sync);

    return result;
}

static EGLBoolean
_egl_fence_nv (EGLSyncNV sync)
{
    EGLBoolean result = EGL_FALSE;
    
    if (! dispatch.eglFenceNV)
        return result;

    result = dispatch.eglFenceNV (sync);

    return result;
}

static EGLint
_egl_client_wait_sync_nv (EGLSyncNV sync, EGLint flags, EGLTimeNV timeout)
{
    /* XXX: is this supposed to be default value ? */
    EGLint result = EGL_TIMEOUT_EXPIRED_NV;

    if (! dispatch.eglClientWaitSyncNV)
        return result;

    result = dispatch.eglClientWaitSyncNV (sync, flags, timeout);

    return result;
}

static EGLBoolean
_egl_signal_sync_nv (EGLSyncNV sync, EGLenum mode)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglSignalSyncNV)
        return result;

    result = dispatch.eglSignalSyncNV (sync, mode);

    return result;
}

static EGLBoolean
_egl_get_sync_attrib_nv (EGLSyncNV sync, EGLint attribute, EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglGetSyncAttribNV)
        return result;

    result = dispatch.eglGetSyncAttribNV (sync, attribute, value);

    return result;
}
#endif

#ifdef EGL_HI_clientpixmap
static EGLSurface
_egl_create_pixmap_surface_hi (EGLDisplay dpy, EGLConfig config,
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
static EGLImageKHR
_egl_create_drm_image_mesa (EGLDisplay dpy, const EGLint *attrib_list)
{
    EGLImageKHR result = EGL_NO_IMAGE_KHR;

    if (! dispatch.eglCreateDRMImageMESA)
        return result;
    
    result = dispatch.eglCreateDRMImageMESA (dpy, attrib_list);

    return result;
}

static EGLBoolean
_egl_export_drm_image_mesa (EGLDisplay dpy, EGLImageKHR image,
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
static EGLBoolean 
_egl_post_subbuffer_nv (EGLDisplay dpy, EGLSurface surface, 
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
static void *
_egl_map_image_sec (EGLDisplay dpy, EGLImageKHR image)
{
    void *result = NULL;

    if (! dispatch.eglMapImageSEC)
        return result;
    
    result = dispatch.eglMapImageSEC (dpy, image);

    return result;
}

static EGLBoolean
_egl_unmap_image_sec (EGLDisplay dpy, EGLImageKHR image)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglUnmapImageSEC)
        return result;
    
    result = dispatch.eglUnmapImageSEC (dpy, image);

    return result;
}

static EGLBoolean
_egl_get_image_attrib_sec (EGLDisplay dpy, EGLImageKHR image, 
                           EGLint attribute, EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglGetImageAttribSEC)
        return result;
    
    result = dispatch.eglGetImageAttribSEC (dpy, image, attribute, value);

    return result;
}
#endif
