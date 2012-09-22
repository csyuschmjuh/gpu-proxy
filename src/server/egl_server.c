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
#include "server.h"
#include "dispatch_private.h"
#include "egl_states.h"
#include "egl_server_private.h"
#include "types_private.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdlib.h>

/* server thread global variables, referenced from 
 * egl_server_helper.c
 */
extern gl_server_states_t server_states;

/* server thread local variable */
__thread link_list_t    *active_state
  __attribute__(( tls_model ("initial-exec"))) = NULL;

exposed_to_tests EGLint
_egl_get_error (server_t *server)
{
    EGLint error = EGL_NOT_INITIALIZED;

    if (! CACHING_SERVER(server)->super_dispatch.eglGetError)
        return error;

    error = CACHING_SERVER(server)->super_dispatch.eglGetError(server);

    return error;
}

exposed_to_tests EGLDisplay
_egl_get_display (server_t *server,
                  EGLNativeDisplayType display_id)
{
    EGLDisplay display = EGL_NO_DISPLAY;

    display = CACHING_SERVER(server)->super_dispatch.eglGetDisplay (server, display_id);

    return display;
}

exposed_to_tests EGLBoolean
_egl_initialize (server_t *server,
                 EGLDisplay display,
                 EGLint *major,
                 EGLint *minor)
{
    EGLBoolean result = EGL_FALSE;

    if (CACHING_SERVER(server)->super_dispatch.eglInitialize) 
        result = CACHING_SERVER(server)->super_dispatch.eglInitialize (server, display, major, minor);

    if (result == EGL_TRUE)
        _server_initialize (display);

    return result;
}

exposed_to_tests EGLBoolean
_egl_terminate (server_t *server,
                EGLDisplay display)
{
    EGLBoolean result = EGL_FALSE;

    if (CACHING_SERVER(server)->super_dispatch.eglTerminate) {
        result = CACHING_SERVER(server)->super_dispatch.eglTerminate (server, display);

        if (result == EGL_TRUE) {
            /* XXX: remove srv structure */
            _server_terminate (display, active_state);
        }
    }

    return result;
}

static const char *
_egl_query_string (server_t *server,
                   EGLDisplay display,
                   EGLint name)
{
    const char *result = NULL;

    if (CACHING_SERVER(server)->super_dispatch.eglQueryString)
        result = CACHING_SERVER(server)->super_dispatch.eglQueryString (server, display, name);

    return result;
}

exposed_to_tests EGLBoolean
_egl_get_configs (server_t *server,
                  EGLDisplay display,
                  EGLConfig *configs,
                  EGLint config_size,
                  EGLint *num_config)
{
    EGLBoolean result = EGL_FALSE;

    if (CACHING_SERVER(server)->super_dispatch.eglGetConfigs)
        result = CACHING_SERVER(server)->super_dispatch.eglGetConfigs (server, display, configs, config_size, num_config);

    return result;
}

exposed_to_tests EGLBoolean
_egl_choose_config (server_t *server,
                    EGLDisplay display,
                    const EGLint *attrib_list,
                    EGLConfig *configs,
                    EGLint config_size,
                    EGLint *num_config)
{
    EGLBoolean result = EGL_FALSE;

    if (CACHING_SERVER(server)->super_dispatch.eglChooseConfig)
        result = CACHING_SERVER(server)->super_dispatch.eglChooseConfig (server, display, attrib_list, configs,
                                           config_size, num_config);

    return result;
}

exposed_to_tests EGLBoolean
_egl_get_config_attrib (server_t *server,
                        EGLDisplay display,
                        EGLConfig config,
                        EGLint attribute,
                        EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (CACHING_SERVER(server)->super_dispatch.eglGetConfigAttrib)
        result = CACHING_SERVER(server)->super_dispatch.eglGetConfigAttrib (server, display, config, attribute, value);

    return result;
}

exposed_to_tests EGLSurface
_egl_create_window_surface (server_t *server,
                            EGLDisplay display,
                            EGLConfig config,
                            EGLNativeWindowType win,
                            const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    if (CACHING_SERVER(server)->super_dispatch.eglCreateWindowSurface)
        surface = CACHING_SERVER(server)->super_dispatch.eglCreateWindowSurface (server, display, config, win,
                                                   attrib_list);

    return surface;
}

exposed_to_tests EGLSurface
_egl_create_pbuffer_surface (server_t *server,
                             EGLDisplay display,
                             EGLConfig config,
                             const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    if (CACHING_SERVER(server)->super_dispatch.eglCreatePbufferSurface)
        surface = CACHING_SERVER(server)->super_dispatch.eglCreatePbufferSurface (server, display, config, attrib_list);

    return surface;
}

exposed_to_tests EGLSurface
_egl_create_pixmap_surface (server_t *server,
                            EGLDisplay display,
                            EGLConfig config,
                            EGLNativePixmapType pixmap, 
                            const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    if (CACHING_SERVER(server)->super_dispatch.eglCreatePixmapSurface)
        surface = CACHING_SERVER(server)->super_dispatch.eglCreatePixmapSurface (server, display, config, pixmap,
                                                   attrib_list);

    return surface;
}

exposed_to_tests EGLBoolean
_egl_destroy_surface (server_t *server,
                      EGLDisplay display,
                      EGLSurface surface)
{
    EGLBoolean result = EGL_FALSE;

    if (!active_state)
        return result;

    if (CACHING_SERVER(server)->super_dispatch.eglDestroySurface) { 
        result = CACHING_SERVER(server)->super_dispatch.eglDestroySurface (server, display, surface);

        if (result == EGL_TRUE) {
            /* update srv states */
            _server_destroy_surface (display, surface, active_state);
        }
    }

    return result;
}

exposed_to_tests EGLBoolean
_egl_query_surface (server_t *server,
                    EGLDisplay display,
                    EGLSurface surface,
                    EGLint attribute,
                    EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (CACHING_SERVER(server)->super_dispatch.eglQuerySurface) 
        result = CACHING_SERVER(server)->super_dispatch.eglQuerySurface (server, display, surface, attribute, value);

    return result;
}

exposed_to_tests EGLBoolean
_egl_bind_api (server_t *server,
               EGLenum api)
{
    EGLBoolean result = EGL_FALSE;

    if (CACHING_SERVER(server)->super_dispatch.eglBindAPI) 
        result = CACHING_SERVER(server)->super_dispatch.eglBindAPI (server, api);

    return result;
}

static EGLenum 
_egl_query_api (server_t *server)
{
    EGLenum result = EGL_NONE;

    if (CACHING_SERVER(server)->super_dispatch.eglQueryAPI) 
        result = CACHING_SERVER(server)->super_dispatch.eglQueryAPI (server);

    return result;
}

static EGLBoolean
_egl_wait_client (server_t *server)
{
    EGLBoolean result = EGL_FALSE;

    if (CACHING_SERVER(server)->super_dispatch.eglWaitClient)
        result = CACHING_SERVER(server)->super_dispatch.eglWaitClient (server);

    return result;
}

exposed_to_tests EGLBoolean
_egl_release_thread (server_t *server)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t *egl_state;
    link_list_t *active_state_out = NULL;

    if (CACHING_SERVER(server)->super_dispatch.eglReleaseThread) {
        result = CACHING_SERVER(server)->super_dispatch.eglReleaseThread (server);

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
_egl_create_pbuffer_from_client_buffer (server_t *server,
                                        EGLDisplay display,
                                        EGLenum buftype,
                                        EGLClientBuffer buffer,
                                        EGLConfig config,
                                        const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;
    
    if (CACHING_SERVER(server)->super_dispatch.eglCreatePbufferFromClientBuffer)
        surface = CACHING_SERVER(server)->super_dispatch.eglCreatePbufferFromClientBuffer (server, display, buftype,
                                                             buffer, config,
                                                             attrib_list);

    return surface;
}

static EGLBoolean
_egl_surface_attrib (server_t *server,
                     EGLDisplay display,
                     EGLSurface surface, 
                     EGLint attribute,
                     EGLint value)
{
    EGLBoolean result = EGL_FALSE;
    
    if (CACHING_SERVER(server)->super_dispatch.eglSurfaceAttrib)
        result = CACHING_SERVER(server)->super_dispatch.eglSurfaceAttrib (server, display, surface, attribute, value);

    return result;
}
    
static EGLBoolean
_egl_bind_tex_image (server_t *server,
                     EGLDisplay display,
                     EGLSurface surface,
                     EGLint buffer)
{
    EGLBoolean result = EGL_FALSE;
    
    if (CACHING_SERVER(server)->super_dispatch.eglBindTexImage)
        result = CACHING_SERVER(server)->super_dispatch.eglBindTexImage (server, display, surface, buffer);

    return result;
}

static EGLBoolean
_egl_release_tex_image (server_t *server,
                        EGLDisplay display,
                        EGLSurface surface,
                        EGLint buffer)
{
    EGLBoolean result = EGL_FALSE;
    
    if (CACHING_SERVER(server)->super_dispatch.eglReleaseTexImage)
        result = CACHING_SERVER(server)->super_dispatch.eglReleaseTexImage (server, display, surface, buffer);

    return result;
}

static EGLBoolean
_egl_swap_interval (server_t *server,
                    EGLDisplay display,
                    EGLint interval)
{
    EGLBoolean result = EGL_FALSE;
    
    if (CACHING_SERVER(server)->super_dispatch.eglSwapInterval)
        result = CACHING_SERVER(server)->super_dispatch.eglSwapInterval (server, display, interval);

    return result;
}

exposed_to_tests EGLContext
_egl_create_context (server_t *server,
                     EGLDisplay display,
                     EGLConfig config,
                     EGLContext share_context,
                     const EGLint *attrib_list)
{
    EGLContext result = EGL_NO_CONTEXT;
    
    if (CACHING_SERVER(server)->super_dispatch.eglCreateContext)
        result = CACHING_SERVER(server)->super_dispatch.eglCreateContext (server, display, config, share_context, 
                                            attrib_list);

    return result;
}

exposed_to_tests EGLBoolean
_egl_destroy_context (server_t *server,
                      EGLDisplay dpy,
                      EGLContext ctx)
{
    EGLBoolean result = GL_FALSE;

    if (CACHING_SERVER(server)->super_dispatch.eglDestroyContext) {
        result = CACHING_SERVER(server)->super_dispatch.eglDestroyContext (server, dpy, ctx); 

        if (result == GL_TRUE) {
            _server_destroy_context (dpy, ctx, active_state);
        }
    }

    return result;
}

exposed_to_tests EGLContext
_egl_get_current_context (server_t *server)
{
    return CACHING_SERVER(server)->super_dispatch.eglGetCurrentContext (server);
    egl_state_t *state;

    if (!active_state)
        return EGL_NO_CONTEXT;

    state = (egl_state_t *) active_state->data;
    return state->context;
}

exposed_to_tests EGLDisplay
_egl_get_current_display (server_t *server)
{
    return CACHING_SERVER(server)->super_dispatch.eglGetCurrentDisplay (server);
    egl_state_t *state;

    if (!active_state)
        return EGL_NO_DISPLAY;

    state = (egl_state_t *) active_state->data;
    return state->display;
}

exposed_to_tests EGLSurface
_egl_get_current_surface (server_t *server,
                          EGLint readdraw)
{
    return CACHING_SERVER(server)->super_dispatch.eglGetCurrentSurface (server, readdraw);
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
_egl_query_context (server_t *server,
                    EGLDisplay display,
                    EGLContext ctx,
                    EGLint attribute,
                    EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglQueryContext)
        return result;

    result = CACHING_SERVER(server)->super_dispatch.eglQueryContext (server, display, ctx, attribute, value);

    return result;
}

static EGLBoolean
_egl_wait_gl (server_t *server)
{
    EGLBoolean result = EGL_FALSE;
    
    if (! CACHING_SERVER(server)->super_dispatch.eglWaitGL)
        return result;

    result = CACHING_SERVER(server)->super_dispatch.eglWaitGL (server);

    return result;
}

static EGLBoolean
_egl_wait_native (server_t *server,
                  EGLint engine)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglWaitNative)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglWaitNative (server, engine);

    return result;
}

static EGLBoolean
_egl_swap_buffers (server_t *server,
                   EGLDisplay display,
                   EGLSurface surface)
{
    EGLBoolean result = EGL_BAD_DISPLAY;

    result = CACHING_SERVER(server)->super_dispatch.eglSwapBuffers (server, display, surface);

    return result;
}

static EGLBoolean
_egl_copy_buffers (server_t *server,
                   EGLDisplay display,
                   EGLSurface surface,
                   EGLNativePixmapType target)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglCopyBuffers)
        return result;

    result = CACHING_SERVER(server)->super_dispatch.eglCopyBuffers (server, display, surface, target);

    return result;
}

static __eglMustCastToProperFunctionPointerType
_egl_get_proc_address (server_t *server,
                       const char *procname)
{
    return CACHING_SERVER(server)->super_dispatch.eglGetProcAddress (server, procname);
}

exposed_to_tests EGLBoolean 
_egl_make_current (server_t *server,
                   EGLDisplay display,
                   EGLSurface draw,
                   EGLSurface read,
                   EGLContext ctx) 
{
    EGLBoolean result = EGL_FALSE;
    link_list_t *exist = NULL;
    link_list_t *active_state_out = NULL;
    bool found = false;

    if (! CACHING_SERVER(server)->super_dispatch.eglMakeCurrent)
        return result;

    /* look for existing */
    found = _match (display, draw, read, ctx, &exist);
    if (found == true) {
        /* set active to exist, tell client about it */
        active_state = exist;

        /* call real makeCurrent */
        return CACHING_SERVER(server)->super_dispatch.eglMakeCurrent (server, display, draw, read, ctx);
    }

    /* We could not find in the saved state, we don't know whether
     * parameters are valid or not 
     */
    result = CACHING_SERVER(server)->super_dispatch.eglMakeCurrent (server, display, draw, read, ctx);
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
_egl_lock_surface_khr (server_t *server,
                       EGLDisplay display,
                       EGLSurface surface,
                       const EGLint *attrib_list)
{
    EGLBoolean result = EGL_FALSE;
    
    if (! CACHING_SERVER(server)->super_dispatch.eglLockSurfaceKHR)
        return result;

    result = CACHING_SERVER(server)->super_dispatch.eglLockSurfaceKHR (server, display, surface, attrib_list);

    return result;
}

static EGLBoolean
_egl_unlock_surface_khr (server_t *server,
                         EGLDisplay display,
                         EGLSurface surface)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglUnlockSurfaceKHR)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglUnlockSurfaceKHR (server, display, surface);

    return result;
}
#endif

#ifdef EGL_KHR_image
static EGLImageKHR
_egl_create_image_khr (server_t *server,
                       EGLDisplay display,
                       EGLContext ctx,
                       EGLenum target,
                       EGLClientBuffer buffer,
                       const EGLint *attrib_list)
{
    EGLImageKHR result = EGL_NO_IMAGE_KHR;

    if (! CACHING_SERVER(server)->super_dispatch.eglCreateImageKHR)
        return result;

    if (display == EGL_NO_DISPLAY)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglCreateImageKHR (server, display, ctx, target,
                                         buffer, attrib_list);

    return result;
}

static EGLBoolean
_egl_destroy_image_khr (server_t *server,
                        EGLDisplay display,
                        EGLImageKHR image)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglDestroyImageKHR)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglDestroyImageKHR (server, display, image);

    return result;
}
#endif

#ifdef EGL_KHR_reusable_sync
static EGLSyncKHR
_egl_create_sync_khr (server_t *server,
                      EGLDisplay display,
                      EGLenum type,
                      const EGLint *attrib_list)
{
    EGLSyncKHR result = EGL_NO_SYNC_KHR;

    if (! CACHING_SERVER(server)->super_dispatch.eglCreateSyncKHR)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglCreateSyncKHR (server, display, type, attrib_list);

    return result;
}

static EGLBoolean
_egl_destroy_sync_khr (server_t *server,
                       EGLDisplay display,
                       EGLSyncKHR sync)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglDestroySyncKHR)
        return result;

    result = CACHING_SERVER(server)->super_dispatch.eglDestroySyncKHR (server, display, sync);

    return result;
}

static EGLint
_egl_client_wait_sync_khr (server_t *server,
                           EGLDisplay display,
                           EGLSyncKHR sync,
                           EGLint flags,
                           EGLTimeKHR timeout)
{
    EGLint result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglClientWaitSyncKHR)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglClientWaitSyncKHR (server, display, sync, flags, timeout);

    return result;
}

static EGLBoolean
_egl_signal_sync_khr (server_t *server,
                      EGLDisplay display,
                      EGLSyncKHR sync,
                      EGLenum mode)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglSignalSyncKHR)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglSignalSyncKHR (server, display, sync, mode);

    return result;
}

static EGLBoolean
_egl_get_sync_attrib_khr (server_t *server,
                          EGLDisplay display,
                          EGLSyncKHR sync,
                          EGLint attribute,
                          EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglGetSyncAttribKHR)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglGetSyncAttribKHR (server, display, sync, attribute, value);

    return result;
}
#endif

#ifdef EGL_NV_sync
static EGLSyncNV 
_egl_create_fence_sync_nv (server_t *server,
                           EGLDisplay display,
                           EGLenum condition,
                           const EGLint *attrib_list)
{
    EGLSyncNV result = EGL_NO_SYNC_NV;

    if (! CACHING_SERVER(server)->super_dispatch.eglCreateFenceSyncNV)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglCreateFenceSyncNV (server, display, condition, attrib_list);

    return result;
}

static EGLBoolean 
_egl_destroy_sync_nv (server_t *server,
                      EGLSyncNV sync)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglDestroySyncNV)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglDestroySyncNV (server, sync);

    return result;
}

static EGLBoolean
_egl_fence_nv (server_t *server,
               EGLSyncNV sync)
{
    EGLBoolean result = EGL_FALSE;
    
    if (! CACHING_SERVER(server)->super_dispatch.eglFenceNV)
        return result;

    result = CACHING_SERVER(server)->super_dispatch.eglFenceNV (server, sync);

    return result;
}

static EGLint
_egl_client_wait_sync_nv (server_t *server,
                          EGLSyncNV sync,
                          EGLint flags,
                          EGLTimeNV timeout)
{
    /* XXX: is this supposed to be default value ? */
    EGLint result = EGL_TIMEOUT_EXPIRED_NV;

    if (! CACHING_SERVER(server)->super_dispatch.eglClientWaitSyncNV)
        return result;

    result = CACHING_SERVER(server)->super_dispatch.eglClientWaitSyncNV (server, sync, flags, timeout);

    return result;
}

static EGLBoolean
_egl_signal_sync_nv (server_t *server,
                     EGLSyncNV sync,
                     EGLenum mode)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglSignalSyncNV)
        return result;

    result = CACHING_SERVER(server)->super_dispatch.eglSignalSyncNV (server, sync, mode);

    return result;
}

static EGLBoolean
_egl_get_sync_attrib_nv (server_t *server,
                         EGLSyncNV sync,
                         EGLint attribute,
                         EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglGetSyncAttribNV)
        return result;

    result = CACHING_SERVER(server)->super_dispatch.eglGetSyncAttribNV (server, sync, attribute, value);

    return result;
}
#endif

#ifdef EGL_HI_clientpixmap
static EGLSurface
_egl_create_pixmap_surface_hi (server_t *server,
                               EGLDisplay display,
                               EGLConfig config,
                               struct EGLClientPixmapHI *pixmap)
{
    EGLSurface result = EGL_NO_SURFACE;

    if (! CACHING_SERVER(server)->super_dispatch.eglCreatePixmapSurfaceHI)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglCreatePixmapSurfaceHI (server, display, config, pixmap);

    return result;
}
#endif

#ifdef EGL_MESA_drm_image
static EGLImageKHR
_egl_create_drm_image_mesa (server_t *server,
                            EGLDisplay display,
                            const EGLint *attrib_list)
{
    EGLImageKHR result = EGL_NO_IMAGE_KHR;

    if (! CACHING_SERVER(server)->super_dispatch.eglCreateDRMImageMESA)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglCreateDRMImageMESA (server, display, attrib_list);

    return result;
}

static EGLBoolean
_egl_export_drm_image_mesa (server_t *server,
                            EGLDisplay display,
                            EGLImageKHR image,
                            EGLint *name,
                            EGLint *handle,
                            EGLint *stride)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglExportDRMImageMESA)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglExportDRMImageMESA (server, display, image, name, handle, stride);

    return result;
}
#endif

#ifdef EGL_NV_post_sub_buffer
static EGLBoolean 
_egl_post_subbuffer_nv (server_t *server,
                        EGLDisplay display,
                        EGLSurface surface, 
                        EGLint x,
                        EGLint y,
                        EGLint width,
                        EGLint height)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglExportDRMImageMESA)
        return result;

    result = CACHING_SERVER(server)->super_dispatch.eglPostSubBufferNV (server, display, surface, x, y, width, height);

    return result;
}
#endif

#ifdef EGL_SEC_image_map
static void *
_egl_map_image_sec (server_t *server,
                    EGLDisplay display,
                    EGLImageKHR image)
{
    void *result = NULL;

    if (! CACHING_SERVER(server)->super_dispatch.eglMapImageSEC)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglMapImageSEC (server, display, image);

    return result;
}

static EGLBoolean
_egl_unmap_image_sec (server_t *server,
                      EGLDisplay display,
                      EGLImageKHR image)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglUnmapImageSEC)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglUnmapImageSEC (server, display, image);

    return result;
}

static EGLBoolean
_egl_get_image_attrib_sec (server_t *server,
                           EGLDisplay display,
                           EGLImageKHR image,
                           EGLint attribute,
                           EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! CACHING_SERVER(server)->super_dispatch.eglGetImageAttribSEC)
        return result;
    
    result = CACHING_SERVER(server)->super_dispatch.eglGetImageAttribSEC (server, display, image, attribute, value);

    return result;
}
#endif

exposed_to_tests void
caching_server_init (caching_server_t *server, buffer_t *buffer)
{
    server_init (&server->super, buffer, false);
    server->super_dispatch = server->super.dispatch;

    /* TODO: Override the methods in the super dispatch table. */
}

exposed_to_tests caching_server_t *
caching_server_new (buffer_t *buffer)
{
    caching_server_t *server = malloc (sizeof(caching_server_t));
    caching_server_init (server, buffer);
    return server;
}
