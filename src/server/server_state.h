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

/* This data structure contains a bing to texture from an EGLImage
 * by calling glEGLImageTargetTexture2DOES or 
 * glEGLImageTargetRenderbufferStorageOES
 */
typedef enum _server_binding_type {
    TEXTURE_BINDING,
    RENDERBUFFER_BINDING,
    NO_BINDING
} server_binding_type_t;

typedef enum _server_surface_type {
    WINDOW_SURFACE,
    PIXMAP_SURFACE,
    PBUFFER_SURFACE
} server_surface_type_t;

typedef struct _native_resource {
    void               *native;
    unsigned int        ref_count;            /* set 0 by eglCreateSurface,
                                               * increase 1 by out-of-order
                                               * eglMakeCurrent and 
                                               * eglCreateImageKHR, decrease
                                               * 1 by in-order
                                               * eglmakecurrent, and
                                               * eglDestroyImageKHR
                                               */
    link_list_t        *lock_servers;
    thread_t            locked_server;
} native_resource_t;

typedef struct _server_context  server_context_t;

struct _server_context {
    EGLContext          egl_context;
    bool                mark_for_deletion;    /* called by out-of-order
                                               * eglDestroySurface
                                               */
    server_context_t   *share_context;
    thread_t            locked_server;
    link_list_t        *lock_servers;
};

/* a server side surface struct contains
 * the EGLDisplay used for creating
 * EGLSurface, and the pointer to the native
 * surface, either a winddow or a pixmap
 */
typedef struct _server_surface {
    EGLSurface             egl_surface;
    server_surface_type_t  type;
    native_resource_t      *native;
    bool                   mark_for_deletion; /* marked by out-of-order
                                               * eglDestroySurface
                                               */
    unsigned int           binding_buffer;
    EGLContext             binding_context;
    link_list_t           *lock_servers;        /* pbuffer only, 
                                                * the server who holds
                                                * lock on it by calling
                                                * out-of-order 
                                                * eglBindTexImage
                                                */
    thread_t               locked_server;

    /* these two are maintenance parameters used in eglMakeCurrent */
    int                    lock_status;
    bool                   lock_visited;
} server_surface_t;

/* in OpenGL ES 2.0, according to EGL_KHR_image_pixmap extension,
 * the only allowed binding is EGL_NATIVE_PIXMAP_KHR and the context
 * must be EGL_NO_CONTEXT in eglCreateImageKHR
 */
typedef struct _server_image {
    EGLImageKHR           egl_image;
    bool                  mark_for_deletion; /* called by out-of-order
                                              * eglDestroyImageKHR
                                              */
    native_resource_t    *native;
    unsigned int          binding_buffer;
    EGLContext            binding_context;
    server_binding_type_t binding_type;
    link_list_t          *lock_servers;
    thread_t              locked_server;
} server_image_t;

typedef struct _server_context_group
{
    link_list_t          *contexts;
} server_context_group_t;

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
    unsigned int        ref_count;           /* set 0 by eglGetDisplay
                                              * from out-of-order buffer,
                                              * increase 1 by out-of-order
                                              * eglMakeCurrent, decrease
                                              * by out-of-order eglTerminate
                                              */
    link_list_t        *context_groups;
    link_list_t        *surfaces;
    link_list_t        *images; 
} server_display_list_t;

typedef struct _server_egl_state
{
    EGLDisplay             egl_display;
    EGLContext             egl_context;
    EGLSurface             egl_draw;
    EGLSurface             egl_read;

    server_display_list_t *server_display;
    server_context_t      *server_context;
    server_surface_t      *server_draw;
    server_surface_t      *server_read;

    unsigned int           texture_binding;
    unsigned int           renderbuffer_binding;

    link_list_t           *images;
    link_list_t           *texture_images;
} server_egl_state;

/*********************************************************
 * functions for server_display_list
 *********************************************************/
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
_server_display_remove (server_display_list_t *display);

/* mark display to be deleted, called by out-of-order eglTerminate */
private bool 
_server_display_mark_for_deletion (EGLDisplay egl_display);


/* get the pointer to the display */
private server_display_list_t *
_server_display_find (EGLDisplay egl_display);

/*********************************************************
 * functions for server_image
 *********************************************************/
/* create an server_image_t, called by out-of-order eglCreateImageKHR */
private server_image_t *
_server_image_create (EGLImageKHR egl_image, void *native);

/* convenience function to add eglimage, called by out-of-order 
 * eglCreateImageKHR */
private void
_server_image_add (server_display_list_t *display, EGLImageKHR egl_image,
                   void *native);

/* remove an eglimage, called by in-order eglDestroyImageKHR */
private void
_server_image_remove (server_display_list_t *display, EGLImageKHR egl_image);

/* mark deletion for EGLImage, called by out-of-order eglDestroyImageKHR */
private void
_server_image_mark_for_deletion (server_display_list_t *display,
                                 EGLImageKHR egl_image);

/* called by out-of-order glEGLImageTarggetTexture2DOES or
 * glEGLImageTargetRenderbufferStorageOES
 */
private void
_server_image_lock (server_display_list_t *display, EGLImageKHR egl_image,
                    thread_t server);

/* called by in-order glEGLImageTargetTexture2DOES or
 * glEGLImageTargetRenderbufferStorageOES,  if image->locked_server != 0
 * return SERVER_WAIT
 */
private server_response_type_t
_server_image_bind_buffer (server_display_list_t *display,
                           EGLImageKHR egl_image,
                           EGLContext egl_context, unsigned int buffer,
                           server_binding_type_t type,
                           thread_t server);


/* delete texture or renderbuffer, causes image binding to release
 * resources, called by glDeleteTextures and glDeleteRenderbuffers
 */
private void
_server_image_unbind_buffer (server_display_list_t *display,
                             EGLContext egl_context,
                             server_binding_type_t type,
                             unsigned int buffer,
                             thread_t server);

private server_image_t *
_server_image_find (EGLDisplay display, EGLImageKHR egl_image);

/*********************************************************
 * functions for server_surface
 *********************************************************/

/* create a server resource on EGLSurface, called by out-of-order
 * eglCreateXXXSurface, eglCreatePbufferFromClientBuffer (anyone using it?)  * and eglCreatePixmapSurfaceHI
 */
private server_surface_t *
_server_surface_create (server_display_list_t *display,
                        EGLSurface egl_surface,
                        server_surface_type_t type,
                        void *native);

/* create a server resource on EGLSurface, called by out-of-order
 * eglCreateXXXSurface, eglCreatePbufferFromClientBuffer (anyone using it?)  * and eglCreatePixmapSurfaceHI
 */
private void
_server_surface_add (server_display_list_t *display,
                     EGLSurface egl_surface,
                     server_surface_type_t type,
                     void *native);
    
/* remove an EGLSurface, called by in-order eglDestroySurface */
private void
_server_surface_remove (server_display_list_t *display, EGLSurface egl_surface);

/* mark deletion for EGLSurface, called by out-of-order eglDestroySurface */
private void
_server_surface_mark_for_deletion (server_display_list_t *display,
                                   EGLSurface egl_surface);

/* lock to a particular server, called by out-of-order eglBindTexImage
 */
private void
_server_surface_lock (server_display_list_t *display,
                      EGLSurface egl_surface,
                      thread_t server);

/* called by in-order glBindTexImage, if the surface locked_server != 0
 * return SERVER_WAIT
 */
private server_response_type_t
_server_surface_bind_buffer (server_display_list_t *dispaly,
                             EGLSurface egl_surface,
                             EGLContext egl_context,
                             unsigned int buffer,
                             thread_t server);

/* called by in-order eglReleaseImage */
private void
_server_surface_unbind_buffer (server_display_list_t *display,
                               EGLSurface egl_surface,
                               EGLContext egl_context);

private server_surface_t *
_server_surface_find (server_display_list_t *display, EGLSurface egl_surface);

/*********************************************************
 * functions for EGLContext
 *********************************************************/
/* create a server resource on EGLContext, called by out-of-order
 * eglCreateContext, set initial ref_count to 1
 */
private server_context_t *
_server_context_create (EGLContext egl_context,
                        server_context_t *share_context);

/* create a server resource on EGLContext, called by out-of-order
 * eglCreateContext set initial ref_count to 1
 */
private void
_server_context_add (server_display_list_t *display,
                     EGLContext egl_context,
                     EGLContext egl_share_context);


/* mark deletion for EGLContext, called by out-of-order eglDestroyContext */
private void
_server_context_mark_for_deletion (server_display_list_t *display,
                                   EGLSurface egl_context);

/* remove EGLContext, called by in-order eglDestroyContext */
private void
_server_context_remove (server_display_list_t *display,
                        EGLContext egl_context);


/*****************************************************************
 * eglMakeCurrent functions
 *****************************************************************/
/* add 0 to lock_servers for a surface if switch to none, called by
 * out-of-order eglMakeCurrent
 */
private void
_server_context_make_current (server_display_list_t *display,
                              EGLContext egl_context,
                              bool switch_out,
                              thread_t server);

private void
_server_image_make_current (server_display_list_t *display,
                            server_context_group_t *group,
                            server_image_t *image,
                            bool switch_out,
                            thread_t server);

private void
_server_bound_surface_make_current (server_display_list_t *display,
                                    server_context_group_t *group,
                                    server_surface_t *surface,
                                    bool switch_out,
                                    thread_t server);

/* called by out-of-order eglMakeCurrent, the function is to 
 * append current server to the lock_serves for context and surfaces
 */
private void
_server_out_of_order_make_current (EGLDisplay new_egl_display,
                                   EGLSurface new_egl_draw,
                                   EGLSurface new_egl_read,
                                   EGLContext new_egl_context,
                                   EGLDisplay cur_egl_display,
                                   EGLSurface cur_egl_draw,
                                   EGLSurface cur_egl_read,
                                   EGLContext cur_egl_context,
                                   thread_t server);

private server_response_type_t
_server_in_order_make_current_test (EGLDisplay new_egl_display,
                                    EGLSurface new_egl_draw,
                                    EGLSurface new_egl_read,
                                    EGLContext new_egl_context,
                                    EGLDisplay cur_egl_display,
                                    EGLSurface cur_egl_draw,
                                    EGLSurface cur_egl_read,
                                    EGLContext cur_egl_context,
                                    thread_t server);
private void
_server_in_order_make_current (EGLDisplay new_egl_display,
                               EGLSurface new_egl_draw,
                               EGLSurface new_egl_read,
                               EGLContext new_egl_context,
                               EGLDisplay cur_egl_display,
                               EGLSurface cur_egl_draw,
                               EGLSurface cur_egl_read,
                               EGLContext cur_egl_context,
                               thread_t server);
#endif
