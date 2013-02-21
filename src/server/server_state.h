#ifndef GPUPROCESS_SERVER_STATE_H
#define GPUPROCESS_SERVER_STATE_H

#include "hash.h"
#include "types_private.h"
#include "thread_private.h"
#include <stdbool.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <X11/Xlib.h>

typedef enum _server_response {
    SERVER_TRUE,
    SERVER_FALSE,
    SERVER_WAIT
} server_response_type_t;

/* server data strcture for display.
 * server will open a new display to X server because
 * almost all calls, except few are async.  In order to
 * eliminate contention to Xlib calls, we need a separate
 * display from client
 */
typedef struct _server_display_list {
    Display            *client_display;
    Display            *server_display;
    EGLDisplay          egl_display;
    bool                mark_for_deletion;   /* called by eglTerminate
                                              * from out-of-order buffer
                                              */
    unsigned int        ref_count;           /* set 1 by eglGetDisplay
                                              * from out-of-order buffer,
                                              * increase 1 by out-of-order
                                              * eglMakeCurrent, decrease
                                              * by out-of-order eglTerminate
                                              */
} server_display_list_t;

typedef struct _server_log {
    thread_t            server;
    double              timestamp;
} server_log_t;

/*********************************************************
 * functions for server_display_list
 *********************************************************/
private link_list_t **
_server_displays ();

/* obtain server side of X server display connection */
private Display *
_server_get_display (EGLDisplay egl_display);

/* create a new server_display_t */
private server_display_list_t *
_server_display_create (Display *server_display, Display *client_display,
                        EGLDisplay egl_display);

/* add newly created EGLDisplay to cache, called in out-of-order
 * eglGetDisplay */
private void 
_server_display_add (Display *server_display,
                     Display *client_display,
                     EGLDisplay egl_display);

/* remove a server_display_t from cache, called by in-order 
 * eglTerminate and by in-order eglMakeCurrent  */
private void
_server_display_remove (EGLDisplay egl_display);

private void 
_server_display_reference (EGLDisplay egl_display);

/* mark display to be deleted, called by out-of-order eglTerminate */
private void 
_server_display_mark_for_deletion (EGLDisplay egl_display);


/* get the pointer to the display */
private server_display_list_t *
_server_display_find (EGLDisplay egl_display);

/********************************************************
 * functions for call orders.  We need to make sure the
 * critical egl/gl calls are kept in order.  These calls
 * include 
 * eglMakeCurrent
 * eglGetDisplay
 * eglInitialize,
 * eglCreateWindowSurface
 * eglCreatePbufferSurface, 
 * eglCreatePixmapSurface
 * eglBindAPI
 * eglCreatePixmapSurfaceHI
 * eglCreateContext
 * eglCreatePbufferFromClientBuffer
 * eglCreateImageKHR
 * eglCreateDRMImageMESA
 * eglLockSurfaceKHR
 * eglCreateSyncKHR
 * eglTerminate
 * eglWaitNative
 * eglWaitGL
 * eglWaitClient
 * eglSwapBuffers
 * eglReleaseThread
 * glFinish
 * glFlush
 * any other egl/gl calls immediately after these calls
 ********************************************************/
private void
_call_order_list_append (thread_t server, double timestamp);

private void
_call_order_list_remove ();

private bool
_call_order_list_head_is_server (thread_t server, double timestamp);

private void
_server_append_call_log (thread_t server, double timestamp);

private bool
_server_allow_call (thread_t server, double timestamp);

private void
_server_remove_call_log ();

#endif
