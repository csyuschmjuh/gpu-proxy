#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdlib.h>

/* XXX: we should move it to the srv */
#include "gpuprocess_dispatch_private.h"
#include "gpuprocess_egl_states.h"
#include "gpuprocess_gles2_srv_private.h"
gpuprocess_dispatch_t	dispatch;

#include "gpuprocess_thread_private.h"
#include "gpuprocess_types_private.h"
#include "gpuprocess_cli_private.h"

/* XXX: initialize static mutex on srv */
gpu_mutex_static_init (mutex);

/* we should call this on srv thread */
static EGLBoolean 
_egl_make_pending_current () 
{
    EGLBoolean make_current_result = GL_FALSE;

    /* check pending make current call */
    if (srv_states.make_current_called && dispatch.eglMakeCurrent) {
	/* we need to make current call */
	make_current_result = 
	    dispatch.eglMakeCurrent (srv_states.pending_display,
				     srv_states.pending_drawable,
				     srv_states.pending_readable,
				     srv_states.pending_context);
	    srv_states.make_current_called = FALSE;
 
	/* we make current on pending eglMakeCurrent() and it succeeds */
	if (make_current_result == EGL_TRUE) {
	    _gpuprocess_srv_make_current (srv_states.pending_display,
					  srv_states.pending_drawable,
					  srv_states.pending_readable,
					  srv_states.pending_context);
	}
    }
   
    return make_current_result;
}

EGLAPI EGLint EGLAPIENTRY
eglGetError (void)
{
    EGLint error = EGL_NOT_INITIALIZED;

    gpu_mutex_lock (mutex);

    /* XXX: we should create a command buffer for it and wait for signal */
    _egl_make_pending_current ();
    if (dispatch.eglGetError)
	error = dispatch.eglGetError();

    gpu_mutex_unlock (mutex);
    return error;
}

/* This is the first call to egl system, we initialize dispatch here,
 * we also initialize gl client states
 */
EGLAPI EGLDisplay EGLAPIENTRY
eglGetDisplay (EGLNativeDisplayType display_id)
{
    EGLDisplay display;

    gpu_mutex_lock (mutex);

    /* XXX: we should initialize on srv */
    _egl_make_pending_current ();
    gpuprocess_dispatch_init (&dispatch);
    
    _gpu_process_cli_init ();

    /* XXX: this should be initialized in srv */
    _gpuprocess_srv_init ();

    /* XXX: we should create a command buffer for it and wait for signal */
    display = dispatch.eglGetDisplay (display_id);

    gpu_mutex_unlock (mutex);

    return display;
}
    
EGLAPI EGLBoolean EGLAPIENTRY
eglInitialize (EGLDisplay dpy, EGLint *major, EGLint *minor)
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);
 
    /* XXX: we should create a command buffer for it and wait for signal */
    _egl_make_pending_current (); 
    if (dispatch.eglInitialize) 
	result = dispatch.eglInitialize (dpy, major, minor);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY eglTerminate (EGLDisplay dpy)
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: we should create a command buffer for it and wait for signal */
    _egl_make_pending_current ();
    if (dispatch.eglTerminate) {
	/* XXX: remove srv structure */
	result = dispatch.eglTerminate (dpy);

	if (result == EGL_TRUE)
	    _gpuprocess_srv_terminate (dpy);
    }

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI const char * EGLAPIENTRY
eglQueryString (EGLDisplay dpy, EGLint name)
{
    const char *result = NULL;

    gpu_mutex_lock (mutex);

    /* XXX: we should create a command buffer for it and wait for signal */
    _egl_make_pending_current (); 
    if (dispatch.eglQueryString)
	result = dispatch.eglQueryString (dpy, name);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetConfigs (EGLDisplay dpy, EGLConfig *configs, 
	       EGLint config_size, EGLint *num_config)
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: we should create a command buffer for it and wait for signal */
    _egl_make_pending_current (); 
    if (dispatch.eglGetConfigs)
	result = dispatch.eglGetConfigs (dpy, configs, config_size, num_config);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglChooseConfig (EGLDisplay dpy, const EGLint *attrib_list,
		 EGLConfig *configs, EGLint config_size, 
		 EGLint *num_config)
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: we should create a command buffer for it and wait for signal */
    _egl_make_pending_current (); 
    if (dispatch.eglChooseConfig) 
	result = dispatch.eglChooseConfig (dpy, attrib_list, configs,
					   config_size, num_config);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetConfigAttrib (EGLDisplay dpy, EGLConfig config, EGLint attribute, 
		    EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: we should create a command buffer for it, and wait for signal */
    _egl_make_pending_current (); 
    if (dispatch.eglGetConfigAttrib)
	result = dispatch.eglGetConfigAttrib (dpy, config, attribute, value);

    gpu_mutex_unlock (mutex);
   
    return result;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreateWindowSurface (EGLDisplay dpy, EGLConfig config,
			EGLNativeWindowType win, 
			const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    gpu_mutex_lock (mutex);

    /* XXX: we should create a command buffer for it, and wait for signal */
    _egl_make_pending_current (); 
    if (dispatch.eglCreateWindowSurface)
	surface = dispatch.eglCreateWindowSurface (dpy, config, win,
						   attrib_list);

    gpu_mutex_unlock (mutex);

    return surface;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreatePbufferSurface (EGLDisplay dpy, EGLConfig config,
			 const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    gpu_mutex_lock (mutex);

    /* XXX: we should create a command buffer for it, and wait for signal */
    _egl_make_pending_current ();
    if (dispatch.eglCreatePbufferSurface)
	surface = dispatch.eglCreatePbufferSurface (dpy, config, attrib_list);

    gpu_mutex_unlock (mutex);

    return surface;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreatePixmapSurface (EGLDisplay dpy, EGLConfig config,
			EGLNativePixmapType pixmap, 
			const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;

    gpu_mutex_lock (mutex);

    /* XXX: we should create a command buffer for it, and wait for signal */
    _egl_make_pending_current ();
    if (dispatch.eglCreatePixmapSurface)
	surface = dispatch.eglCreatePixmapSurface (dpy, config, pixmap,
						   attrib_list);

    gpu_mutex_unlock (mutex);

    return surface;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglDestroySurface (EGLDisplay dpy, EGLSurface surface)
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: we should create a command buffer for it, and wait for signal */
    _egl_make_pending_current ();
    if (dispatch.eglDestroySurface) { 
	result = dispatch.eglDestroySurface (dpy, surface);
	
	if (result == EGL_TRUE && 
	    (srv_states.active_state->readable == surface ||
	     srv_states.active_state->drawable == surface))
	    srv_states.active_state = NULL;
    }

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglQuerySurface (EGLDisplay dpy, EGLSurface surface,
		 EGLint attribute, EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: we should create a command buffer for it, and wait for signal */
    _egl_make_pending_current ();
    if (dispatch.eglQuerySurface) 
	result = dispatch.eglQuerySurface (dpy, surface, attribute, value);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglBindAPI (EGLenum api)
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: we should create a command buffer for it, and wait for signal */
    _egl_make_pending_current ();
    if (dispatch.eglBindAPI) 
	result = dispatch.eglBindAPI (api);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLenum EGLAPIENTRY 
eglQueryAPI (void)
{
    EGLenum result = EGL_NONE;

    gpu_mutex_lock (mutex);

    /* XXX: we should create a command buffer for it, and wait for signal */
    _egl_make_pending_current ();
    if (dispatch.eglQueryAPI) 
	result = dispatch.eglQueryAPI ();

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglWaitClient (void)
{
    EGLBoolean result = EGL_FALSE;
    
    gpu_mutex_lock (mutex);

    /* XXX: we should create a command buffer for it, and wait for signal */
    _egl_make_pending_current ();
    if (dispatch.eglWaitClient)
	result = dispatch.eglWaitClient ();

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglReleaseThread (void)
{
    EGLBoolean result = EGL_FALSE;
    
    gpu_mutex_lock (mutex);

    /* XXX: we should create a command buffer for it, and wait for signal */
    _egl_make_pending_current ();
    if (dispatch.eglReleaseThread)
	result = dispatch.eglReleaseThread ();

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreatePbufferFromClientBuffer (EGLDisplay dpy, EGLenum buftype,
				  EGLClientBuffer buffer, 
				  EGLConfig config, const EGLint *attrib_list)
{
    EGLSurface surface = EGL_NO_SURFACE;
    
    gpu_mutex_lock (mutex);

    /* XXX: we should create a command buffer for it, and wait for signal */
    _egl_make_pending_current ();
    if (dispatch.eglCreatePbufferFromClientBuffer)
	surface = dispatch.eglCreatePbufferFromClientBuffer (dpy, buftype,
							     buffer, config,
							     attrib_list);

    gpu_mutex_unlock (mutex);

    return surface;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglSurfaceAttrib (EGLDisplay dpy, EGLSurface surface, 
		  EGLint attribute, EGLint value)
{
    EGLBoolean result = EGL_FALSE;
    
    gpu_mutex_lock (mutex);

    /* XXX: we should create a command buffer for it, and wait for signal */
    _egl_make_pending_current ();
    if (dispatch.eglSurfaceAttrib)
	result = dispatch.eglSurfaceAttrib (dpy, surface, attribute, value);

    gpu_mutex_unlock (mutex);

    return result;
}
    
EGLAPI EGLBoolean EGLAPIENTRY
eglBindTexImage (EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    EGLBoolean result = EGL_FALSE;
    
    gpu_mutex_lock (mutex);

    /* XXX: we should create a command buffer for it, and wait for signal */
    _egl_make_pending_current ();
    if (dispatch.eglBindTexImage)
	result = dispatch.eglBindTexImage (dpy, surface, buffer);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglReleaseTexImage (EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    EGLBoolean result = EGL_FALSE;
    
    gpu_mutex_lock (mutex);

    /* XXX: we should create a command buffer for it, and wait for signal */
    _egl_make_pending_current ();
    if (dispatch.eglReleaseTexImage)
	result = dispatch.eglReleaseTexImage (dpy, surface, buffer);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglSwapInterval (EGLDisplay dpy, EGLint interval)
{
    EGLBoolean result = EGL_FALSE;
    
    gpu_mutex_lock (mutex);

    /* XXX: we should create a command buffer for it, and wait for signal */
    _egl_make_pending_current ();
    if (dispatch.eglSwapInterval)
	result = dispatch.eglSwapInterval (dpy, interval);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLContext EGLAPIENTRY
eglCreateContext (EGLDisplay dpy, EGLConfig config,
		  EGLContext share_context,
		  const EGLint *attrib_list)
{
    EGLContext result = EGL_NO_CONTEXT;
    
    gpu_mutex_lock (mutex);

    /* XXX: we should create a command buffer for it, and wait for signal */
    _egl_make_pending_current ();
    if (dispatch.eglCreateContext)
	result = dispatch.eglCreateContext (dpy, config, share_context, 
					    attrib_list);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglDestroyContext (EGLDisplay dpy, EGLContext ctx)
{
    EGLBoolean result = GL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: we should create a command buffer for it, and wait for signal */
    _egl_make_pending_current ();
    if (dispatch.eglDestroyContext) {
	result = dispatch.eglDestroyContext (dpy, ctx); 

	if (result == GL_TRUE) 
	    _gpuprocdess_srv_destroy_context (dpy, ctx);
    }

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLContext EGLAPIENTRY 
eglGetCurrentContext (void)
{
    EGLContext result = EGL_NO_CONTEXT;

    gpu_mutex_lock (mutex);

    if (! dispatch.eglGetCurrentContext)
	goto FINISH;
    
    _egl_make_pending_current ();
    result = srv_states.active_state->context;
	
FINISH:
    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLDisplay EGLAPIENTRY 
eglGetCurrentDisplay (void)
{
    EGLDisplay result = EGL_NO_DISPLAY;

    gpu_mutex_lock (mutex);

    if (! dispatch.eglGetCurrentDisplay)
	goto FINISH;

    _egl_make_pending_current ();
    result = srv_states.active_state->display;

FINISH:
    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLSurface EGLAPIENTRY 
eglGetCurrentSurface (EGLint readdraw)
{
    EGLSurface result = EGL_NO_SURFACE;

    gpu_mutex_lock (mutex);

    if (! dispatch.eglGetCurrentSurface) 
	goto FINISH;

    _egl_make_pending_current ();

    if (readdraw == EGL_DRAW)
	result = srv_states.active_state->drawable;
    else
	result = srv_states.active_state->readable;

FINISH:
    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglQueryContext (EGLDisplay dpy, EGLContext ctx,
		 EGLint attribute, EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglQueryContext)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglQueryContext (dpy, ctx, attribute, value);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglWaitGL (void)
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglWaitGL)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglWaitGL ();

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglWaitNative (EGLint engine)
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglWaitNative)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglWaitNative (engine);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY 
eglCopyBuffers (EGLDisplay dpy, EGLSurface surface, 
		EGLNativePixmapType target)
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglCopyBuffers)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglCopyBuffers (dpy, surface, target);

    gpu_mutex_unlock (mutex);

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

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (_gpuprocess_srv_is_equal (srv_states.active_state,
				  dpy, draw, read, ctx) &&
	! srv_states.make_current_called)
	result = EGL_TRUE;
	goto FINISH;

    srv_states.pending_display = dpy;
    srv_states.pending_context = ctx;
    srv_states.pending_drawable = draw;
    srv_states.pending_readable = read;
    srv_states.make_current_called = TRUE;

    /* used for multithreading */
    if (dpy == NULL && draw == NULL && read == NULL && ctx == NULL)
	result = EGL_TRUE;
	goto FINISH;

    result = _egl_make_pending_current ();

FINISH:
    gpu_mutex_unlock (mutex);

    return result;
}

/* start of eglext.h */
#ifdef EGL_KHR_lock_surface
EGLAPI EGLBoolean EGLAPIENTRY
eglLockSurfaceKHR (EGLDisplay display, EGLSurface surface, 
		   const EGLint *attrib_list)
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglLockSurfaceKHR)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglLockSurfaceKHR (display, surface, attrib_list);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglUnlockSurfaceKHR (EGLDisplay display, EGLSurface surface)
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglUnlockSurfaceKHR)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglUnlockSurfaceKHR (display, surface);

    gpu_mutex_unlock (mutex);

    return result;
}
#endif

#ifdef EGL_KHR_image
EGLAPI EGLImageKHR EGLAPIENTRY
eglCreateImageKHR (EGLDisplay dpy, EGLContext ctx, EGLenum target,
		   EGLClientBuffer buffer, const EGLint *attrib_list)
{
    EGLImageKHR result = EGL_NO_IMAGE_KHR;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglCreateImageKHR)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglCreateImageKHR (dpy, ctx, target,
					 buffer, attrib_list);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglDestroyImageKHR (EGLDisplay dpy, EGLImageKHR image)
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglDestroyImageKHR)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglDestroyImageKHR (dpy, image);

    gpu_mutex_unlock (mutex);

    return result;
}
#endif

#ifdef EGL_KHR_reusale_sync
EGLAPI EGLSyncKHR EGLAPIENTRY
eglCreateSyncKHR (EGLDisplay dpy, EGLenum type, const EGLint *attrib_list)
{
    EGLSyncKHR result = EGL_NO_SYNC_KHR;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglCreateSyncKHR)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglCreateSyncKHR (dpy, type, attrib_list);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglCreateSyncKHR (EGLDisplay dpy, EGLSyncKHR sync)
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglDestroySyncKHR)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglDestroySyncKHR (dpy, sync);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLint EGLAPIENTRY
eglClientWaitSyncKHR (EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, 
		      EGLTimeKHR timeout)
{
    EGLint result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglClientWaitSyncKHR)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglClientWaitSyncKHR (dpy, sync, flags, timeout);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglSignalSyncKHR (EGLDisplay dpy, EGLSyncKHR sync, EGLenum mode)
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglSignalSyncKHR)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglSignalSyncKHR (dpy, sync, mode);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetSyncAttribKHR (EGLDisplay dpy, EGLSyncKHR sync,
		     EGLint attribute, EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglGetSyncAttribKHR)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglGetSyncAttribKHR (dpy, sync, attribute, value);

    gpu_mutex_unlock (mutex);

    return result;
}
#endif

#ifdef EGL_NV_sync
EGLSyncNV 
eglCreateFenceSyncNV (EGLDisplay dpy, EGLenum condition, 
		      const EGLint *attrib_list)
{
    EGLSyncNV result = EGL_NO_SYNC_NV;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglCreateFenceSyncNV)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglCreateFenceSyncNV (dpy, condition, attrib_list);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLBoolean 
eglDestroySyncNV (EGLSyncNV sync) 
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglDestroySyncNV)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglDestroySyncNV (sync);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLBoolean 
eglFenceNV (EGLSyncNV sync) 
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglFenceNV)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglFenceNV (sync);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLint
eglClientWaitSyncNV (EGLSyncNV sync, EGLint flags, EGLTimeNV timeout)
{
    /* XXX: is this supposed to be default value ? */
    EGLint result = EGL_TIMEOUT_EXPIRED_NV;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglClientWaitSyncNV)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglClientWaitSyncNV (sync, flags, timeout);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLBoolean
eglSignalSyncNV (EGLSyncNV sync, EGLenum mode)
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglSignalSyncNV)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglSignalSyncNV (sync, mode);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLBoolean
eglGetSyncAttribNV (EGLSyncNV sync, EGLint attribute, EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglGetSyncAttribNV)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglGetSyncAttribNV (sync, attribute, value);

    gpu_mutex_unlock (mutex);

    return result;
}
#endif

#ifdef EGL_HI_clientpixmap
EGLAPI EGLSurface EGLAPIENTRY
eglCreatePixmapSurfaceHI (EGLDisplay dpy, EGLConfig config,
			  struct EGLClientPixmapHI *pixmap)
{
    EGLSurface result = EGL_NO_SURFACE;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglCreatePixmapSurfaceHI)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglCreatePixmapSurfaceHI (dpy, config, pixmap);

    gpu_mutex_unlock (mutex);

    return result;
}
#endif

#ifdef EGL_MESA_drm_image
EGLAPI EGLImageKHR EGLAPIENTRY
eglCreateDRMImageMESA (EGLDisplay dpy, const EGLint *attrib_list)
{
    EGLImageKHR result = EGL_NO_IMAGE_KHR;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglCreateDRMImageMESA)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglCreateDRMImageMESA (dpy, attrib_list);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglExportDRMImageMESA (EGLDisplay dpy, EGLImageKHR image,
		       EGLint *name, EGLint *handle, EGLint *stride)
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglExportDRMImageMESA)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglExportDRMImageMESA (dpy, image, name, handle, stride);

    gpu_mutex_unlock (mutex);

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

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglExportDRMImageMESA)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.egPostSubBufferNV (dpy, surface, x, y, width, height);

    gpu_mutex_unlock (mutex);

    return result;
}
#endif

#ifdef EGL_SEC_image_map
EGLAPI void * EGLAPIENTRY
eglMapImageSEC (EGLDisplay dpy, EGLImageKHR image)
{
    void *result = NULL;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglMapImageSEC)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglMapImageSEC (dpy, image);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY 
eglUnmapImageSEC (EGLDisplay dpy, EGLImageKHR image)
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglUnmapImageSEC)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglUnmapImageSEC (dpy, image);

    gpu_mutex_unlock (mutex);

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetImageAttribSEC (EGLDisplay dpy, EGLImageKHR image, EGLint attribute,
		      EGLint *value)
{
    EGLBoolean result = EGL_FALSE;

    gpu_mutex_lock (mutex);

    /* XXX: We should put a command buffer instead calling here */
    if (! dispatch.eglGetImageAttribSEC)
	return result;
    
    _egl_make_pending_current ();

    result = dispatch.eglGetImageAttribSEC (dpy, image, attribute, value);

    gpu_mutex_unlock (mutex);

    return result;
}
#endif
