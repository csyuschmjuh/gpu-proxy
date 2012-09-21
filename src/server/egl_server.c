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
#include "command_buffer_server.h"
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
extern command_buffer_server_t *command_buffer_server;

/* server thread local variable */
__thread link_list_t    *active_state
  __attribute__(( tls_model ("initial-exec"))) = NULL;

exposed_to_tests EGLint
_egl_get_error (void)
{
    EGLint error = EGL_NOT_INITIALIZED;

    if (! command_buffer_server->dispatch.eglGetError)
        return error;

    error = command_buffer_server->dispatch.eglGetError(command_buffer_server);

    return error;
}

/* This is the first call to egl system, we initialize dispatch here,
 * we also initialize gl client states
 */
exposed_to_tests EGLDisplay
_egl_get_display (EGLNativeDisplayType display_id)
{
    EGLDisplay display = EGL_NO_DISPLAY;
   
    command_buffer_server = malloc (sizeof (command_buffer_server_t));
    _server_init (command_buffer_server);

    display = command_buffer_server->dispatch.eglGetDisplay (command_buffer_server, display_id);

    return display;
}

exposed_to_tests EGLBoolean
_egl_initialize (EGLDisplay display,
                 EGLint *major,
                 EGLint *minor)
{
    EGLBoolean result = EGL_FALSE;

    if (command_buffer_server->dispatch.eglInitialize) 
        result = command_buffer_server->dispatch.eglInitialize (command_buffer_server, display, major, minor);

    if (result == EGL_TRUE)
        _server_initialize (display);

    return result;
}

exposed_to_tests EGLBoolean
_egl_terminate (EGLDisplay display)
{
    EGLBoolean result = EGL_FALSE;

    if (command_buffer_server->dispatch.eglTerminate) {
        result = command_buffer_server->dispatch.eglTerminate (command_buffer_server, display);

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

    if (command_buffer_server->dispatch.eglQueryString)
        result = command_buffer_server->dispatch.eglQueryString (command_buffer_server, display, name);

    return result;
}

exposed_to_tests EGLBoolean
_egl_get_configs (EGLDisplay display,
                  EGLConfig *configs, 
                  EGLint config_size,
                  EGLint *num_config)
{
    EGLBoolean result = EGL_FALSE;

    if (command_buffer_server->dispatch.eglGetConfigs)
        result = command_buffer_server->dispatch.eglGetConfigs (command_buffer_server, display, configs, config_size, num_config);

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

    if (command_buffer_server->dispatch.eglChooseConfig)
        result = command_buffer_server->dispatch.eglChooseConfig (command_buffer_server, display, attrib_list, configs,
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

    if (command_buffer_server->dispatch.eglGetConfigAttrib)
        result = command_buffer_server->dispatch.eglGetConfigAttrib (command_buffer_server, display, config, attribute, value);

    return result;
}

exposed_to_tests EGLSurface
_egl_create_window_surface (EGLDisplay display,
                            EGLConfig config,
                            EGLNativeWindowType win,
                            const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    if (command_buffer_server->dispatch.eglCreateWindowSurface)
        surface = command_buffer_server->dispatch.eglCreateWindowSurface (command_buffer_server, display, config, win,
                                                   attrib_list);

    return surface;
}

exposed_to_tests EGLSurface
_egl_create_pbuffer_surface (EGLDisplay display,
                             EGLConfig config,
                             const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    if (command_buffer_server->dispatch.eglCreatePbufferSurface)
        surface = command_buffer_server->dispatch.eglCreatePbufferSurface (command_buffer_server, display, config, attrib_list);

    return surface;
}

exposed_to_tests EGLSurface
_egl_create_pixmap_surface (EGLDisplay display,
                            EGLConfig config,
                            EGLNativePixmapType pixmap, 
                            const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    if (command_buffer_server->dispatch.eglCreatePixmapSurface)
        surface = command_buffer_server->dispatch.eglCreatePixmapSurface (command_buffer_server, display, config, pixmap,
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

    if (command_buffer_server->dispatch.eglDestroySurface) { 
        result = command_buffer_server->dispatch.eglDestroySurface (command_buffer_server, display, surface);

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

    if (command_buffer_server->dispatch.eglQuerySurface) 
        result = command_buffer_server->dispatch.eglQuerySurface (command_buffer_server, display, surface, attribute, value);

    return result;
}

exposed_to_tests EGLBoolean
_egl_bind_api (EGLenum api)
{
    EGLBoolean result = EGL_FALSE;

    if (command_buffer_server->dispatch.eglBindAPI) 
        result = command_buffer_server->dispatch.eglBindAPI (command_buffer_server, api);

    return result;
}

static EGLenum 
_egl_query_api (void)
{
    EGLenum result = EGL_NONE;

    if (command_buffer_server->dispatch.eglQueryAPI) 
        result = command_buffer_server->dispatch.eglQueryAPI (command_buffer_server);

    return result;
}

static EGLBoolean
_egl_wait_client (void)
{
    EGLBoolean result = EGL_FALSE;

    if (command_buffer_server->dispatch.eglWaitClient)
        result = command_buffer_server->dispatch.eglWaitClient (command_buffer_server);

    return result;
}

exposed_to_tests EGLBoolean
_egl_release_thread (void)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t *egl_state;
    link_list_t *active_state_out = NULL;

    if (command_buffer_server->dispatch.eglReleaseThread) {
        result = command_buffer_server->dispatch.eglReleaseThread (command_buffer_server);

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
    
    if (command_buffer_server->dispatch.eglCreatePbufferFromClientBuffer)
        surface = command_buffer_server->dispatch.eglCreatePbufferFromClientBuffer (command_buffer_server, display, buftype,
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
    
    if (command_buffer_server->dispatch.eglSurfaceAttrib)
        result = command_buffer_server->dispatch.eglSurfaceAttrib (command_buffer_server, display, surface, attribute, value);

    return result;
}
    
static EGLBoolean
_egl_bind_tex_image (EGLDisplay display,
                     EGLSurface surface,
                     EGLint buffer)
{
    EGLBoolean result = EGL_FALSE;
    
    if (command_buffer_server->dispatch.eglBindTexImage)
        result = command_buffer_server->dispatch.eglBindTexImage (command_buffer_server, display, surface, buffer);

    return result;
}

static EGLBoolean
_egl_release_tex_image (EGLDisplay display,
                        EGLSurface surface,
                        EGLint buffer)
{
    EGLBoolean result = EGL_FALSE;
    
    if (command_buffer_server->dispatch.eglReleaseTexImage)
        result = command_buffer_server->dispatch.eglReleaseTexImage (command_buffer_server, display, surface, buffer);

    return result;
}

static EGLBoolean
_egl_swap_interval (EGLDisplay display,
                    EGLint interval)
{
    EGLBoolean result = EGL_FALSE;
    
    if (command_buffer_server->dispatch.eglSwapInterval)
        result = command_buffer_server->dispatch.eglSwapInterval (command_buffer_server, display, interval);

    return result;
}

exposed_to_tests EGLContext
_egl_create_context (EGLDisplay display,
                     EGLConfig config,
                     EGLContext share_context,
                     const EGLint *attrib_list)
{
    EGLContext result = EGL_NO_CONTEXT;
    
    if (command_buffer_server->dispatch.eglCreateContext)
        result = command_buffer_server->dispatch.eglCreateContext (command_buffer_server, display, config, share_context, 
                                            attrib_list);

    return result;
}

exposed_to_tests EGLBoolean
_egl_destroy_context (EGLDisplay dpy,
                      EGLContext ctx)
{
    EGLBoolean result = GL_FALSE;

    if (command_buffer_server->dispatch.eglDestroyContext) {
        result = command_buffer_server->dispatch.eglDestroyContext (command_buffer_server, dpy, ctx); 

        if (result == GL_TRUE) {
            _server_destroy_context (dpy, ctx, active_state);
        }
    }

    return result;
}

exposed_to_tests EGLContext
_egl_get_current_context (void)
{
    return command_buffer_server->dispatch.eglGetCurrentContext (command_buffer_server);
    egl_state_t *state;

    if (!active_state)
        return EGL_NO_CONTEXT;

    state = (egl_state_t *) active_state->data;
    return state->context;
}

exposed_to_tests EGLDisplay
_egl_get_current_display (void)
{
    return command_buffer_server->dispatch.eglGetCurrentDisplay (command_buffer_server);
    egl_state_t *state;

    if (!active_state)
        return EGL_NO_DISPLAY;

    state = (egl_state_t *) active_state->data;
    return state->display;
}

exposed_to_tests EGLSurface
_egl_get_current_surface (EGLint readdraw)
{
    return command_buffer_server->dispatch.eglGetCurrentSurface (command_buffer_server, readdraw);
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

    if (! command_buffer_server->dispatch.eglQueryContext)
        return result;

    result = command_buffer_server->dispatch.eglQueryContext (command_buffer_server, display, ctx, attribute, value);

    return result;
}

static EGLBoolean
_egl_wait_gl (void)
{
    EGLBoolean result = EGL_FALSE;
    
    if (! command_buffer_server->dispatch.eglWaitGL)
        return result;

    result = command_buffer_server->dispatch.eglWaitGL (command_buffer_server);

    return result;
}

static EGLBoolean
_egl_wait_native (EGLint engine)
{
    EGLBoolean result = EGL_FALSE;

    if (! command_buffer_server->dispatch.eglWaitNative)
        return result;
    
    result = command_buffer_server->dispatch.eglWaitNative (command_buffer_server, engine);

    return result;
}

static EGLBoolean
_egl_swap_buffers (EGLDisplay display,
                   EGLSurface surface)
{
    EGLBoolean result = EGL_BAD_DISPLAY;

    result = command_buffer_server->dispatch.eglSwapBuffers (command_buffer_server, display, surface);

    return result;
}

static EGLBoolean
_egl_copy_buffers (EGLDisplay display,
                   EGLSurface surface,
                   EGLNativePixmapType target)
{
    EGLBoolean result = EGL_FALSE;

    if (! command_buffer_server->dispatch.eglCopyBuffers)
        return result;

    result = command_buffer_server->dispatch.eglCopyBuffers (command_buffer_server, display, surface, target);

    return result;
}

static __eglMustCastToProperFunctionPointerType
_egl_get_proc_address (const char *procname)
{
    return command_buffer_server->dispatch.eglGetProcAddress (command_buffer_server, procname);
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

    if (! command_buffer_server->dispatch.eglMakeCurrent)
        return result;

    /* look for existing */
    found = _match (display, draw, read, ctx, &exist);
    if (found == true) {
        /* set active to exist, tell client about it */
        active_state = exist;

        /* call real makeCurrent */
        return command_buffer_server->dispatch.eglMakeCurrent (command_buffer_server, display, draw, read, ctx);
    }

    /* We could not find in the saved state, we don't know whether
     * parameters are valid or not 
     */
    result = command_buffer_server->dispatch.eglMakeCurrent (command_buffer_server, display, draw, read, ctx);
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
    
    if (! command_buffer_server->dispatch.eglLockSurfaceKHR)
        return result;

    result = command_buffer_server->dispatch.eglLockSurfaceKHR (command_buffer_server, display, surface, attrib_list);

    return result;
}

static EGLBoolean
_egl_unlock_surface_khr (EGLDisplay display,
                         EGLSurface surface)
{
    EGLBoolean result = EGL_FALSE;

    if (! command_buffer_server->dispatch.eglUnlockSurfaceKHR)
        return result;
    
    result = command_buffer_server->dispatch.eglUnlockSurfaceKHR (command_buffer_server, display, surface);

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

    if (! command_buffer_server->dispatch.eglCreateImageKHR)
        return result;

    if (display == EGL_NO_DISPLAY)
        return result;
    
    result = command_buffer_server->dispatch.eglCreateImageKHR (command_buffer_server, display, ctx, target,
                                         buffer, attrib_list);

    return result;
}

static EGLBoolean
_egl_destroy_image_khr (EGLDisplay display,
                        EGLImageKHR image)
{
    EGLBoolean result = EGL_FALSE;

    if (! command_buffer_server->dispatch.eglDestroyImageKHR)
        return result;
    
    result = command_buffer_server->dispatch.eglDestroyImageKHR (command_buffer_server, display, image);

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

    if (! command_buffer_server->dispatch.eglCreateSyncKHR)
        return result;
    
    result = command_buffer_server->dispatch.eglCreateSyncKHR (command_buffer_server, display, type, attrib_list);

    return result;
}

static EGLBoolean
_egl_destroy_sync_khr (EGLDisplay display,
                       EGLSyncKHR sync)
{
    EGLBoolean result = EGL_FALSE;

    if (! command_buffer_server->dispatch.eglDestroySyncKHR)
        return result;

    result = command_buffer_server->dispatch.eglDestroySyncKHR (command_buffer_server, display, sync);

    return result;
}

static EGLint
_egl_client_wait_sync_khr (EGLDisplay display,
                           EGLSyncKHR sync,
                           EGLint flags,
                           EGLTimeKHR timeout)
{
    EGLint result = EGL_FALSE;

    if (! command_buffer_server->dispatch.eglClientWaitSyncKHR)
        return result;
    
    result = command_buffer_server->dispatch.eglClientWaitSyncKHR (command_buffer_server, display, sync, flags, timeout);

    return result;
}

static EGLBoolean
_egl_signal_sync_khr (EGLDisplay display,
                      EGLSyncKHR sync,
                      EGLenum mode)
{
    EGLBoolean result = EGL_FALSE;

    if (! command_buffer_server->dispatch.eglSignalSyncKHR)
        return result;
    
    result = command_buffer_server->dispatch.eglSignalSyncKHR (command_buffer_server, display, sync, mode);

    return result;
}

static EGLBoolean
_egl_get_sync_attrib_khr (EGLDisplay display,
                          EGLSyncKHR sync,
                          EGLint attribute,
                          EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! command_buffer_server->dispatch.eglGetSyncAttribKHR)
        return result;
    
    result = command_buffer_server->dispatch.eglGetSyncAttribKHR (command_buffer_server, display, sync, attribute, value);

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

    if (! command_buffer_server->dispatch.eglCreateFenceSyncNV)
        return result;
    
    result = command_buffer_server->dispatch.eglCreateFenceSyncNV (command_buffer_server, display, condition, attrib_list);

    return result;
}

static EGLBoolean 
_egl_destroy_sync_nv (EGLSyncNV sync) 
{
    EGLBoolean result = EGL_FALSE;

    if (! command_buffer_server->dispatch.eglDestroySyncNV)
        return result;
    
    result = command_buffer_server->dispatch.eglDestroySyncNV (command_buffer_server, sync);

    return result;
}

static EGLBoolean
_egl_fence_nv (EGLSyncNV sync)
{
    EGLBoolean result = EGL_FALSE;
    
    if (! command_buffer_server->dispatch.eglFenceNV)
        return result;

    result = command_buffer_server->dispatch.eglFenceNV (command_buffer_server, sync);

    return result;
}

static EGLint
_egl_client_wait_sync_nv (EGLSyncNV sync,
                          EGLint flags,
                          EGLTimeNV timeout)
{
    /* XXX: is this supposed to be default value ? */
    EGLint result = EGL_TIMEOUT_EXPIRED_NV;

    if (! command_buffer_server->dispatch.eglClientWaitSyncNV)
        return result;

    result = command_buffer_server->dispatch.eglClientWaitSyncNV (command_buffer_server, sync, flags, timeout);

    return result;
}

static EGLBoolean
_egl_signal_sync_nv (EGLSyncNV sync,
                     EGLenum mode)
{
    EGLBoolean result = EGL_FALSE;

    if (! command_buffer_server->dispatch.eglSignalSyncNV)
        return result;

    result = command_buffer_server->dispatch.eglSignalSyncNV (command_buffer_server, sync, mode);

    return result;
}

static EGLBoolean
_egl_get_sync_attrib_nv (EGLSyncNV sync,
                         EGLint attribute,
                         EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! command_buffer_server->dispatch.eglGetSyncAttribNV)
        return result;

    result = command_buffer_server->dispatch.eglGetSyncAttribNV (command_buffer_server, sync, attribute, value);

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

    if (! command_buffer_server->dispatch.eglCreatePixmapSurfaceHI)
        return result;
    
    result = command_buffer_server->dispatch.eglCreatePixmapSurfaceHI (command_buffer_server, display, config, pixmap);

    return result;
}
#endif

#ifdef EGL_MESA_drm_image
static EGLImageKHR
_egl_create_drm_image_mesa (EGLDisplay display,
                            const EGLint *attrib_list)
{
    EGLImageKHR result = EGL_NO_IMAGE_KHR;

    if (! command_buffer_server->dispatch.eglCreateDRMImageMESA)
        return result;
    
    result = command_buffer_server->dispatch.eglCreateDRMImageMESA (command_buffer_server, display, attrib_list);

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

    if (! command_buffer_server->dispatch.eglExportDRMImageMESA)
        return result;
    
    result = command_buffer_server->dispatch.eglExportDRMImageMESA (command_buffer_server, display, image, name, handle, stride);

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

    if (! command_buffer_server->dispatch.eglExportDRMImageMESA)
        return result;

    result = command_buffer_server->dispatch.eglPostSubBufferNV (command_buffer_server, display, surface, x, y, width, height);

    return result;
}
#endif

#ifdef EGL_SEC_image_map
static void *
_egl_map_image_sec (EGLDisplay display,
                    EGLImageKHR image)
{
    void *result = NULL;

    if (! command_buffer_server->dispatch.eglMapImageSEC)
        return result;
    
    result = command_buffer_server->dispatch.eglMapImageSEC (command_buffer_server, display, image);

    return result;
}

static EGLBoolean
_egl_unmap_image_sec (EGLDisplay display,
                      EGLImageKHR image)
{
    EGLBoolean result = EGL_FALSE;

    if (! command_buffer_server->dispatch.eglUnmapImageSEC)
        return result;
    
    result = command_buffer_server->dispatch.eglUnmapImageSEC (command_buffer_server, display, image);

    return result;
}

static EGLBoolean
_egl_get_image_attrib_sec (EGLDisplay display,
                           EGLImageKHR image,
                           EGLint attribute,
                           EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    if (! command_buffer_server->dispatch.eglGetImageAttribSEC)
        return result;
    
    result = command_buffer_server->dispatch.eglGetImageAttribSEC (command_buffer_server, display, image, attribute, value);

    return result;
}
#endif
