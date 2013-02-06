#ifndef GPUPROCESS_SERVER_STATE_H
#define GPUPROCESS_SERVER_STATE_H

#include "hash.h"
#include "types_private.h"
#include <stdbool.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <X11/Xlib.h>

typedef struct _server_eglimage {
    EGLDisplay display;
    EGLImageKHR image;
    EGLContext context;
} server_eglimage_t;

typedef struct _server_context {
    EGLDisplay display;
    EGLContext context;
} server_context_t;

typedef struct _server_surface {
    EGLDisplay display;
    EGLSurface surface;
} server_surface_t;

typedef struct _server_display {
    Display *dpy;
    EGLDisplay egl_dpy;
} server_display_t;

private Display *
_server_get_display (EGLDisplay egl_display);

private void
_destroy_display (void *abstract_display);

private link_list_t **
_server_displays ();

private server_display_t *
_server_display_create (Display *dpy, EGLDisplay egl_dpy);

private void
_server_display_add (Display *dpy, EGLDisplay egl_dpy);

private void
_server_display_remove (Display *dpy, EGLDisplay egl_dpy);

private void
_server_destroy_display (EGLDisplay display);
    
private void
_destroy_eglimage (void *abstract_image);

private link_list_t **
_server_eglimages ();

private server_eglimage_t *
_server_eglimage_create (EGLDisplay display, EGLImageKHR image, EGLSurface surface);

private void
_server_eglimage_remove (EGLDisplay display, EGLImageKHR image);

private void
_server_eglimage_add (EGLDisplay display, EGLImageKHR image, EGLContext context);

private link_list_t **
_server_shared_contexts ();

private server_context_t *
_server_context_create (EGLDisplay display, EGLContext context);

private void
_server_shared_contexts_add (EGLDisplay, EGLSurface context);

private void
_server_shared_contexts_remove (EGLDisplay display, EGLContext context);

private void
_destroy_context (void *abstract_context);

private link_list_t **
_server_shared_surfaces ();

private server_surface_t *
_server_surface_create (EGLDisplay display, EGLSurface surface);

private void
_server_shared_surfaces_add (EGLDisplay display, EGLSurface surface);

private void
_server_shared_surfaces_remove (EGLDisplay display, EGLSurface surface);

private void
_destroy_surface (void *abstract_surface);

#endif

