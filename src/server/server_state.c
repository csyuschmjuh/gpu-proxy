#include "server_state.h"
#include "thread_private.h"
#include <stdlib.h>
#include <stdio.h>

/*****************************************************************
 * static functions 
 *****************************************************************/
/* get cached server side server_display_t list.  It is static such
 * that all servers share it
 */
static link_list_t **
_server_displays ()
{
    static link_list_t *dpys = NULL;
    return &dpys;
}

static link_list_t **
_server_native_surfaces ()
{
    static link_list_t *native_surfaces = NULL;
    return &native_surfaces;
}

static link_list_t **
_server_surfaces (server_display_list_t *display)
{
    return &display->surfaces;
}

static link_list_t **
_server_images (server_display_list_t *display)
{
    return &display->images;
}

static link_list_t **
_server_contexts (server_display_list_t *display)
{
    return &display->contexts;
}

/* called by link_list_element from _destroy_image */
static void
_destroy_binding (void *abstract_binding)
{
    free (abstract_binding);
}

static void
_destroy_native_surface (void *abstract_surface)
{
    free (abstract_surface);
}

/* called by link_list_delete_element */
static void
_destroy_image (void *abstract_image)
{
    server_image_t *image = (server_image_t *)abstract_image;

    /* reduce ref_count on native surface */
    if (image->native_surface) {
        server_native_surface_t *native_surface = (server_native_surface_t *)image->native_surface->data;
        if (native_surface->ref_count > 0)
            native_surface->ref_count --;

        if (native_surface->ref_count == 0 &&
            native_surface->mark_for_deletion == true &&
            native_surface->lock_server == 0) {
            link_list_t **native_surfaces = _server_native_surfaces ();
            link_list_delete_element (native_surfaces, image->native_surface);
        }
    }

    /* remove image binding */
    link_list_clear (&image->image_bindings);

    free (image);
}

static void
_destroy_surface (void *abstract_surface)
{
    server_surface_t *surface = (server_surface_t *)abstract_surface;

    link_list_clear (&surface->surface_bindings);
    link_list_clear (&surface->native_surface);

    free (surface);
}


/* destroy server_display_list_t, called by link_list_delete_element */
static void
_destroy_display (void *abstract_display)
{
    server_display_list_t *dpy = (server_display_list_t *) abstract_display;

    XCloseDisplay (dpy->server_display);

    link_list_clear (&dpy->images);
    link_list_clear (&dpy->contexts);
    link_list_clear (&dpy->surfaces);

    free (dpy);
}

/*****************************************************************
 * display related functions 
 *****************************************************************/
/* obtain server side of X server display connection */
Display *
_server_get_display (EGLDisplay egl_display)
{
    link_list_t **dpys = _server_displays ();
    link_list_t *head = *dpys;

    while (head) {
        server_display_list_t *server_dpy = (server_display_list_t *)head->data;
        if (server_dpy->egl_display == egl_display)
            return server_dpy->server_display;

        head = head->next;
    }

    return NULL;
}

/* create a new server_display_t sture */
server_display_list_t *
_server_display_create (Display *server_display,
                        Display *client_display,
                        EGLDisplay egl_display)
{
    server_display_list_t *server_dpy = (server_display_list_t *) malloc (sizeof (server_display_list_t));
    server_dpy->server_display = server_display;
    server_dpy->egl_display = egl_display;
    server_dpy->client_display = client_display;
    server_dpy->ref_count = 1;
    server_dpy->mark_for_deletion = false;

    server_dpy->images = NULL;
    server_dpy->contexts = NULL;
    server_dpy->surfaces = NULL;
    return server_dpy;
}

/* add new EGLDisplay to cache, called in eglGetDisplay */
EGLBoolean
_server_display_add (Display *server_display, Display *client_display,
                     EGLDisplay egl_display)
{
    link_list_t **dpys = _server_displays ();
    server_display_list_t *server_dpy;
    link_list_t *head = *dpys;

    while (head) {
        server_dpy = (server_display_list_ *)head->data;

        if (server_dpy->server_display == server_display &&
            server_dpy->client_display == client_display &&
            server_dpy->egl_display == egl_display &&
            server_dpy->mark_for_deletion == false)
            return EGL_TRUE;
        }
        head = head->next;
    }

    server_dpy = _server_display_create (server_display, client_display, egl_display);
    link_list_append (dpys, (void *)server_dpy, _destroy_display);
    return EGL_TRUE;
}

/* called by out-of-order eglMakeCurrent */
EGLBoolean
_server_display_reference (EGLDisplay egl_display)
{
    link_list_t **dpys = _server_displays ();
    server_display_list_t *server_dpy;
    link_list_t *head = *dpys;

    while (head) {
        server_dpy = (server_display_list_ *)head->data;

        if (server_dpy->egl_display == egl_display) {
            /* if at this moment, ref_count > 1, it means there are
             * more than two threads are using the display
             */
            if (server_dpy->mark_for_deletion == true &&
                server_dpy->ref_count <= 1)
                return EGL_FALSE;

            server_dpy->ref_count++;
            return EGL_TRUE;
        }
        head = head->next;
    }
    return EGL_FALSE;
}

EGLBoolean
_server_display_unreference (EGLDisplay egl_display)
{
    link_list_t **dpys = _server_displays ();
    server_display_list_t *server_dpy;
    link_list_t *head = *dpys;

    while (head) {
        server_dpy = (server_display_list_ *)head->data;

        if (server_dpy->egl_display == egl_display) {
            if (server_dpy->ref_count > 1) {
                server_dpy->ref_count--;
                return EGL_TRUE;
        }
        head = head->next;
    }
    return EGL_FALSE;
}

/* remove a server_display from cache, called by in-order eglTerminate */
void
_server_display_remove (Display *server_display, EGLDisplay egl_display)
{
    link_list_t **dpys = _server_displays ();
    server_display_list_t *server_dpy;
    link_list_t *head = *dpys;

    while (head) {
        server_dpy = (server_display_list_t *) head->data;

        if (server_dpy->server_display == server_display &&
            server_dpy->egl_display == egl_display) {
            if (server_dpy->ref_count == 0 &&
                server_dpy->mark_for_deletion == true) {
                link_list_delete_element (dpys, head);
	        return;
            }
        }
        head = head->next;
    }
}

/* mark display for removal, called by out-of-order eglTerminate */
EGLBoolean
_server_destroy_mark_for_deletion (EGLDisplay egl_display)
{
    /* look into cached display */
    link_list_t **dpys = _server_displays ();
    link_list_t *head = *dpys;
    server_display_list *display = NULL;

    while (head) {
        server_display_list_t *dpy = (server_display_list_t *) head->data;
        if (dpy->egl_display == egl_display) {
            dpy_mark_for_deletion = true;
            display = dpy;
            break;
         }
         head = head->next;
    }

    if (! display)
        return EGL_FALSE;

    /* next is the contexts */
    link_list_t **ctxs = _server_contexts (display);
    head = *ctxs;
    
    while (head) {
        server_context_t *ctx = (server_context_t *) head->data;
        ctx->mark_for_deletion = true;

        head = head->next;
    }

    /* surface deletion */
    link_list_t **surfaces = _server_surfaces (display);
    head = surfaces;
 
    while (head) {
        server_surface_t *surface = (server_surface_t *) head->data;
        surface->mark_for_deletion = true;
        head = head->next;
    }
    
    /* eglimages */
    link_list_t **images = _server_images (display);
    head = images;
 
    while (head) {
        server_image_t *image = (server_image_t *) head->data;
        image->mark_for_deletion = true;
        head = head->next;
    }
}

server_display_list_t *
_server_display_find (EGLDisplay egl_display)
{
    link_list_t **dpys = _server_displays ();
    link_list_t *head = *dpy;

    while (head) {
        server_display_list_t *dpy = (server_display_list_t *) head->data;
        if (dpy->egl_display == egl_display)
            return dpy;

        head = head->next;
    }
    return NULL;
}

/*****************************************************************
 * image related functions 
 *****************************************************************/
/* create a server_image_t, out-of-order called by eglCreateImageKHR */
server_image_t *
_server_image_create (EGLDisplay egl_display, EGLImageKHR egl_image,
                      EGLCLientBuffer buffer)
{
    server_image_t *image = (server_image_t *)malloc (sizeof (server_image_t));
    server_native_surface_t *native_surface;

    image->egl_image = egl_image;
    image->mark_for_deletion = false;
    image->ref_count = 1;
    image->image_bindings = NULL;
    image->lock_server = 0;
    bool find_native_surface = false;

    /* look up native surface */
    link_list_t **native_surfaces = _server_native_surfaces ();
    link_list_t *head = *native_surfaces;

    while (head) {
        native_surface = (server_native_surface *)head->data;

        if (native_surface->native_surface == (void *)buffer) {
            native_surface->ref_count ++;
            find_native_surface = true;
            image->native_surface = head;
            break;
        }

        head = head->next;
    }

    if (find_native_surface == false) {
        native_surface = _server_native_surface_create ((void *)buffer);
        link_list_append (native_surfaces, native_surface, _destroy_native_surface);
        image->native_surface = native_surface;
    }

    return image;
}

/* convenience function to add eglimage, called by eglCreateImageKHR,
 * called by out-of-order eglCreateImageKHR */
void
_server_image_add (EGLDisplay egl_display, EGLImageKHR egl_image,
                   EGLClientBuffer buffer)
{
    server_display_list_t *display = _server_display_find (egl_display);
    if (! display)
        return;

    link_list_t **images = _server_images (display);
    link_list_t *head = *images;

    while (head) {
        server_image_t *image = (server_image_t *)head->data;

        if (image->egl_image == egl_image) {
            if (image->native_surface) {
                server_native_surface_t *native_surface = (server_native_surface_t *) image->native_surface->data;
                if (native_surface->native_surface == (void *)buffer) {
                    return;
                }
            }
        }
        head = head->next;
    }

    server_image_t *new_image = _server_image_create (egl_display,
                                                      egl_image,
                                                      buffer);
    link_list_append (images, new_image, _destroy_image);
}

/* remove an EGLImage, called by in-order eglDestroyImageKHR */
_server_image_remove (EGLDisplay egl_display, EGLImageKHR egl_image)
{
    server_display_list_t *display = _server_display_find (egl_display);

    if (! display)
        return;

    link_list_t **images = _server_images (display);
    link_list_t *head = *images;
    link_list_t **native_surfaces = _server_native_surfaces ();

    while (head) {
        server_image_t *image = (server_image_t *)head->data;
        if (image->egl_image == egl_image) {
            if (image->ref_count > 1)
                image->ref_count --;

            /* decrease reference to native surface */
            if (image->native_surface) {
                server_native_surface_t *native = (server_native_surface_t *)image->native_surface->data;
                if (native->ref_count > 1)
                    native->ref_count--;

                if (native->ref_count == 0 &&
                    natiive->lock_server == 0) {
                    link_list_delete_element (native_surfaces, image->native_surface);
                    image->native_surface = 0;
                }
            }

            if (image->ref_count == 0 &&
                image->lock_server == 0) {
                link_list_delete_element (images, head);
            }
            return;
        }

        head = head->next;
    }
}

/* mark deletion for EGLImage, called by out-of-order eglDestroyImageKHR
 * We must set mark_for_deletion with out-of-order. because, real
 * eglDestroyImageKHR can be later in pipeline.  If other threads, for
 * some reason, uses the delete eglimage again, it should fail.  But since
 * eglDestroyImageKHR can be processed later, the other thread might still
 * see the image is valid
 */
void
_server_image_mark_for_deletion (EGLDisplay egl_display,
                                 EGLImageKHR egl_image);
{
    server_display_list_t *display = server_display_find (egl_display);
    if (! display)
        return;

    link_list_t **images = _server_images (display);
    link_list_t *head = *images;

    while (head) {
        server_image_t *image = (server_image_t *)head->data;
        if (image->egl_image == egl_image)
            image->mark_for_deletion = true;

            /* decrease reference to native surface */
            if (image->native_surface) {
                server_native_surface_t *native = (server_native_surface_t *)image->native_surface->data;
                native->mark_for_deletion = true;
                return;
            }
        }

        head = head->next;
    }
}

/* lock EGLImage to a particular server, such that no on can delete it.
 * the lock succeeds only if eglimage has not been marked for deletion.
 * called by out-of-order glEGLImageTargetTexture2DOES and
 * glEGLImageTargetRenderbufferStorageOES
 */
void
_server_image_lock (EGLDisplay display, EGLImageKHR egl_image,
                    thread_t server)
{
    server_display_list_t *display = server_display_find (egl_display);
    if (! display)
        return;

    link_list_t **images = _server_images (display);
    link_list_t *head = *images;

    while (head) {
        server_image_t *image = (server_image_t *)head->data;
        if (image->egl_image == egl_image &&
            image->mark_for_deletion == false) {

            /* we can change the lock regardless of previous lock, because
             * the app must locked a user lock before call us,
             * we just need to change the current server lock to unlocked 
             * status such that next egl/gl call from that server will
             * check whether it can lock this image or lock on the native
             * surface
             */
            image->lock_server = server;
            image->ref_count ++;

            /* change lock on native surface */
            if (image->native_surface) {
                server_native_surface_t *native = (server_native_surface_t *)image->native_surface->data;
                native->lock = server;
                native->ref_count++;
                return;
            }
        }

        head = head->next;
    }
}
    
/* set image bindiing to a texture or a renderbuffer, called by
 * in-order glEGLImageTargetTexture2DOES or
 * glEGLImageTargetRenderbufferStorageOES
 */
void
_server_image_bind_buffer (EGLDisplay egl_display,
                           EGLContext egl_context,
                           EGLImageKHR egl_image,
                           server_binding_type_t type,
                           unsigned int buffer,
                           thread_t server)
{
    server_display_list_t *display = server_display_find (egl_display);
    if (! display)
        return;

    link_list_t **images = _server_images (display);
    link_list_t *head = *images;

    while (head) {
        server_image_t *image = (server_image_t *)head->data;
        if (image->egl_image == egl_image &&
            image->lock_server == server) {
            link_list_t *binding_head = image->image_bindings;

            while (binding_head) {
                server_binding_t *binding = (server_binding_t *)binding_head->data;
                if (binding->egl_display &&
                    binding->egl_context == egl_context &&
                    binding->binding_buffer == buffer) {
                    binding->ref_count ++;
                    return;
                }
                binding_head = binding_head->next;
            }

            /* we did not find binding, let's create */
            server_binding_t *new_bind = (server_binding_t *)malloc (sizeof (server_binding_t));
            new_bind->egl_display = egl_display;
            new_bind->egl_context = egl_context;
            new_bind->binding_buffer = buffer;
            new_bind->type = type;
            new_bind->ref_count = 1;
            link_list_add (&image->image_bindings, new_bind, _destroy_binding);
            return;
        }

        head = head->next;
    }
}

/* delete texture or renderbuffer causes image binding to release resources
 * called by glDeleteTextures or glDeleteRenderbuffers
 */
void
_server_image_unbind_buffer_and_unlock (EGLDisplay egl_display,
                                        EGLContext egl_context,
                                        EGLImageKHR egl_image,
                                        server_binding_type_t type,
                                        unsigned int buffer,
                                        thread_t server)
{
    server_display_list_t *display = server_display_find (egl_display);
    if (! display)
        return;

    link_list_t **images = _server_images (display);
    link_list_t *head = *images;
    link_list_t **native_surfaces = _server_native_surfaces ();

    while (head) {
        server_image_t *image = (server_image_t *)head->data;
        /* we igore mark_for_deletion here because we might be delayed */
        if (image->egl_image == egl_image &&
            image->lock_server == server) {
            link_list_t *binding_head = image->image_bindings;

            while (binding_head) {
                server_binding_t *binding = (server_binding_t *)binding_head->data;
                if (binding->egl_display &&
                    binding->egl_context == egl_context &&
                    binding->binding_buffer == buffer) {
                    binding->ref_count --;
                }

                if (binding->ref_count == 0) {
                    link_list_delete_element (&image->image_bindings, binding);
                }

                binding_head = binding_head->next;
            }

            image->ref_count--;
            image->lock_server = 0;

            if (image->native_surface) {
               server_native_surface_t *surface = (server_native_surface_t *)image->native_surface->data;
               if (surface->ref_count > 0) {
                  surface->ref_count --;
                  surface->lock_server = 0;
               }
            }

            return;
        }

        head = head->next;
    }
}

server_image_t *
_server_image_find (EGLDiisplay display, EGLImageKHR image)
{
    server_display_list_t *display = server_display_find (egl_display);
    if (! display)
        return NULL;

    link_list_t **images = _server_images ();
    link_list_t *head = *images;

    while (head) {
        server_image_t *img = (server_image_t *) head->data;

        if (img->egl_image == image)
            return img;

        head = head->next;
    }
}
                   
/*****************************************************************
 * native_surface functions 
 *****************************************************************/
server_native_surface_t *
_server_native_surface_create (void *native_surface)
{
    server_native_surface_t *surface = (server_native_surface_t *)malloc (sizeof (server_native_surface_t));

    surface->native_surface = native_surface;
    surface->lock_server = 0;
    surface->ref_count = 1;

    return surface;
}

/* add a native surface to cache, called by out-of-order eglCreateSurface */
void
_server_native_surface_add (EGLDisplay egl_display, void *native_surface)
{
    link_list_t **surfaces = _server_native_surfaces ();
    link_list_t *head = *surfaces;
    server_native_surface_t *surface;

    while (head) {
        surface = (server_native_surface_t *) head->data;

        if (surface->native_surface == native_surface) {
            return;
        }
        head = head->next;
    }

    surface = _server_native_surface_create (egl_display, native_surface);
    link_list_append (surfaces, surface);
}

/* mark deletion, called by out-of-order eglDestroySurface */
void
_server_native_surface_mark_for_deletion (EGLDisplay egl_display,
                                          void *native_display)
{
    link_list_t **surfaces = _server_native_surfaces ();
    link_list_t *head = *surfaces;
    server_native_surface_t *surface;

    while (head) {
        surface = (server_native_surface_t *) head->data;

        if (surface->native_surface == native_surface) {
            surface->mark_for_deletion = true;
            return;
        }
        head = head->next;
    }
}

/*****************************************************************
 * egl surface functions 
 *****************************************************************/
/* create server resource on EGLSurface, called by out-of-order
 * eglCreateXXXXSurface(), eglCreatePbufferFromClientBuffer and 
 * eglCreatePixmapSurfaceHI.  for the last two calls, we do not
 * save native surface for them
 */
server_surface_t *
_server_surface_create (EGLDisplay egl_display, EGLSurface egl_surface,
                        void *native_surface, server_surface_typ_t type)
{
    link_list_t **native_surfaces = _server_native_surfaces ();
    bool native_surface_find = false;

    server_surface_t *new_surface = (server_surface_t *) malloc (sizeof (server_surface_t));

    new_surface->egl_surface = egl_surface;
    new_surface->mark_for_deletion = false;
    new_surface->type = type;
    new_surface->ref_count = 1;
    new_surface->surface_binding = NULL;
    new_surface->lock_server = 0;
    new_surface->native_surface = NULL;

    link_list_t *head = *native_surfaces;
    while (head) {
        server_native_surface_t *native_surf = (server_native_surface_t *)head->data;
        if (native_surf->native_surface == native_surface) {
            new_surface->native_surface = head;
            native_surf->ref_count ++;
            native_surface_find = true;
            break;
        }

        head = head->next;
    }

    if (native_surface_find == false) {
        native_surface = _server_native_surface_create (native_surface);
        link_list_append (native_surfaces, native_surface, _destroy_native_surface);
        new_surface->native_surface = native_surface;
    }
    return new_surface;

}
  
/* create server resource on EGLSurface, called by out-of-order
 * eglCreateXXXXSurface(), eglCreatePbufferFromClientBuffer and 
 * eglCreatePixmapSurfaceHI.  for the last two calls, we do not
 * save native surface for them
 */
server_surface_t *
_server_surface_add (EGLDisplay egl_display, EGLSurface egl_surface,
                     void *native_surface, server_surface_typ_t type)
{
    server_display_list_t *display = server_display_find (egl_display);
    if (! display)
        return NULL;
    
    link_list_t **surfaces = _server_surfaces (display);
    link_list_t *head = *surfaces;

    while (head) {
        server_surface_t *surface = (server_surface_t *)head->data;

        if (surface->egl_surface == egl_surface) {
            if (surface->native_surface) {
                server_native_surface_t *native_surf = (server_native_surface_t *) surface->native_surface->data;
                if (native_surf->native_surface == (void *)buffer) {
                    return;
                }
            }
        }
        head = head->next;
    }

    server_surface_t *new_surface = _server_surface_create (egl_display,
                                                            egl_surface,
                                                            native_surface,
                                                            type);
    link_list_append (surfaces, new_surface, _destroy_surface);
}

/* remove an EGLSurface, called by in-order eglDestroyXXXSurface */
void
_server_surface_remove (EGLDisplay egl_display, EGLSurface egl_surface)
{
    server_display_list_t *display = server_display_find (egl_display);
    if (! display)
        return;
    
    link_list_t **surface = _server_surfaces (display);
    link_list_t *head = *surfaces;
    link_list_t **native_surfaces = _server_native_surfaces ();

    while (head) {
        server_surface_t *surface = (server_surface_t *)head->data;
        if (surface->egl_surface == egl_surface) {
            if (surface->ref_count > 1)
                surface->ref_count --;

            /* decrease reference to native surface */
            if (surface->native_surface) {
                server_native_surface_t *native = (server_native_surface_t *)image->native_surface->data;
                if (native->ref_count > 1)
                    native->ref_count--;

                if (native->ref_count == 0 &&
                    natiive->lock_server == 0) {
                    link_list_delete_element (native_surfaces, surface->native_surface);
                    surface->native_surface = 0;
                }
            }

            if (surface->ref_count == 0 &&
                surface->lock_server == 0) {
                link_list_delete_element (surfaces, head);
            }
            return;
        }

        head = head->next;
    }
}
 
/* mark deletion for EGLSurface, called by out-of-order eglDestroySurface
 * We must set mark_for_deletion with out-of-order. because, real
 * eglDestroySurface can be later in pipeline.  If other threads, for
 * some reason, uses the delete eglsurface again, it should fail.  But since
 * eglDestroySurface can be processed later, the other thread might still
 * see the surface is valid
 */
void
_server_surface_mark_for_deletion (EGLDisplay egl_display,
                                   EGLSurface egl_surface);
{
    server_display_list_t *display = server_display_find (egl_display);
    if (! display)
        return;
    
    link_list_t **surfaces = _server_surfaces (display);
    link_list_t *head = *surfaces;

    while (head) {
        server_surface_t *surface = (server_surface_t *)head->data;
        if (surface->egl_surface == egl_surface) {
            surface->mark_for_deletion = true;

            /* decrease reference to native surface */
            if (surface->native_surface) {
                server_native_surface_t *native = (server_native_surface_t *)surface->native_surface->data;
                native->mark_for_deletion = true;
                return;
            }
        }

        head = head->next;
    }
}

/* Lock and increase reference count on EGLSurface for a particular server
 * such that no one can delete it.  The lock is only successful if the
 * surface has not been marked for deletion and ref_count > 1, called by
 * out-of-order eglMakeCurrent
 */
EGLBoolean
_server_surface_lock (EGLDisplay egl_display, EGLSurface egl_surface,
                      thread_t server)
{
    server_display_list_t *display = server_display_find (egl_display);
    if (! display)
        return EGL_FALSE;
    
    link_list_t **surfaces = _server_surfaces (display);
    link_list_t *head = *surfaces;

    while (head) {
        server_surface_t *surface = (server_surface_t *)head;
        if (surface->egl_surface == egl_surface) {
            if (surface->mark_for_deletion == true &&
                surface->ref_count <= 1)
                return EGL_FALSE;

            surface->lock_server = server;
            surface->ref_count++;
            
            if (surface->native_surface) {
                server_native_surface_t *native = (server_native_surface_t *)surface->native_surface->data;
                native->ref_count++;
            }
             return EGL_TRUE;
        }
        head = head->next;
    }

    return EGL_FALSE;
}

/* called by in-order eglMakeCurrent, unlock surface from a particular
 * server, descrease ref_count, remove surface if mark_for_deletion 
 * and ref_count == 0
 */
EGLBoolean
_server_surface_unlock (EGLDisplay egl_display, EGLSurface egl_surface,
                        thread_t server)
{
    server_display_list_t *display = server_display_find (egl_display);
    if (! display)
        return FALSE;
    
    link_list_t **surfaces = _server_surfaces (display);
    link_list_t *head = *surfaces;

    while (head) {
        server_surface_t *surface = (server_surface_t *)head;
        if (surface->egl_surface == egl_surface) {
            surface->lock_server = 0;
            break;
        }
        head = head->next;
    }

    return _server_surface_remove (egl_display, egl_surface);
    return EGL_TRUE;
}
                    
link_list_t **
_registered_lock_requests ()
{
    static link_list_t *requests = NULL;

    return &requests;
}


