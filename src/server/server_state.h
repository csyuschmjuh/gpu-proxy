#ifndef GPUPROCESS_SERVER_STATE_H
#define GPUPROCESS_SERVER_STATE_H

#include "hash.h"
#include "types_private.h"
#include "thread_private.h"
#include <stdbool.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <X11/Xlib.h>

/* This data structure contains a bing to texture from an EGLImage
 * by calling glEGLImageTargetTexture2DOES or 
 * glEGLImageTargetRenderbufferStorageOES
 */
typedef enum _server_binding_type {
    TEXTURE_BINDING,
    RENDERBUFFER_BINDING
} server_binding_type_t;

typedef struct _server_binding {
    EGLDisplay            egl_display;
    EGLContext            egl_context;
    unsigned int          binding_buffer;    /* renderbuffer or texture id */
    unsigned int          ref_count;
    server_binding_type_t type;
} server_binding_t;

typedef enum _server_surface_type {
    WINDOW_SURFACE,
    PIXMAP_SURFACE,
    PBUFFER_SURFACE
} server_surface_type_t;

/* server data strcture for display.
 * server will open a new display to X server because
 * almost all calls, except few are async.  In order to
 * eliminate contention to Xlib calls, we need a separate
 * display from client
 */
typedef struct _server_display {
    Display            *client_display;
    Display            *server_display;
    EGLDisplay          egl_display;
    bool                mark_for_deletion;   /* called by eglTerminate
                                              * from out-of-order buffer
                                              */
    unsigned int        ref_count;           /* set 0 by eglGetDisplay
                                              * from out-of-order buffer,
                                              * increase 1 by out-of-order
                                              * eglMakeCurrent, decrease
                                              * by out-of-order eglTerminate
                                              */
} server_display_t;

typedef struct _server_context {
    EGLDisplay          egl_display;
    EGLContext          egl_context;
    bool                mark_for_deletion;    /* called by out-of-order
                                               * eglDestroySurface
                                               */
    unsigned int        ref_count;            /* initial 0 by
                                               * eglCreateContext, max
                                               * reaches 1 by eglMakecurrent
                                               */ 
} server_context_t;

/* a server side surface struct contains
 * the EGLDisplay used for creating
 * EGLSurface, and the pointer to the native
 * surface, either a winddow or a pixmap
 */
typedef struct _server_surface {
    EGLDisplay             egl_display;
    EGLSurface             egl_surface;
    server_surface_type_t  type;
    bool                   mark_for_deletion; /* marked by out-of-order
                                               * eglDestroySurface
                                               */
    unsigned int           ref_count;         /* initial 0, increase by
                                               * eglMakeCurrent, 
                                               * eglBindTexImage,
                                               * decrease by eglMakecurrent,
                                               * eglReleaseTexImage
                                               */
    link_list_t           *surface_bindings;  /* a list of server_binding_t
                                               * only for pbuffer
                                               */
    thread_t               lock_server;        /* pbuffer only, 
                                                * the server who holds
                                                * lock on it by calling
                                                * out-of-order 
                                                * eglBindTexImage
                                                */
    link_list_t *native_surface;               /* for pixmap and window
                                                * surfaces, points to
                                                * native surface in
                                                * the native surface cache
                                                */
} server_surface_t;

/* a server side native surface is either a window or a pixmap.
 * It contains two important fields.
 * 1. a reference count.  When a server creates an EGLSurface from the
 *    native surface, reference count increase by 1.  When destroy
 *    an EGLSurface, reference count decrease by 1.  If reference
 *    count reaches 0, the native surface is removed.
 * 2. a reference to a server thread id,  This is the data to indicate
 *    which server holds the lock on the native surface
 */
typedef struct _server_native_surface {
    void              *native_surface;   /* Holds XID for pixmap or window */
    unsigned int       ref_count;
    thread_t           lock_server;      /* the server thread that holds
                                          * the lock on the surface, by
                                          * out-of-order eglMakeCurrent,
                                          * glEGLTargetImageTexture2DOES,
                                          * glEGLTargetImageRenderbufferStorageOES
                                          * descrease by glDeleteTextures,
                                          * glDeleteRenderbuffers
                                          */ 
} server_native_surface_t;


/* in OpenGL ES 2.0, according to EGL_KHR_image_pixmap extension,
 * the only allowed binding is EGL_NATIVE_PIXMAP_KHR and the context
 * must be EGL_NO_CONTEXT in eglCreateImageKHR
 */
typedef struct _server_image {
    EGLDisplay         egl_display;
    EGLImageKHR        egl_image;
    bool               mark_for_deletion; /* called by out-of-order
                                           * eglDestroyImageKHR
                                           */
    unsigned int       ref_count;         /* initial 0, increase by 
                                           * out-of-order eglCreateImageKHR,
                                           * glEGLTargetImageTexture2DOES,
                                           * glEGLTargetImageRenderbufferStorageOES,
                                           * decrease by in-order 
                                           * glDeleteTextures and
                                           * glDeleteRenderbuffers
                                           */
    link_list_t       *native_surface;    /* must be a pixmap */
    link_list_t       *image_bindings;    /* a list of server_binding_t */
    thread_t           lock_server;
} server_image_t;

/* get the cached server_display_t list */
private link_list_t **
_server_displays ();

/* obtain server sidde of X server display connection */
private Display *
_server_get_display (EGLDisplay egl_display);

/* destroy the server_display_t structure, called by
 * link_list_delete_element
 */
private void
_destroy_display (void *abstract_display);

/* create a new server_display_t */
private server_display_t *
_server_display_create (Display *server_display, Display *client_display,
                        EGLDisplay egl_display);

/* add newly created EGLDisplay to cache, called in
 * eglGetDisplay */
private void
_server_display_add (Display *server_display,
                     Display *client_display,
                     EGLDisplay egl_display);

/* remove a server_display_t from cache, called by eglTerminate */
private void
_server_display_remove (Display *server_display, EGLDisplay egl_display);

/* remove all display related resources marked for deletion,
   returns true if there are still display related resources unfreed */
private bool
_server_destroy_display_mark_for_deletion (EGLDisplay egl_display);

/* mark display to be deleted, called by out-of-order eglTerminate */
private void
_server_display_mark_for_deletion (EGLDisplay egl_display);

/* remove all display related resources regardless whether they are locked
 * or mark_for_deletion or not.  This happens app exits without completely
 * destroy resources
 */
private void
_server_destroy_display (Display *client_display);

/* delete function called by link_list_delete_element from
 * _destroy_image
 */
private void
_destroy_binding (void *abstract_binding);

/* delete function called by link_list_delete_element */
private void
_destroy_image (void *abstract_image);

/* get cached images */
private link_list_t **
_server_images ();

/* create an server_image_t, called by eglCreateImageKHR */
private server_image_t *
_server_image_create (EGLDisplay egl_display, EGLImageKHR egl_image,
                      EGLClientBuffer buffer);

/* convenience function to add eglimage, called by eglCreateImageKHR */
private void
_server_image_add (EGLDisplay egl_display, EGLImageKHR egl_image,
                   EGLClientBuffer buffer);

/* remove an eglimage, called by in-order eglDestroyImageKHR */
private void
_server_image_remove (EGLDisplay egl_display, EGLImageKHR egl_image);

/* mark deletion for EGLImage, called by out-of-order eglDestroyImageKHR */
private void
_server_image_mark_for_deletion (EGLDisplay egl_display,
                                 EGLImageKHR egl_image);

/* lock EGLImage to a particular server, such that no one can delete it.
 * The lock is only successful if eglimage has not been marked for
 * deletion.  This is called by out-of-order glEGLImageTargetTexture2DOES
 * or glEGLImageTargetRenderbufferStorageOES
 */
private void
_server_image_lock (EGLImageKHR egl_image, thread_t server);

/* set image binding to a texture or a renderbuffer, called by
 * in-order glEGLImageTargetTexture2DOES or
 * glEGLImageTargetRenderbufferStorageOES
 */
private void
_server_image_bind_buffer (EGLDisplay egl_display,
                           EGLContext egl_context, 
                           EGLImageKHR egl_image,
                           server_binding_type_t type,
                           unsigned int buffer,
                           thread_t server);

/* delete texture or renderbuffer, causes image binding to release
 * resources, called by glDeleteTextures and glDeleteRenderbuffers
 */
private void
_server_image_unbind_buffer_and_unlock (EGLDisplay egl_display,
                                        EGLContext egl_context,
                                        EGLImageKHR egl_image,
                                        server_binding_type_t type,
                                        unsigned int buffer,
                                        thread_t server);
/* get cached native surfaces */
private link_list_t **
_server_native_surfaces ();

private void
_native_surface_destroy (void *abstract_native_surface);

private
server_native_surface_t *
_server_native_surface_create (void *native_surface);

/* add a native surface to cache.  called by out-of-order eglCreateSurface */
private void
_server_native_surface_add (EGLDisplay egl_display, void *native_surface);

/* mark deletion, called by out-of-order eglDestroySurface */
_server_native_surface_mark_for_deletion (EGLDisplay egl_display,
                                          void *native_surface);
    
private link_list_t **
_registered_lock_requests ();

#endif

