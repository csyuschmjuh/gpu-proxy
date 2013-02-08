#include "server_state.h"
#include "thread_private.h"
#include <stdlib.h>
#include <stdio.h>

/* get cached server side server_display_t list.  It is static such
 * that all servers share it
 */
link_list_t **
_server_displays ()
{
    static link_list_t *dpys = NULL;
    return &dpys;
}

/* obtain server side of X server display connection */
Display *
_server_get_display (EGLDisplay egl_display)
{
    link_list_t **dpys = _server_displays ();
    link_list_t *head = *dpys;

    while (head) {
        server_display_t *server_dpy = (server_display_t *)head->data;
        if (server_dpy->egl_display == egl_display)
            return server_dpy->server_display;

        head = head->next;
    }

    return NULL;
}

/* destroy server_display_t, called by link_list_delete_element */
void
_destroy_display (void *abstract_display)
{
    server_display_t *dpy = (server_display_t *) abstract_display;

    XCloseDisplay (dpy->server_display);

    free (dpy);
}

/* create a new server_display_t sture */
server_display_t *
_server_display_create (Display *server_display,
                        Display *client_display,
                        EGLDisplay egl_display)
{
    server_display_t *server_dpy = (server_display_t *) malloc (sizeof (server_display_t));
    server_dpy->server_display = server_display;
    server_dpy->egl_display = egl_display;
    server_dpy->client_display = client_display;
    server_dpy->ref_count = 0;
    server_dpy->mark_for_deletion = false;
    return server_dpy;
}

/* add new EGLDisplay to cache, called in eglGetDisplay */
void
_server_display_add (Display *server_display, Display *client_display,
                     EGLDisplay egl_display)
{
    link_list_t **dpys = _server_displays ();
    server_display_t *server_dpy;
    link_list_t *head = *dpys;

    while (head) {
        server_dpy = (server_display_t *)head->data;

        if (server_dpy->server_display == server_display &&
            server_dpy->client_display == client_display &&
            server_dpy->egl_display == egl_display) {
            server_dpy->ref_count ++;
            return;
        }
        head = head->next;
    }

    server_dpy = _server_display_create (server_display, client_display, egl_display);
    link_list_append (dpys, (void *)server_dpy, _destroy_display);
}

/* remove a server_display from cache, called by eglTerminate */
void
_server_display_remove (Display *server_display, EGLDisplay egl_display)
{
    link_list_t **dpys = _server_displays ();
    server_display_t *server_dpy;
    link_list_t *head = *dpys;

    while (head) {
        server_dpy = (server_display_t *) head->data;

        if (server_dpy->server_display == server_display &&
            server_dpy->egl_display == egl_display) {
            if (server_dpy->ref_count > 1)
                server_dpy->ref_count --;
            
            if (server_dpy->ref_count == 0 &&
                server_dpy->mark_for_deletion == true) {
                link_list_delete_element (dpys, head);
	        return;
            }
        }
        head = head->next;
    }
}

void
_server_destroy_mark_for_deletion (EGLDisplay egl_display)
{
    /* look into cached display */
    link_list_t **dpys = _server_displays ();
    link_list_t *head = *dpys;

    while (head) {
        server_display_t *dpy = (server_display_t *) head->data;
        if (dpy->egl_display == egl_display) {
            dpy_mark_for_deletion = true;
         }
         head = head->next;
    }

    /* next is the contexts */
    link_list_t **ctxs = _server_contexts ();
    head = *ctxs;
    
    while (head) {
        server_context_t *ctx = (server_context_t *) head->data;
        if (ctx->egl_display == egl_display) {
            ctx->mark_for_deletion = true;
             }
         }
         head = head->next;
    }

    /* surface deletion */
    link_list_t **surfaces = _server_surfaces ();
    head = surfaces;
 
    while (head) {
        server_surface_t *surface = (server_surface_t *) head->data;
        if (surface->egl_display == egl_display) {
            surface->mark_for_deletion = true;
         }
         head = head->next;
    }
    
    /* eglimages */
    link_list_t **images = _server_images ();
    head = images;
 
    while (head) {
        server_image_t *image = (server_image_t *) head->data;
        if (image->egl_display == egl_display) {
            image->mark_for_deletion = true;
         }
         head = head->next;
    }
}

bool
_server_destroy_display_mark_for_deletion (EGLDisplay egl_display)
{
    /* look into cached display */
    link_list_t **dpys = _server_displays ();
    link_list_t *head = *dpys;
    bool has_resources = false;

    while (head) {
        server_display_t *dpy = (server_display_t *) head->data;
        if (dpy->egl_display == egl_display) {
            if (dpy->mark_for_deletion == true) {
                link_list_t *next = head->next;
                link_list_delete_element (dpys, head);
                if (dpys == NULL)
                    break;
                else
                    head = next;
             }
             else
                 has_resources = true;
         }
         head = head->next;
    }

    /* next is the contexts */
    link_list_t **ctxs = _server_contexts ();
    head = *ctxs;
    
    while (head) {
        server_context_t *ctx = (server_context_t *) head->data;
        if (ctx->egl_display == egl_display) {
            if (ctx->mark_for_deletion == true &&
                ctx->ref_count == 0) {
                link_list_t *next = head->next;
                link_list_delete_element (ctxs, head);
                if (ctxs == NULL)
                    break;
                else
                    head = next;
             }
             else
                 has_resources = true;
         }
         head = head->next;
    }

    /* surface deletion */
    link_list_t **surfaces = _server_surfaces ();
    head = surfaces;
 
    while (head) {
        server_surface_t *surface = (server_surface_t *) head->data;
        if (surface->egl_display == egl_display) {
            if (surface->mark_for_deletion == true &&
                surface->ref_count == 0 &&
                surface->lock_server == 0) {
                link_list_t *next = head->next;
                link_list_delete_element (surfaces, head);
                if (surfaces == NULL)
                    break;
                else
                    head = next;
             }
             else
                 has_resources = true;
         }
         head = head->next;
    }
    
    /* eglimages */
    link_list_t **images = _server_images ();
    head = images;
 
    while (head) {
        server_image_t *image = (server_image_t *) head->data;
        if (image->egl_display == egl_display) {
            if (image->mark_for_deletion == true &&
                image->ref_count == 0 &&
                image->lock_server == 0) {
                link_list_t *next = head->next;
                link_list_delete_element (images, head);
                if (images == NULL)
                    break;
                else
                    head = next;
             }
             else
                 has_resources = true;
         }
         head = head->next;
    }

    /* native surfaces */
    link_list_t **native_surfaces = _server_native_surfaces ();

    while (head) {
        server_native_surface_t *native_surface = (server_native_surface_t *) head->data;
        if (native_surface->ref_count == 0 &&
            native_surface->lock_server == 0) {
            link_list_t *next = head->next;
            link_list_delete_element (native_surfaces, head);
            if (native_surfaces == NULL)
                break;
            else
                head = next;
         }
         head = head->next;
    }

    return has_resources;
}

void
_server_destroy_display (Display *client_display)
{
    /* look into cached display */
    link_list_t **dpys = _server_displays ();
    link_list_t *head = *dpys;
    EGLDisplay egl_display = EGL_NO_DISPLAY;

    while (head) {
        server_display_t *dpy = (server_display_t *) head->data;
        if (dpy->client_display== client_display) {
            egl_display = dpy->egl_display;
            
            link_list_t *next = head->next;
            link_list_delete_element (dpys, head);
            if (dpys == NULL)
                break;
            else
                head = next;
         }
         head = head->next;
    }

    /* next is the contexts */
    link_list_t **ctxs = _server_contexts ();
    head = *ctxs;
    
    while (head) {
        server_context_t *ctx = (server_context_t *) head->data;
        if (ctx->egl_display == egl_display) {
            link_list_t *next = head->next;
            link_list_delete_element (ctxs, head);
            if (ctxs == NULL)
                break;
            else
                head = next;
         }
         head = head->next;
    }

    /* surface deletion */
    link_list_t **surfaces = _server_surfaces ();
    head = surfaces;
 
    while (head) {
        server_surface_t *surface = (server_surface_t *) head->data;
        if (surface->egl_display == egl_display) {
            link_list_t *next = head->next;
            link_list_delete_element (surfaces, head);
            if (surfaces == NULL)
                break;
            else
                head = next;
         }
         head = head->next;
    }
    
    /* eglimages */
    link_list_t **images = _server_images ();
    head = images;
 
    while (head) {
        server_image_t *image = (server_image_t *) head->data;
        if (image->egl_display == egl_display) {
            link_list_t *next = head->next;
            link_list_delete_element (images, head);
            if (images == NULL)
                break;
            else
                head = next;
         }
         head = head->next;
    }

    /* native surfaces */
    link_list_t **native_surfaces = _server_native_surfaces ();

    while (head) {
        server_native_surface_t *native_surface = (server_native_surface_t *) head->data;
        if (native_surface->ref_count == 0)
            link_list_t *next = head->next;
            link_list_delete_element (native_surfaces, head);
            if (native_surfaces == NULL)
                break;
            else
                head = next;
         }
         head = head->next;
    }
}

/* called by link_list_element from _destroy_image */
void
_destroy_binding (void *abstract_binding)
{
    free (abstract_binding);
}

/* called by link_list_delete_element */
void
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
    link_list_clear (image->image_bindings);

    free (image);
}

/* get cached eglimages */
link_list_t **
_server_images ()
{
    static link_list_t *images = NULL;
    return images;
}

/* create a server_image_t, called by eglCreateImageKHR */
_server_image_create (EGLDisplay egl_display, EGLImageKHR egl_image,
                      EGLCLientBuffer buffer)
{
    server_image_t *image = (server_image_t *)malloc (sizeof (server_image_t));
    server_native_surface_t *native_surface;

    image->egl_display = egl_display;
    image->egl_image = egl_image;
    image->mark_for_deletion = false;
    image->ref_count = 0;
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
        native_surface->ref_count++;
        link_list_append (native_surfaces, native_surface, _destroy_native_surface);
        image->native_surface = native_surface;
    }

    return image;
}

/* convenience function to add eglimage, called by eglCreateImageKHR */
void
_server_image_add (EGLDisplay egl_display, EGLImageKHR egl_image,
                   EGLClientBuffer buffer)
{
    link_list_t **images = _server_images ();
    link_list_t *head = *images;

    while (head) {
        server_image_t *image = (server_image_t *)head->data;

        if (image->egl_image == egl_image &&
            image->egl_display == egl_display) {
            if (image->native_surface) {
                server_native_surface_t *native_surface = (server_native_surface_t *) image->native_surface->data;
                if (native_surface->native_surface == (void *)buffer) {
                    native_surface->ref_count ++;
                    image->ref_count ++;
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
    link_list_t **images = _server_images ();
    link_list_t *head = *images;
    link_list_t **native_surfaces = _server_native_surfaces ();

    while (head) {
        server_image_t *image = (server_image_t *)head->data;
        if (image->egl_display == egl_display &&
            image->egl_image == egl_image) {
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
    link_list_t **images = _server_images ();
    link_list_t *head = *images;

    while (head) {
        server_image_t *image = (server_image_t *)head->data;
        if (image->egl_display == egl_display &&
            image->egl_image == egl_image)
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
_server_image_lock (EGLImageKHR egl_image, thread_t server)
{
    link_list_t **images = _server_images ();
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
    link_list_t **images = _server_images ();
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
    link_list_t **images = _server_images ();
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
                   
/* get cached native surfaces */
link_list_t **
_server_native_surfaces ()
{
    static link_list_t *native_surfaces = NULL;
    return &native_surfaces;
}

void
_native_surface_destroy (void *abstract_native_surface)
{
    free (abstract_native_surface);
}

server_native_surface_t *
_server_native_surface_create (void *native_surface)
{
    server_native_surface_t *surface = (server_native_surface_t *)malloc (sizeof (server_native_surface_t));

    surface->native_surface = native_surface;
    surface->lock_server = 0;
    surface->ref_count = 0;

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

        if (surface->egl_display == egl_display &&
            surface->native_surface == native_surface) {
            surface->ref_count ++;
            return;
        }
        head = head->next;
    }

    surface = _server_native_surface_create (egl_display, native_surface);
    surface->ref_count = 1;
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

        if (surface->egl_display == egl_display &&
            surface->native_surface == native_surface) {
            surface->mark_for_deletion = true;
            return;
        }
        head = head->next;
    }
}
  
 
                    



           link_list_t *native_head = *native_surfaces;

           while (native_head) {
               server_native_surface_t *native_surface = (server_native_surface_t *)native_head->data;
               if (native_surface->native_surface == (void *)buffer) {
               

    

    

link_list_t **
_server_shared_contexts ()
{
    static link_list_t *contexts = NULL;
    return &contexts;
}

link_list_t **
_server_shared_surfaces ()
{
    static link_list_t *surfaces = NULL;
    return &surfaces;
}

server_surface_t *
_server_surface_create (EGLDisplay display, EGLSurface surface)
{
    server_surface_t *surf = (server_surface_t *) malloc (sizeof (server_surface_t));

    surf->display = display;
    surf->surface = surface;
    return surf;
}

void
_server_shared_surfaces_add (EGLDisplay display, EGLSurface surface)
{
    link_list_t **surfaces = _server_shared_surfaces ();
    server_surface_t *surf;
    link_list_t *head = *surfaces;

    while (head) {
        surf = (server_surface_t *)head->data;

        if (surf->display == display &&
            surf->surface == surface)
            return;
        head = head->next;
    }

    surf = _server_surface_create (display, surface);
    link_list_append (surfaces, (void *)surf, _destroy_surface);
}

void
_server_shared_surfaces_remove (EGLDisplay display, EGLSurface surface)
{
    link_list_t **surfaces = _server_shared_surfaces ();
    link_list_t *head = *surfaces;

    if (!head)
	return;
 
    while (head) {
	server_surface_t *surf = (server_surface_t *)head->data;

        if (surf->surface == surface &&
            surf->display == display) {
            link_list_delete_first_entry_matching_data (surfaces, (void *)surf);
            return;
        }
        head = head->next;
    }
}

void
_destroy_surface (void *abstract_surface)
{
    free (abstract_surface);
}

server_context_t *
_server_context_create (EGLDisplay display, EGLContext context)
{
    server_context_t *ctx = (server_context_t *) malloc (sizeof (server_context_t));

    ctx->display = display;
    ctx->context = context;
    return ctx;
}

void
_server_shared_contexts_add (EGLDisplay display, EGLContext context)
{
    link_list_t **contexts = _server_shared_contexts ();
    link_list_t *head = *contexts;
    server_context_t *ctx;

    while (head) {
        ctx = (server_context_t *)head->data;
        if (ctx->display == display &&
            ctx->context == context)
	    return;

        head = head->next;
    }

    ctx = _server_context_create (display, context);
    link_list_append (contexts, (void *)ctx, _destroy_context);
}

void
_server_shared_contexts_remove (EGLDisplay display, EGLContext context)
{
    link_list_t **contexts = _server_shared_contexts ();
    link_list_t *head = *contexts;

    if (!head)
	return;
 
    while (head) {
	server_context_t *ctx = (server_context_t *)head->data;

        if (ctx == context &&
            ctx->display == display) {
            link_list_delete_first_entry_matching_data (contexts, (void *)ctx);
            return;
        }
        head = head->next;
    }
}

void
_destroy_context (void *abstract_context)
{
    free (abstract_context);
}

link_list_t **
_server_eglimages ()
{
    static link_list_t *eglimages = NULL;
    return &eglimages;
}

server_eglimage_t *
_server_eglimage_create (EGLDisplay display, EGLImageKHR image, EGLContext context)
{
    server_eglimage_t *eglimage = (server_eglimage_t *) malloc (sizeof (server_eglimage_t));

    eglimage->display = display;
    eglimage->image = image;
    eglimage->context = context;
    return eglimage;
}

void
_server_eglimage_remove (EGLDisplay display, EGLImageKHR image)
{
    link_list_t **eglimages = _server_eglimages ();
    link_list_t *head = *eglimages;

    if (!head)
	return;
 
    while (head) {
	server_eglimage_t *eglimage = (server_eglimage_t *)head->data;

        if (eglimage->image == image) {
            link_list_delete_first_entry_matching_data (eglimages, (void *)eglimage);
            return;
        }
        head = head->next;
    }
}

void
_server_eglimage_add (EGLDisplay display, EGLImageKHR image, EGLContext context)
{
    link_list_t **eglimages = _server_eglimages ();
    server_eglimage_t *eglimage;
    link_list_t *head = *eglimages;

    while (head) {
        eglimage = (server_eglimage_t *)head->data;

        if (eglimage->display == display &&
            eglimage->image == image &&
            eglimage->context == context)
            return;

        head = head->next;
    }
    eglimage = _server_eglimage_create (display, image, context);
    link_list_append (eglimages, (void *) eglimage, _destroy_eglimage);
}

void
_destroy_eglimage (void *abstract_image)
{
    free (abstract_image);
}

link_list_t **
_registered_lock_requests ()
{
    static link_list_t *requests = NULL;

    return &requests;
}


