/* This file implements the server thread side of egl functions.
 * After the server thread reads the command buffer, if it is 
 * egl calls, it is routed to here.
 *
 * It keeps two global variables for all server threads:
 * (1) dispatch - a dispatch table to real egl and gl calls. It 
 * is initialized during eglGetDisplay() -> _egl_get_display()
 * (2) server_states - this is a double-linked structure contains
 * all active and inactive egl states.  When a client switches context,
 * the previous egl_state is set to be inactive and thus is subject to
 * be destroyed during _egl_terminate(), _egl_destroy_context() and
 * _egl_release_thread()  The inactive context's surface can also be 
 * destroyed by _egl_destroy_surface().
 * (3) active_state - this is the pointer to the current active state.
 */

#include "config.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdlib.h>

/* XXX: we should move it to the srv */
#include "dispatch_private.h"
#include "egl_states.h"
#include "egl_server_private.h"
#include "types_private.h"

/* server thread global variables, referenced from 
 * egl_server_helper.c
 */
extern dispatch_t dispatch;
extern gl_server_states_t server_states;

/* server thread local variable */
__thread link_list_t    *active_state
  __attribute__(( tls_model ("initial-exec"))) = NULL;

exposed_to_tests EGLint
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
exposed_to_tests EGLDisplay
_egl_get_display (EGLNativeDisplayType display_id)
{
    EGLDisplay display = EGL_NO_DISPLAY;
   
    /* XXX: we should initialize once for both dispatch and srv structure */
    _server_init ();

    display = dispatch.eglGetDisplay (display_id);

    return display;
}

exposed_to_tests EGLBoolean
_egl_initialize (EGLDisplay display,
                 EGLint *major,
                 EGLint *minor)
{
    EGLBoolean result = EGL_FALSE;

    if (dispatch.eglInitialize) 
        result = dispatch.eglInitialize (display, major, minor);

    if (result == EGL_TRUE)
        _server_initialize (display);

    return result;
}

exposed_to_tests EGLBoolean
_egl_terminate (EGLDisplay display)
{
    EGLBoolean result = EGL_FALSE;

    if (dispatch.eglTerminate) {
        result = dispatch.eglTerminate (display);

        if (result == EGL_TRUE) {
            /* XXX: remove srv structure */
            _server_terminate (display, active_state);
        }
    }

    return result;
}

static const char *
_egl_query_string (EGLDisplay display,
                   EGLint name)
{
    const char *result = NULL;

    if (dispatch.eglQueryString)
        result = dispatch.eglQueryString (display, name);

    return result;
}

exposed_to_tests EGLBoolean
_egl_get_configs (EGLDisplay display,
                  EGLConfig *configs, 
                  EGLint config_size,
                  EGLint *num_config)
{
    EGLBoolean result = EGL_FALSE;

    if (dispatch.eglGetConfigs)
        result = dispatch.eglGetConfigs (display, configs, config_size, num_config);

    return result;
}

exposed_to_tests EGLBoolean
_egl_choose_config (EGLDisplay display,
                    const EGLint *attrib_list,
                    EGLConfig *configs,
                    EGLint config_size,
                    EGLint *num_config)
{
    EGLBoolean result = EGL_FALSE;

    if (dispatch.eglChooseConfig)
        result = dispatch.eglChooseConfig (display, attrib_list, configs,
                                           config_size, num_config);

    return result;
}

exposed_to_tests EGLBoolean
_egl_get_config_attrib (EGLDisplay display,
                        EGLConfig config,
                        EGLint attribute,
                        EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (dispatch.eglGetConfigAttrib)
        result = dispatch.eglGetConfigAttrib (display, config, attribute, value);

    return result;
}

exposed_to_tests EGLSurface
_egl_create_window_surface (EGLDisplay display,
                            EGLConfig config,
                            EGLNativeWindowType win,
                            const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    if (dispatch.eglCreateWindowSurface)
        surface = dispatch.eglCreateWindowSurface (display, config, win,
                                                   attrib_list);

    return surface;
}

exposed_to_tests EGLSurface
_egl_create_pbuffer_surface (EGLDisplay display,
                             EGLConfig config,
                             const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    if (dispatch.eglCreatePbufferSurface)
        surface = dispatch.eglCreatePbufferSurface (display, config, attrib_list);

    return surface;
}

exposed_to_tests EGLSurface
_egl_create_pixmap_surface (EGLDisplay display,
                            EGLConfig config,
                            EGLNativePixmapType pixmap, 
                            const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    if (dispatch.eglCreatePixmapSurface)
        surface = dispatch.eglCreatePixmapSurface (display, config, pixmap,
                                                   attrib_list);

    return surface;
}

exposed_to_tests EGLBoolean
_egl_destroy_surface (EGLDisplay display,
                      EGLSurface surface)
{
    EGLBoolean result = EGL_FALSE;

    if (!active_state)
        return result;

    if (dispatch.eglDestroySurface) { 
        result = dispatch.eglDestroySurface (display, surface);

        if (result == EGL_TRUE) {
            /* update srv states */
            _server_destroy_surface (display, surface, active_state);
        }
    }

    return result;
}

exposed_to_tests EGLBoolean
_egl_query_surface (EGLDisplay display,
                    EGLSurface surface,
                    EGLint attribute,
                    EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (dispatch.eglQuerySurface) 
        result = dispatch.eglQuerySurface (display, surface, attribute, value);

    return result;
}

exposed_to_tests EGLBoolean
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

    if (dispatch.eglWaitClient)
        result = dispatch.eglWaitClient ();

    return result;
}

exposed_to_tests EGLBoolean
_egl_release_thread (void)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t *egl_state;
    link_list_t *active_state_out = NULL;

    if (dispatch.eglReleaseThread) {
        result = dispatch.eglReleaseThread ();

        if (result == EGL_TRUE) {
            if (! active_state)
                return result;
            
            egl_state = (egl_state_t *) active_state->data;

            _server_make_current (egl_state->display,
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
_egl_create_pbuffer_from_client_buffer (EGLDisplay display,
                                        EGLenum buftype,
                                        EGLClientBuffer buffer,
                                        EGLConfig config,
                                        const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;
    
    if (dispatch.eglCreatePbufferFromClientBuffer)
        surface = dispatch.eglCreatePbufferFromClientBuffer (display, buftype,
                                                             buffer, config,
                                                             attrib_list);

    return surface;
}

static EGLBoolean
_egl_surface_attrib (EGLDisplay display,
                     EGLSurface surface, 
                     EGLint attribute,
                     EGLint value)
{
    EGLBoolean result = EGL_FALSE;
    
    if (dispatch.eglSurfaceAttrib)
        result = dispatch.eglSurfaceAttrib (display, surface, attribute, value);

    return result;
}
    
static EGLBoolean
_egl_bind_tex_image (EGLDisplay display,
                     EGLSurface surface,
                     EGLint buffer)
{
    EGLBoolean result = EGL_FALSE;
    
    if (dispatch.eglBindTexImage)
        result = dispatch.eglBindTexImage (display, surface, buffer);

    return result;
}

static EGLBoolean
_egl_release_tex_image (EGLDisplay display,
                        EGLSurface surface,
                        EGLint buffer)
{
    EGLBoolean result = EGL_FALSE;
    
    if (dispatch.eglReleaseTexImage)
        result = dispatch.eglReleaseTexImage (display, surface, buffer);

    return result;
}

static EGLBoolean
_egl_swap_interval (EGLDisplay display,
                    EGLint interval)
{
    EGLBoolean result = EGL_FALSE;
    
    if (dispatch.eglSwapInterval)
        result = dispatch.eglSwapInterval (display, interval);

    return result;
}

exposed_to_tests EGLContext
_egl_create_context (EGLDisplay display,
                     EGLConfig config,
                     EGLContext share_context,
                     const EGLint *attrib_list)
{
    EGLContext result = EGL_NO_CONTEXT;
    
    if (dispatch.eglCreateContext)
        result = dispatch.eglCreateContext (display, config, share_context, 
                                            attrib_list);

    return result;
}

exposed_to_tests EGLBoolean
_egl_destroy_context (EGLDisplay dpy,
                      EGLContext ctx)
{
    EGLBoolean result = GL_FALSE;

    if (dispatch.eglDestroyContext) {
        result = dispatch.eglDestroyContext (dpy, ctx); 

        if (result == GL_TRUE) {
            _server_destroy_context (dpy, ctx, active_state);
        }
    }

    return result;
}

exposed_to_tests EGLContext
_egl_get_current_context (void)
{
    return dispatch.eglGetCurrentContext ();
    egl_state_t *state;

    if (!active_state)
        return EGL_NO_CONTEXT;

    state = (egl_state_t *) active_state->data;
    return state->context;
}

exposed_to_tests EGLDisplay
_egl_get_current_display (void)
{
    return dispatch.eglGetCurrentDisplay ();
    egl_state_t *state;

    if (!active_state)
        return EGL_NO_DISPLAY;

    state = (egl_state_t *) active_state->data;
    return state->display;
}

exposed_to_tests EGLSurface
_egl_get_current_surface (EGLint readdraw)
{
    return dispatch.eglGetCurrentSurface (readdraw);
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


exposed_to_tests EGLBoolean
_egl_query_context (EGLDisplay display,
                    EGLContext ctx,
                    EGLint attribute,
                    EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglQueryContext)
        return result;

    result = dispatch.eglQueryContext (display, ctx, attribute, value);

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
_egl_swap_buffers (EGLDisplay display,
                   EGLSurface surface)
{
    EGLBoolean result = EGL_BAD_DISPLAY;

    result = dispatch.eglSwapBuffers (display, surface);

    return result;
}

static EGLBoolean
_egl_copy_buffers (EGLDisplay display,
                   EGLSurface surface,
                   EGLNativePixmapType target)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglCopyBuffers)
        return result;

    result = dispatch.eglCopyBuffers (display, surface, target);

    return result;
}

static __eglMustCastToProperFunctionPointerType
_egl_get_proc_address (const char *procname)
{
    return dispatch.GetProcAddress (procname);
}

exposed_to_tests EGLBoolean 
_egl_make_current (EGLDisplay display,
                   EGLSurface draw,
                   EGLSurface read,
                   EGLContext ctx) 
{
    EGLBoolean result = EGL_FALSE;
    link_list_t *exist = NULL;
    link_list_t *active_state_out = NULL;
    bool found = false;

    if (! dispatch.eglMakeCurrent)
        return result;

    /* look for existing */
    found = _match (display, draw, read, ctx, &exist);
    if (found == true) {
        /* set active to exist, tell client about it */
        active_state = exist;

        /* call real makeCurrent */
        return dispatch.eglMakeCurrent (display, draw, read, ctx);
    }

    /* We could not find in the saved state, we don't know whether
     * parameters are valid or not 
     */
    result = dispatch.eglMakeCurrent (display, draw, read, ctx);
    if (result == EGL_TRUE) {
        _server_make_current (display, draw, read, ctx,
                                         active_state, 
                                         &active_state_out);
        active_state = active_state_out;
    }
    return result;
}

/* start of eglext.h */
#ifdef EGL_KHR_lock_surface
static EGLBoolean
_egl_lock_surface_khr (EGLDisplay display,
                       EGLSurface surface,
                       const EGLint *attrib_list)
{
    EGLBoolean result = EGL_FALSE;
    
    if (! dispatch.eglLockSurfaceKHR)
        return result;

    result = dispatch.eglLockSurfaceKHR (display, surface, attrib_list);

    return result;
}

static EGLBoolean
_egl_unlock_surface_khr (EGLDisplay display,
                         EGLSurface surface)
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
_egl_create_image_khr (EGLDisplay display,
                       EGLContext ctx,
                       EGLenum target,
                       EGLClientBuffer buffer,
                       const EGLint *attrib_list)
{
    EGLImageKHR result = EGL_NO_IMAGE_KHR;

    if (! dispatch.eglCreateImageKHR)
        return result;

    if (display == EGL_NO_DISPLAY)
        return result;
    
    result = dispatch.eglCreateImageKHR (display, ctx, target,
                                         buffer, attrib_list);

    return result;
}

static EGLBoolean
_egl_destroy_image_khr (EGLDisplay display,
                        EGLImageKHR image)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglDestroyImageKHR)
        return result;
    
    result = dispatch.eglDestroyImageKHR (display, image);

    return result;
}
#endif

#ifdef EGL_KHR_reusable_sync
static EGLSyncKHR
_egl_create_sync_khr (EGLDisplay display,
                      EGLenum type,
                      const EGLint *attrib_list)
{
    EGLSyncKHR result = EGL_NO_SYNC_KHR;

    if (! dispatch.eglCreateSyncKHR)
        return result;
    
    result = dispatch.eglCreateSyncKHR (display, type, attrib_list);

    return result;
}

static EGLBoolean
_egl_destroy_sync_khr (EGLDisplay display,
                       EGLSyncKHR sync)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglDestroySyncKHR)
        return result;

    result = dispatch.eglDestroySyncKHR (display, sync);

    return result;
}

static EGLint
_egl_client_wait_sync_khr (EGLDisplay display,
                           EGLSyncKHR sync,
                           EGLint flags,
                           EGLTimeKHR timeout)
{
    EGLint result = EGL_FALSE;

    if (! dispatch.eglClientWaitSyncKHR)
        return result;
    
    result = dispatch.eglClientWaitSyncKHR (display, sync, flags, timeout);

    return result;
}

static EGLBoolean
_egl_signal_sync_khr (EGLDisplay display,
                      EGLSyncKHR sync,
                      EGLenum mode)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglSignalSyncKHR)
        return result;
    
    result = dispatch.eglSignalSyncKHR (display, sync, mode);

    return result;
}

static EGLBoolean
_egl_get_sync_attrib_khr (EGLDisplay display,
                          EGLSyncKHR sync,
                          EGLint attribute,
                          EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglGetSyncAttribKHR)
        return result;
    
    result = dispatch.eglGetSyncAttribKHR (display, sync, attribute, value);

    return result;
}
#endif

#ifdef EGL_NV_sync
static EGLSyncNV 
_egl_create_fence_sync_nv (EGLDisplay display,
                           EGLenum condition,
                           const EGLint *attrib_list)
{
    EGLSyncNV result = EGL_NO_SYNC_NV;

    if (! dispatch.eglCreateFenceSyncNV)
        return result;
    
    result = dispatch.eglCreateFenceSyncNV (display, condition, attrib_list);

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
_egl_client_wait_sync_nv (EGLSyncNV sync,
                          EGLint flags,
                          EGLTimeNV timeout)
{
    /* XXX: is this supposed to be default value ? */
    EGLint result = EGL_TIMEOUT_EXPIRED_NV;

    if (! dispatch.eglClientWaitSyncNV)
        return result;

    result = dispatch.eglClientWaitSyncNV (sync, flags, timeout);

    return result;
}

static EGLBoolean
_egl_signal_sync_nv (EGLSyncNV sync,
                     EGLenum mode)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglSignalSyncNV)
        return result;

    result = dispatch.eglSignalSyncNV (sync, mode);

    return result;
}

static EGLBoolean
_egl_get_sync_attrib_nv (EGLSyncNV sync,
                         EGLint attribute,
                         EGLint *value)
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
_egl_create_pixmap_surface_hi (EGLDisplay display,
                               EGLConfig config,
                               struct EGLClientPixmapHI *pixmap)
{
    EGLSurface result = EGL_NO_SURFACE;

    if (! dispatch.eglCreatePixmapSurfaceHI)
        return result;
    
    result = dispatch.eglCreatePixmapSurfaceHI (display, config, pixmap);

    return result;
}
#endif

#ifdef EGL_MESA_drm_image
static EGLImageKHR
_egl_create_drm_image_mesa (EGLDisplay display,
                            const EGLint *attrib_list)
{
    EGLImageKHR result = EGL_NO_IMAGE_KHR;

    if (! dispatch.eglCreateDRMImageMESA)
        return result;
    
    result = dispatch.eglCreateDRMImageMESA (display, attrib_list);

    return result;
}

static EGLBoolean
_egl_export_drm_image_mesa (EGLDisplay display,
                            EGLImageKHR image,
                            EGLint *name,
                            EGLint *handle,
                            EGLint *stride)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglExportDRMImageMESA)
        return result;
    
    result = dispatch.eglExportDRMImageMESA (display, image, name, handle, stride);

    return result;
}
#endif

#ifdef EGL_NV_post_sub_buffer
static EGLBoolean 
_egl_post_subbuffer_nv (EGLDisplay display,
                        EGLSurface surface, 
                        EGLint x,
                        EGLint y,
                        EGLint width,
                        EGLint height)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglExportDRMImageMESA)
        return result;

    result = dispatch.eglPostSubBufferNV (display, surface, x, y, width, height);

    return result;
}
#endif

#ifdef EGL_SEC_image_map
static void *
_egl_map_image_sec (EGLDisplay display,
                    EGLImageKHR image)
{
    void *result = NULL;

    if (! dispatch.eglMapImageSEC)
        return result;
    
    result = dispatch.eglMapImageSEC (display, image);

    return result;
}

static EGLBoolean
_egl_unmap_image_sec (EGLDisplay display,
                      EGLImageKHR image)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglUnmapImageSEC)
        return result;
    
    result = dispatch.eglUnmapImageSEC (display, image);

    return result;
}

static EGLBoolean
_egl_get_image_attrib_sec (EGLDisplay display,
                           EGLImageKHR image,
                           EGLint attribute,
                           EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! dispatch.eglGetImageAttribSEC)
        return result;
    
    result = dispatch.eglGetImageAttribSEC (display, image, attribute, value);

    return result;
}
#endif
