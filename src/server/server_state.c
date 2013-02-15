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
_server_native_resources ()
{
    static link_list_t *resources = NULL;
    return &resources;
}

static link_list_t **
_server_displays ()
{
    static link_list_t *dpys = NULL;
    return &dpys;
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
_server_context_groups (server_display_list_t *display)
{
    return &display->context_groups;
}

static link_list_t **
_server_contexts (server_context_group_t *group)
{
    return &group->contexts;
}

static void
_destroy_native_resource (void *abstract_resource)
{
    native_resource_t *resource = (native_resource_t *)abstract_resource;

    link_list_clear (&resource->lock_servers);
    free (resource);
}

/* called by link_list_delete_element */
static void
_destroy_image (void *abstract_image)
{
    server_image_t *image = (server_image_t *)abstract_image;

    link_list_clear (&image->lock_servers);
    if (image->native) {
        image->native->ref_count--;
        if (image->native->ref_count == 0)
            link_list_delete_first_entry_matching_data (_server_native_resources (), image->native);
    }
    free (image);
}

static void
_destroy_surface (void *abstract_surface)
{
    server_surface_t *surface = (server_surface_t *)abstract_surface;

    link_list_clear (&surface->lock_servers);
    if (surface->native) {
        surface->native->ref_count--;

        if (surface->native->ref_count == 0)
            link_list_delete_first_entry_matching_data (_server_native_resources (), surface->native);
    }
    free (surface);
}

static void
_destroy_context (void *abstract_context)
{
    server_context_t *context = (server_context_t *)abstract_context;
    link_list_clear (&context->lock_servers);
    free (context);
}

/* destroy server_display_list_t, called by link_list_delete_element */
static void
_destroy_display (void *abstract_display)
{
    server_display_list_t *dpy = (server_display_list_t *) abstract_display;

    XCloseDisplay (dpy->server_display);

    link_list_clear (&dpy->images);
    link_list_clear (&dpy->context_groups);
    link_list_clear (&dpy->surfaces);

    free (dpy);
}

static void
_destroy_context_group (void *abstract_group)
{
    server_context_group_t *group = (server_context_group_t *)abstract_group;
    link_list_clear (&group->contexts);
    free (group);
}

static server_context_group_t *
_find_context_group (server_display_list_t *display, EGLContext egl_context)
{
    link_list_t **groups = _server_context_groups (display);
    link_list_t *head = *groups;

    while (head) {
        server_context_group_t *group = (server_context_group_t *)head->data;
        link_list_t **contexts = _server_contexts (group);
        link_list_t *g_head = *contexts;

        while (g_head) {
            server_context_t *context = (server_context_t *)g_head->data;
            if (context->egl_context == egl_context)
                return group;
            g_head = g_head->next;
        }
        head = head->next;
    }
    return NULL;
}

static server_context_t *
_find_context (server_context_group_t *group, EGLContext egl_context)
{
    if (! group || !group->contexts)
        return NULL;
    link_list_t *head = group->contexts;

    while (head) {
        server_context_t *context = (server_context_t *)head->data;
        if (context->egl_context == egl_context)
            return context;
        head = head->next;
    }
    return NULL;
}

static server_context_group_t *
_server_context_group_create ()
{
    server_context_group_t *group = (server_context_group_t *)malloc (sizeof (server_context_group_t));
    group->contexts = NULL;
    return group;
}

static void
_server_context_group_remove (server_display_list_t *display,
                              server_context_group_t *group)
{
    link_list_delete_first_entry_matching_data (&display->context_groups, group);
}

static void
_add_context (server_display_list_t *display, server_context_t *context)
{
    server_context_group_t *group = _find_context_group (display, context->egl_context);

    if (group == NULL && context->share_context)
        group = _find_context_group (display, context->share_context->egl_context);

    /* add context to existing group */
    if (group) {
        link_list_append (&group->contexts, context, _destroy_context);
    }
    else {
        group = _server_context_group_create ();
        link_list_append (&group->contexts, context, _destroy_context);
        link_list_append (&display->context_groups, group, _destroy_context_group);
    }
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
    server_dpy->ref_count = 0;
    server_dpy->mark_for_deletion = false;

    server_dpy->images = NULL;
    server_dpy->context_groups = NULL;
    server_dpy->surfaces = NULL;
    return server_dpy;
}

/* add new EGLDisplay to cache, called in out-of-order eglGetDisplay */
void
_server_display_add (Display *server_display, Display *client_display,
                     EGLDisplay egl_display)
{
    link_list_t **dpys = _server_displays ();
    server_display_list_t *server_dpy;
    link_list_t *head = *dpys;

    while (head) {
        server_dpy = (server_display_list_t *)head->data;

        if (server_dpy->server_display == server_display &&
            server_dpy->client_display == client_display &&
            server_dpy->egl_display == egl_display &&
            server_dpy->mark_for_deletion == false)
            return;
 
        head = head->next;
    }

    server_dpy = _server_display_create (server_display, client_display, egl_display);
    link_list_append (dpys, (void *)server_dpy, _destroy_display);
}

/* remove a server_display from cache, called by in-order eglTerminate 
 * and by in-order eglMakeCurrent */
void
_server_display_remove (server_display_list_t *display)
{
    link_list_t **displays = _server_displays ();
    link_list_t *head = *displays;

    if (display->ref_count > 1)
        display->ref_count--;

    if (display->ref_count == 0 &&
        display->mark_for_deletion == true) {
        link_list_delete_element (displays, head);
        return;
    }
}

/* mark display for removal, called by out-of-order eglTerminate */
bool
_server_destroy_mark_for_deletion (server_display_list_t *display)
{
    display->mark_for_deletion = true;

    /* next is the contexts */
    link_list_t **groups = _server_context_groups (display);
    link_list_t *head = *groups;
    while (head) {
        server_context_group_t *group = (server_context_group_t *)head->data;
        link_list_t **ctxs = _server_contexts (group);
        link_list_t *ctx_head = *ctxs;
    
        while (ctx_head) {
            server_context_t *ctx = (server_context_t *) ctx_head->data;
            ctx->mark_for_deletion = true;

            ctx_head = ctx_head->next;
        }
        head = head->next;
    }

    /* surface deletion */
    link_list_t **surfaces = _server_surfaces (display);
    head = *surfaces;
 
    while (head) {
        server_surface_t *surface = (server_surface_t *) head->data;
        surface->mark_for_deletion = true;
        head = head->next;
    }
    
    /* eglimages */
    link_list_t **images = _server_images (display);
    head = *images;
 
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
    link_list_t *head = *dpys;

    while (head) {
        server_display_list_t *dpy = (server_display_list_t *) head->data;
        if (dpy->egl_display == egl_display)
            return dpy;

        head = head->next;
    }
    return NULL;
}

static native_resource_t *
_server_native_resource_create (void *native)
{
    native_resource_t *resource = (native_resource_t *) malloc (sizeof (native_resource_t));
    resource->ref_count = 1;
    resource->lock_servers = NULL;
    resource->locked_server = 0;
}

static native_resource_t *
_server_native_resource_add (void *native)
{
    link_list_t **resources = _server_native_resources ();
    link_list_t *head = *resources;

    while (head) {
        native_resource_t *resource = (native_resource_t *)head->data;
        if (resource->native == native) {
            resource->ref_count ++;
            return resource;
        }
        head = head->next;
    }

    native_resource_t *new_resource = _server_native_resource_create (native);
    link_list_append (resources, new_resource, _destroy_native_resource);
    return new_resource;
}

/*****************************************************************
 * image related functions 
 *****************************************************************/
/* create a server_image_t, out-of-order called by eglCreateImageKHR */
server_image_t *
_server_image_create (EGLImageKHR egl_image, void *native)
{
    server_image_t *image = (server_image_t *)malloc (sizeof (server_image_t));
    image->egl_image = egl_image;
    image->mark_for_deletion = false;
    image->binding_buffer = 0;
    image->binding_context = EGL_NO_CONTEXT;
    image->lock_servers = NULL;
    image->locked_server = 0;
    image->binding_type = NO_BINDING;
    
    /* look for native sources */
    image->native = _server_native_resource_add (native);

    return image;
}

/* convenience function to add eglimage, called by eglCreateImageKHR,
 * called by out-of-order eglCreateImageKHR */
void
_server_image_add (server_display_list_t *display, EGLImageKHR egl_image,
                   void *native)
{
    link_list_t **images = _server_images (display);
    link_list_t *head = *images;

    while (head) {
        server_image_t *image = (server_image_t *)head->data;

        if (image->egl_image == egl_image) {
            return;
        }
        head = head->next;
    }

    server_image_t *new_image = _server_image_create (egl_image, native);
    link_list_append (images, new_image, _destroy_image);
}

/* remove an EGLImage, called by in-order eglDestroyImageKHR */
void
_server_image_remove (server_display_list_t *display, EGLImageKHR egl_image)
{
    link_list_t **images = _server_images (display);
    link_list_t *head = *images;

    while (head) {
        server_image_t *image = (server_image_t *)head->data;
        thread_t thread = 0;
        if (image->lock_servers) {
            thread_t *t = (thread_t *) image->lock_servers->data;
            thread = *t;
        }
        if (image->egl_image == egl_image &&
            image->locked_server == 0 &&
            thread == 0 &&
            image->mark_for_deletion == true) {
            link_list_delete_element (images, head);
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
_server_image_mark_for_deletion (server_display_list_t *display,
                                 EGLImageKHR egl_image)
{
    link_list_t **images = _server_images (display);
    link_list_t *head = *images;

    while (head) {
        server_image_t *image = (server_image_t *)head->data;
        if (image->egl_image == egl_image) {
            image->mark_for_deletion = true;
            return;
        }

        head = head->next;
    }
}

/* called by out-of-order glEGLImageTargetTexture2DOES, 
 * glEGLImageTargetRenderbufferStorageOES.  
 */
void
_server_image_lock (server_display_list_t *display, EGLImageKHR egl_image,
                    thread_t server)
{
    link_list_t **images = _server_images (display);
    link_list_t *head = *images;

    while (head) {
        server_image_t *image = (server_image_t *) head->data;
        if (image->egl_image == egl_image) {
            thread_t *thread_ptr = (thread_t *)malloc (sizeof (thread_t));
            *thread_ptr = server;
            link_list_append (&image->lock_servers, thread_ptr, free);
            
            if (image->native) {
                thread_t *native_thread_ptr = (thread_t *)malloc (sizeof (thread_t));
                *native_thread_ptr = server;
                link_list_append (&image->native->lock_servers, native_thread_ptr, free);
            }
            return;
        }
        head = head->next;
    }
}

/* called by in-order glEGLImageTarggetTexture2DOES or
 * glEGLImageTargetRenderbufferStorageOES, return SERVER_WAIT if
 * locked_server != 0
 */
server_response_type_t
_server_image_bind_buffer (server_display_list_t *display,
                           EGLImageKHR egl_image,
                           EGLContext egl_context, unsigned int buffer,
                           server_binding_type_t type,
                           thread_t server)
{
    link_list_t **images = _server_images (display);
    link_list_t *head = *images;

    while (head) {
        server_image_t *image = (server_image_t *)head->data;
        if (image->egl_image == egl_image) {
            if (image->locked_server != 0)
                return SERVER_WAIT;

            /* we did not register for lock before because either we
             * did not make out-of-order call, or when we made call,
             * it has been marked for deletion
             */
            if (image->lock_servers == NULL)
                return SERVER_FALSE;
            thread_t *thread = (thread_t *) image->lock_servers->data;
            if (*thread != server)
                return SERVER_WAIT;

            /* is native resource locked ? */
            if (image->native) {
                if (image->native->locked_server != server &&
                    image->native->locked_server != 0)
                    return SERVER_WAIT;
                else if (image->native->locked_server == 0) {
                    thread = (thread_t *) image->native->lock_servers->data;
                    if (*thread != server)
                        return SERVER_WAIT;
                }
            }

            /* update locking status */
            link_list_delete_element (&image->lock_servers, image->lock_servers);
            image->locked_server = server;
            image->binding_context = egl_context;
            image->binding_buffer = buffer;
            image->binding_type = type;

            if (image->native) {
                link_list_delete_element (&image->native->lock_servers,
                                          image->native->lock_servers);
                image->native->locked_server = server;
            }
            return SERVER_TRUE;
        }
        head = head->next;
    }

    return SERVER_FALSE;
}

/* glDeleteTexture and glDeleteRenderbuffer causes unbinding of 
 * image
 */            
void
_server_image_unbind_buffer (server_display_list_t *display,
                             EGLContext egl_context,
                             server_binding_type_t type,
                             unsigned int buffer,
                             thread_t server)
{
    link_list_t **images = _server_images (display);
    link_list_t *head = *images;

    while (head) {
        server_image_t *image = (server_image_t *)head->data;
        if (image->binding_context == egl_context &&
            image->binding_type == type &&
            image->binding_buffer == buffer &&
            image->locked_server == server) {
                /* has eglDestroyImageKHR called ? */
            if (image->mark_for_deletion == true) {
                _server_image_remove (display, image->egl_image);
                return;
            }

            image->locked_server = 0;
            image->binding_context = EGL_NO_CONTEXT;
            image->binding_type = NO_BINDING;
            image->binding_buffer = 0;

            if (image->native) {
                image->native->locked_server = 0;
            }
            return;
        }
        head = head->next;
    }
}

server_image_t *
_server_image_find (EGLDisplay egl_display, EGLImageKHR image)
{
    server_display_list_t *display = _server_display_find (egl_display);
    if (! display)
        return NULL;

    link_list_t **images = _server_images (display);
    link_list_t *head = *images;

    while (head) {
        server_image_t *img = (server_image_t *) head->data;

        if (img->egl_image == image)
            return img;

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
_server_surface_create (server_display_list_t *display,
                        EGLSurface egl_surface,
                        server_surface_type_t type,
                        void *native)
{
    server_surface_t *new_surface = (server_surface_t *) malloc (sizeof (server_surface_t));

    new_surface->egl_surface = egl_surface;
    new_surface->mark_for_deletion = false;
    new_surface->type = type;
    new_surface->binding_buffer = 0;
    new_surface->binding_context = EGL_NO_CONTEXT;
    new_surface->locked_server = 0;
    new_surface->lock_servers = NULL;

    new_surface->lock_status = 0;
    new_surface->lock_visited = false;
    new_surface->native = NULL;

    if (type != PBUFFER_SURFACE) {
        /* look for native sources */
        new_surface->native = _server_native_resource_add (native);
    }
    return new_surface;

}
  
/* create server resource on EGLSurface, called by out-of-order
 * eglCreateXXXXSurface(), eglCreatePbufferFromClientBuffer and 
 * eglCreatePixmapSurfaceHI.  for the last two calls, we do not
 * save native surface for them
 */
void
_server_surface_add (server_display_list_t *display,
                     EGLSurface egl_surface,
                     server_surface_type_t type,
                     void *native)
{
    link_list_t **surfaces = _server_surfaces (display);
    link_list_t *head = *surfaces;

    while (head) {
        server_surface_t *surface = (server_surface_t *)head->data;

        if (surface->egl_surface == egl_surface &&
            surface->type == type) {
            return;
        }
        head = head->next;
    }

    server_surface_t *new_surface = _server_surface_create (display,
                                                            egl_surface,
                                                            type, native);
    link_list_append (surfaces, new_surface, _destroy_surface);
}

/* remove an EGLSurface, called by in-order eglDestroyXXXSurface */
void
_server_surface_remove (server_display_list_t *display, EGLSurface egl_surface)
{
    link_list_t **surfaces = _server_surfaces (display);
    link_list_t *head = *surfaces;

    while (head) {
        server_surface_t *surface = (server_surface_t *)head->data;
        thread_t thread = 0;
        if (surface->lock_servers) {
            thread_t *t = (thread_t *) surface->lock_servers->data;
            thread = *t;
        }
        if (surface->egl_surface == egl_surface &&
            surface->locked_server == 0 &&
            thread == 0 &&
            surface->mark_for_deletion == true) {
            link_list_delete_element (surfaces, head);
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
_server_surface_mark_for_deletion (server_display_list_t *display,
                                   EGLSurface egl_surface)
{
    link_list_t **surfaces = _server_surfaces (display);
    link_list_t *head = *surfaces;

    while (head) {
        server_surface_t *surface = (server_surface_t *)head->data;
        if (surface->egl_surface == egl_surface) {
            surface->mark_for_deletion = true;

            return;
        }

        head = head->next;
    }
}

/* lock to a particular server, called by out-of-order eglBindTexImage
 */
void
_server_surface_lock (server_display_list_t *display,
                      EGLSurface egl_surface,
                      thread_t server)
{
    link_list_t **surfaces = _server_surfaces (display);
    link_list_t *head = *surfaces;

    while (head) {
        server_surface_t *surface = (server_surface_t *)head->data;
        if (surface->egl_surface == egl_surface &&
            surface->mark_for_deletion == false) {
            thread_t *thread_ptr = (thread_t *)malloc (sizeof (thread_t));
            *thread_ptr = server;
            link_list_append (&surface->lock_servers, thread_ptr, free);

            if (surface->native) {
                thread_t *native_thread_ptr = (thread_t *)malloc (sizeof (thread_t));
                *native_thread_ptr = server;
                link_list_append (&surface->native->lock_servers, native_thread_ptr, free);
            }

            return;    
        }

        head = head->next;
    }
}

/* called by in-order glBindTexImage, if the surface locked_server != 0
 * return SERVER_WAIT
 */
server_response_type_t
_server_surface_bind_buffer (server_display_list_t *display,
                             EGLSurface egl_surface,
                             EGLContext egl_context,
                             unsigned int buffer,
                             thread_t server)
{
    link_list_t **surfaces = _server_surfaces (display);
    link_list_t *head = *surfaces;

    while (head) {
        server_surface_t *surface = (server_surface_t *)head->data;
        if (surface->egl_surface == egl_surface) {
            if (surface->locked_server != 0)
                return SERVER_WAIT;

            /* we did not register for lock before because either we
             * did not make out-of-order call, or when we made call,
             * it has been marked for deletion
             */
            if (surface->lock_servers == NULL)
                return SERVER_FALSE;
            thread_t *thread = (thread_t *) surface->lock_servers->data;
            if (*thread != server)
                return SERVER_WAIT;

            /* native resource locked ? */
            if (surface->native) {
                if (surface->native->locked_server != server &&
                    surface->native->locked_server != 0)
                    return SERVER_WAIT;
                else if (surface->native->locked_server == 0) {
                    thread = (thread_t *) surface->native->lock_servers->data;
                    if (*thread != server)
                        return SERVER_WAIT;
                }
            }

            /* update locking status */
            link_list_delete_element (&surface->lock_servers, surface->lock_servers);
            surface->locked_server = server;
            surface->binding_context = egl_context;
            surface->binding_buffer = buffer;

            if (surface->native) {
                surface->native->locked_server = server;
                link_list_delete_element (&surface->native->lock_servers, surface->native->lock_servers);
            }
            return SERVER_TRUE;
        }
        head = head->next;
    }

    return SERVER_FALSE;
}

/* called by in-order eglReleaseImage */
void
_server_surface_unbind_buffer (server_display_list_t *display,
                               EGLSurface egl_surface,
                               EGLContext egl_context)
{
    link_list_t **surfaces = _server_surfaces (display);
    link_list_t *head = *surfaces;

    while (head) {
        server_surface_t *surface = (server_surface_t *)head->data;
        if (surface->binding_context == egl_context &&
            surface->egl_surface == egl_surface) {
            //surface->locked_server == server) {
            /* has eglDestroySurface called ? */
            if (surface->mark_for_deletion == true) {
                _server_surface_remove (display, surface->egl_surface);
                return;
            }

            surface->locked_server = 0;
            surface->binding_context = EGL_NO_CONTEXT;
            surface->binding_buffer = 0;

            if (surface->native) {
                surface->native->locked_server = 0;
            }
            return;
        }
        head = head->next;
    }
}

server_surface_t *
_server_surface_find (server_display_list_t *display, EGLSurface egl_surface)
{
    link_list_t **surfaces = _server_surfaces (display);
    link_list_t *head = *surfaces;

    while (head) {
        server_surface_t *surface = (server_surface_t *)head->data;

        if (surface->egl_surface == egl_surface)
            return surface;
        head = head->next;
    }
    return NULL;
}

/*****************************************************************
 * egl context functions 
 *****************************************************************/
/* called by out-of-order eglCreateContext, set initial ref_count = 1
 */
server_context_t *
_server_context_create (EGLContext egl_context,
                        server_context_t *share_context)
{
    server_context_t *context = (server_context_t *)malloc (sizeof (server_context_t));

    context->egl_context = egl_context;
    context->mark_for_deletion = false;
    context->share_context = share_context;
    context->locked_server = 0;
    context->lock_servers = NULL;

    return context;
}

/* called by out-of-order eglCreateContext 
 */
void
_server_context_add (server_display_list_t *display,
                     EGLContext egl_context,
                     EGLContext egl_share_context)
{
    server_context_t *share_context = NULL;
    server_context_t *context = NULL;
    server_context_group_t *group = NULL;

    if (egl_context == egl_share_context)
        egl_share_context = EGL_NO_CONTEXT;

    if (egl_share_context != EGL_NO_CONTEXT) {
        group = _find_context_group (display, egl_share_context);
        if (group) {
            context = _find_context (group, egl_context);
            if (context)
                return;
            context = _server_context_create (egl_context,
                                              egl_share_context);
            _add_context (display, context);
            return;
        }
     }
     else {
        group = _find_context_group (display, egl_context);
        if (group)
            return;

        context = _server_context_create (egl_context,
                                          egl_share_context);
        _add_context (display, context);
    }
}

/* mark deletion, called by out-of-order eglDestroyContext */
void
_server_context_mark_for_deletion (server_display_list_t *display,
                                   EGLContext egl_context)
{
    server_context_group_t *group = _find_context_group (display, egl_context);
    if (! group)
        return;

    server_context_t *context = _find_context (group, egl_context);
    if (! context)
        return;

    context->mark_for_deletion = true;
}

/* remove an EGLContext, called by in-order eglDestroyContext */
void
_server_context_remove (server_display_list_t *display, EGLContext egl_context)
{
    server_context_group_t *group = _find_context_group (display, egl_context);
    if (! group)
        return;

    server_context_t *context = _find_context (group, egl_context);
    if (! context)
        return;

    thread_t thread = 0;
    if (context->lock_servers) {
        thread_t *t = (thread_t *) context->lock_servers->data;
        thread = *t;
    }
    if (context->locked_server == 0 &&
        thread == 0 &&
        context->mark_for_deletion == true)
        link_list_delete_first_entry_matching_data (&group->contexts, context);

    if (! group->contexts)
        _server_context_group_remove (display, group);
}

/*****************************************************************
 * eglMakeCurrent functions
 *****************************************************************/

void
_surface_make_current (server_display_list_t *display,
                       EGLSurface egl_surface,
                       bool switch_out,
                       thread_t server)
{
    server_surface_t *surface = _server_surface_find (display, egl_surface);
    if (! surface)
        return;

    if (surface->mark_for_deletion == true)
        return;

    int real_server = 1;
    if (switch_out)
        real_server = 0;

    surface->lock_status += real_server;
}

/* add 0 to lock_servers for a surface if switch to none, called by
 * out-of-order eglMakeCurrent
 */
void
_server_surface_make_current (server_display_list_t *display,
                              EGLSurface egl_surface,
                              thread_t server)
{
    server_surface_t *surface = _server_surface_find (display, egl_surface);
    if (! surface)
        return;

    if (surface->mark_for_deletion == true)
        return;

    if (surface->lock_visited == true)
        return;

    if (surface->lock_status == 0) {
        thread_t *thread_ptr = (thread_t *)malloc (sizeof (thread_t));
        *thread_ptr = 0;
        link_list_append (&surface->lock_servers, thread_ptr, free);
        if (surface->native) {
            thread_t *native_thread_ptr = (thread_t *)malloc (sizeof (thread_t));
            *native_thread_ptr = 0;
            link_list_append (&surface->native->lock_servers, native_thread_ptr, free);
        }
    }
    else {
        thread_t *thread_ptr = (thread_t *)malloc (sizeof (thread_t));
        *thread_ptr = server;
        link_list_append (&surface->lock_servers, thread_ptr, free);
        if (surface->native) {
            thread_t *native_thread_ptr = (thread_t *)malloc (sizeof (thread_t));
            *native_thread_ptr = server;
            link_list_append (&surface->native->lock_servers, native_thread_ptr, free);
        }
    }
    surface->lock_visited = true;
    surface->lock_status = 0;
}
            
void
_server_context_make_current (server_display_list_t *display,
                              EGLContext egl_context,
                              bool switch_out,
                              thread_t server)
{
    thread_t real_server = server;
    if (switch_out)
        real_server = 0;
    
    if (egl_context == EGL_NO_CONTEXT)
        return;

    server_context_group_t *group = _find_context_group (display, egl_context);
    if (! group)
        return;

    link_list_t *head = group->contexts;
    while (head) {
        server_context_t *context = (server_context_t *) head->data;

        thread_t *thread_ptr = (thread_t *) malloc (sizeof (thread_t));
        *thread_ptr = server;
        link_list_append (&context->lock_servers, thread_ptr, free);

        head = head->next;
    }
}

void
_server_image_make_current (server_display_list_t *display,
                            server_context_group_t *group,
                            server_image_t *image,
                            bool switch_out,
                            thread_t server)
{
    thread_t real_server = server;
    if (switch_out)
        real_server = 0;

    link_list_t *head = group->contexts;
    while (head) {
        server_context_t *context = (server_context_t *)head->data;

        if (image->binding_context == context->egl_context) {
            thread_t *thread_ptr = (thread_t *)malloc (sizeof (thread_t));
            *thread_ptr = real_server;
            link_list_append (&image->lock_servers, thread_ptr, free);

            if (image->native) {
                thread_t *native_thread_ptr = (thread_t *)malloc (sizeof (thread_t));
                *native_thread_ptr = real_server;
                link_list_append (&image->native->lock_servers, native_thread_ptr, free);
            }
            return;
        }
        head = head->next;
    }
}

void
_server_bound_surface_make_current (server_display_list_t *display,
                                    server_context_group_t *group,
                                    server_surface_t *surface,
                                    bool switch_out,
                                    thread_t server)
{
    thread_t real_server = 1;
    if (switch_out)
        real_server = 0;

    link_list_t *head = group->contexts;
    while (head) {
        server_context_t *context = (server_context_t *)head->data;

        if (surface->type == PBUFFER_SURFACE &&
            surface->binding_context == context->egl_context) {
            surface->lock_status += real_server;
            return;
        }
        head = head->next;
    }
}

void
_change_surfaces_locking (server_display_list_t *display,
                           EGLSurface egl_draw,
                           EGLSurface egl_read,
                           bool lock,
                           thread_t server)
{
    link_list_t **images;
    link_list_t **surfaces;
    link_list_t *head;

    if (lock == false) {
        _surface_make_current (display, egl_draw,
                               true, server);
        _surface_make_current (display, egl_read,
                               true, server);
    }
    else {
        _surface_make_current (display, egl_draw,
                               false, server);
        _surface_make_current (display, egl_read,
                               false, server);
    }
}

void
_change_resources_locking (server_display_list_t *display,
                           EGLContext egl_context,
                           EGLSurface egl_draw,
                           EGLSurface egl_read,
                           bool lock,
                           thread_t server)
{
    link_list_t **images;
    link_list_t **surfaces;
    link_list_t *head;
    server_context_group_t *group;

    if (lock == false) {
        /* release current context */
        _server_context_make_current (display, egl_context,
                                      true, server);
        _change_surfaces_locking (display, egl_draw, egl_read, 
                                  lock, server);
        group = _find_context_group (display, egl_context);

        if (group) {
            /* find images belong to current context */
            images = _server_images (display);
            head = *images;
            while (head) {
                server_image_t *image = (server_image_t *) head->data;
               _server_image_make_current (display, group, image,
                                           true, server); 
                head = head->next;
            }
            /* find any surfaces bounded to current context */
            surfaces = _server_surfaces (display);
            head = *surfaces;
            while (head) {
                server_surface_t *surface = (server_surface_t *)head->data;
                if (surface->mark_for_deletion == false && 
                    surface->egl_surface != egl_draw &&
                    surface->egl_surface != egl_read) 
                    _server_bound_surface_make_current (display,
                                                        group, surface,
                                                        true, server);
 
                head = head->next;
            }
        }
    }
    else {
        /* add context */
        _server_context_make_current (display, egl_context,
                                      false, server);
        _change_surfaces_locking (display, egl_draw, egl_read, 
                                  lock, server);
        group = _find_context_group (display, egl_context);

        if (group) {
            /* find images belong to current context */
            images = _server_images (display);
            head = *images;
            while (head) {
                server_image_t *image = (server_image_t *) head->data;
               _server_image_make_current (display, group, image,
                                           false, server); 
                head = head->next;
            }
            /* find any surfaces bounded to current context */
            surfaces = _server_surfaces (display);
            head = *surfaces;
            while (head) {
                server_surface_t *surface = (server_surface_t *)head->data;
                if (surface->mark_for_deletion == false && 
                    surface->egl_surface != egl_draw &&
                    surface->egl_surface != egl_read) 
                    _server_bound_surface_make_current (display,
                                                        group, surface,
                                                        false, server);
 
                head = head->next;
            }
        }
    }
}
           
void
_server_out_of_order_make_current (EGLDisplay new_egl_display,
                                   EGLSurface new_egl_draw,
                                   EGLSurface new_egl_read,
                                   EGLContext new_egl_context,
                                   EGLDisplay cur_egl_display,
                                   EGLSurface cur_egl_draw,
                                   EGLSurface cur_egl_read,
                                   EGLContext cur_egl_context,
                                   thread_t server)
{
    link_list_t **images;
    link_list_t **surfaces;
    link_list_t *head;

    if ((new_egl_display == EGL_NO_DISPLAY ||
         new_egl_context == EGL_NO_CONTEXT) &&
        (cur_egl_display == EGL_NO_DISPLAY ||
         cur_egl_context == EGL_NO_CONTEXT))
        return;
    server_display_list_t *new_display = NULL;
    server_display_list_t *cur_display = NULL;

    if (new_egl_display != EGL_NO_DISPLAY) {
        new_display = _server_display_find (new_egl_display);
        cur_display = new_display;
    }
    if (cur_egl_display != new_egl_display &&
        cur_egl_display != EGL_NO_DISPLAY)
        cur_display = _server_display_find (cur_egl_display);

    server_context_group_t *new_group = NULL;
    server_context_group_t *cur_group = NULL;
    server_context_t *new_context = NULL;
    server_context_t *cur_context = NULL;

    server_surface_t *new_draw = NULL;
    server_surface_t *cur_draw = NULL;
    server_surface_t *new_read = NULL;
    server_surface_t *cur_read = NULL;

    if (new_egl_context != EGL_NO_CONTEXT &&
        new_display) {
        new_group = _find_context_group (new_display, new_egl_context);
        new_context = _find_context (new_group, new_egl_context);
    }
    if (cur_egl_context != EGL_NO_CONTEXT &&
        cur_display) { 
        cur_group = _find_context_group (cur_display, cur_egl_context);
        cur_context = _find_context (cur_group, cur_egl_context);
    }

    if (new_egl_draw != EGL_NO_SURFACE &&
        new_display)
        new_draw = _server_surface_find (new_display, new_egl_draw);
    if (new_egl_read != EGL_NO_SURFACE &&
        new_display)
        new_read = _server_surface_find (new_display, new_egl_read);
    if (cur_egl_draw != EGL_NO_SURFACE &&
        cur_display)
        cur_draw = _server_surface_find (cur_display, cur_egl_draw);
    if (cur_egl_read != EGL_NO_SURFACE &&
        cur_display)
        cur_read = _server_surface_find (cur_display, cur_egl_read);

    server_context_group_t *group = NULL;

    /* we are switching from no context to a context */
    if (! cur_display && ! new_display)
        return;
    else if (! cur_display && new_display) {
        if (! new_context)
            return;
        /* add to context */
        _change_resources_locking (new_display, new_egl_context,
                                   new_egl_draw, new_egl_read,
                                   true, server);
    }
    /* switch from a context to none */
    else if (!new_display && cur_display) {
        if (! cur_context)
            return;
        /* release current context */
        _change_resources_locking (cur_display, cur_egl_context,
                                   cur_egl_draw, cur_egl_read,
                                   false, server);
    }
    /* both valid, but not same */
    else if (new_display != cur_display) {
        /* release current context */
        _change_resources_locking (cur_display, cur_egl_context,
                                   cur_egl_draw, cur_egl_read,
                                   false, server);
        /* add locking */
        _change_resources_locking (new_display, new_egl_context,
                                   new_egl_draw, new_egl_read,
                                   true, server);
    }            
    /* displays are same and valid*/
    else {
        if (! new_context && ! cur_context)
            return;
        /* switching from no to a context */
        if (new_context && ! cur_context) {            
            /* add to context */
            _change_resources_locking (new_display, new_egl_context,
                                       new_egl_draw, new_egl_read,
                                       true, server);
        }
        /* switch from a context to no */
        else if (! new_context && cur_context) {
            /* release current context */
            _change_resources_locking (cur_display, cur_egl_context,
                                       cur_egl_draw, cur_egl_read,
                                       false, server);
        }
        /* switch from one context to another context */
        else if (new_context != cur_context) {
            /* release current context */
            _change_resources_locking (cur_display, cur_egl_context,
                                       cur_egl_draw, cur_egl_read,
                                       false, server);
            /* add locking */
            _change_resources_locking (new_display, new_egl_context,
                                       new_egl_draw, new_egl_read,
                                       true, server);
        }
        /* contexts and displays are same */
        else {
            thread_t *thread_ptr = (thread_t *)malloc (sizeof (thread_t));
            *thread_ptr = server;
            link_list_append (&cur_context->lock_servers, thread_ptr, free);
            /* release current surfaces */
            _change_surfaces_locking (cur_display, cur_egl_draw,
                                       cur_egl_read, false, server);
            /* add locking */
            _change_surfaces_locking (new_display, new_egl_draw,
                                       new_egl_read, false, server);
        }
    }

    /* we have done all lockings, we need to append lock to all
     * changed surfaces
     */
    /* first current draw/read */
    if (cur_display) {
        _server_surface_make_current (cur_display, cur_egl_draw, server);
        _server_surface_make_current (cur_display, cur_egl_read, server);

        surfaces = _server_surfaces (cur_display);
        head = *surfaces;
        while (head) {
            server_surface_t *surface = (server_surface_t *)head->data;
            _server_surface_make_current (cur_display, surface->egl_surface, server);
            head = head->next;
        }
    }
    if (new_display) {
        _server_surface_make_current (new_display, new_egl_draw, server);
        _server_surface_make_current (new_display, new_egl_read, server);

        surfaces = _server_surfaces (new_display);
        head = *surfaces;
        while (head) {
            server_surface_t *surface = (server_surface_t *)head->data;
            _server_surface_make_current (new_display, surface->egl_surface, server);
            head = head->next;
        }
    }

    /* updated all surfaces */
    if (cur_display) {
        surfaces = _server_surfaces (cur_display);
        head = *surfaces;
        while (head) {
            server_surface_t *surface = (server_surface_t *)head->data;
            if (surface->lock_visited == true)
                surface->lock_visited = false;
            head = head->next;
        }
    }
    if (new_display) {
        surfaces = _server_surfaces (new_display);
        head = *surfaces;
        while (head) {
            server_surface_t *surface = (server_surface_t *)head->data;
            if (surface->lock_visited == true)
                surface->lock_visited = false;
            head = head->next;
        }
    }
}
/* called by in-order eglMakeCurrent.  There are two steps
 * first step, for each context and its sibling context in the share
 * group, if any of the context is locked, return SERVER_WAIT
 * for surface, for each of read/write surface, if it is locked, return
 * SERVER_WAIT, if surface->native is locked, return SERVER_WAIT
 * for each image, if it belongs to a context in the share group, if it
 * is locked, return SERVER_WAIT, if image->native is locked, return
 * SERVER_WAIT,
 * for each bound server in a context in the share group, if it is locked
 * return SERVER_WAIT
 * second step - if any of surfaces contexts, images locked_server == 0
 * and mark_for_deletion == true, and lock_servers first entry == 0
 * remove, reduce ref_count on native, if native ref_count == 0, remove
 * native
 */     
server_response_type_t
_server_in_order_make_current_test (EGLDisplay new_egl_display,
                                    EGLSurface new_egl_draw,
                                    EGLSurface new_egl_read,
                                    EGLContext new_egl_context,
                                    EGLDisplay cur_egl_display,
                                    EGLSurface cur_egl_draw,
                                    EGLSurface cur_egl_read,
                                    EGLContext cur_egl_context,
                                    thread_t server)
{
    link_list_t **images;
    link_list_t **surfaces;
    link_list_t *head;

    if ((new_egl_display == EGL_NO_DISPLAY ||
         new_egl_context == EGL_NO_CONTEXT) &&
        (cur_egl_display == EGL_NO_DISPLAY ||
         cur_egl_context == EGL_NO_CONTEXT))
        return SERVER_TRUE;
    server_display_list_t *new_display = NULL;
    server_display_list_t *cur_display = NULL;

    if (new_egl_display != EGL_NO_DISPLAY) {
        new_display = _server_display_find (new_egl_display);
        cur_display = new_display;
    }
    if (cur_egl_display != new_egl_display &&
        cur_egl_display != EGL_NO_DISPLAY)
        cur_display = _server_display_find (cur_egl_display);

    server_context_group_t *new_group = NULL;
    server_context_group_t *cur_group = NULL;
    server_context_t *new_context = NULL;
    server_context_t *cur_context = NULL;

    server_surface_t *new_draw = NULL;
    server_surface_t *cur_draw = NULL;
    server_surface_t *new_read = NULL;
    server_surface_t *cur_read = NULL;

    if (new_egl_context != EGL_NO_CONTEXT &&
        new_display) {
        new_group = _find_context_group (new_display, new_egl_context);
        new_context = _find_context (new_group, new_egl_context);
    }
    if (cur_egl_context != EGL_NO_CONTEXT &&
        cur_display) { 
        cur_group = _find_context_group (cur_display, cur_egl_context);
        cur_context = _find_context (cur_group, cur_egl_context);
    }

    if (new_egl_draw != EGL_NO_SURFACE &&
        new_display)
        new_draw = _server_surface_find (new_display, new_egl_draw);
    if (new_egl_read != EGL_NO_SURFACE &&
        new_display)
        new_read = _server_surface_find (new_display, new_egl_read);
    if (cur_egl_draw != EGL_NO_SURFACE &&
        cur_display)
        cur_draw = _server_surface_find (cur_display, cur_egl_draw);
    if (cur_egl_read != EGL_NO_SURFACE &&
        cur_display)
        cur_read = _server_surface_find (cur_display, cur_egl_read);

    server_context_group_t *group = NULL;
    thread_t *locking_server;

    /* first step, check context */
    if (new_display && new_context) {
        if (new_context->locked_server != 0 &&
            new_context->locked_server != server)
            return SERVER_WAIT;
        
        if (new_context->lock_servers) { 
            locking_server = (thread_t *)new_context->lock_servers->data;
            if (*locking_server != server)
                return SERVER_WAIT;
        }
        else
            return SERVER_FALSE;

        /* read and draw surface */
        if (new_draw) {
            if (new_draw->locked_server != 0 &&
                new_draw->locked_server != server)
               return SERVER_WAIT;
   
            /* check native */
            if (new_draw->native) {
                if (new_draw->native->locked_server != 0 &&
                    new_draw->native->locked_server != server)
                    return SERVER_WAIT;
            }
       
            if (new_draw->lock_servers) { 
                locking_server = (thread_t *)new_draw->lock_servers->data;
                if (*locking_server != server)
                    return SERVER_WAIT;
            }
            else
                return SERVER_FALSE;

            if (new_draw->native && new_draw->native->lock_servers) {
                locking_server = (thread_t *)new_draw->native->lock_servers->data;
                if (*locking_server != server)
                    return SERVER_WAIT;
            }
            else if (new_draw->native && !new_draw->native->lock_servers)
                return SERVER_FALSE;
        }
        if (new_read) {
            if (new_read->locked_server != 0 &&
                new_read->locked_server != server)
               return SERVER_WAIT;
   
            /* check native */
            if (new_read->native) {
                if (new_read->native->locked_server != 0 &&
                    new_read->native->locked_server != server)
                    return SERVER_WAIT;
            }
            
            if (new_read->lock_servers) { 
                locking_server = (thread_t *)new_read->lock_servers->data;
                if (*locking_server != server)
                    return SERVER_WAIT;
            }
            else
               return SERVER_FALSE;

            if (new_read->native && new_read->native->lock_servers) {
                locking_server = (thread_t *)new_read->native->lock_servers->data;
                if (*locking_server != server)
                    return SERVER_WAIT;
            }
            else if (new_read->native && ! new_read->native->lock_servers)
                return SERVER_FALSE;
        }

        /* check bounded surface */
        surfaces = _server_surfaces (new_display);
        head = *surfaces;
        while (head) {
            server_surface_t *surface = (server_surface_t *) head->data;
            if (surface->binding_context == new_egl_context) {
                if (surface->locked_server != 0 &&
                    surface->locked_server != server)
                    return SERVER_WAIT;

                if (surface->lock_servers) {
                    locking_server = (thread_t *)surface->lock_servers->data;
                    if (*locking_server != server)
                        return SERVER_WAIT;
                }
                else
                    return SERVER_FALSE;

                if (surface->native && surface->native->lock_servers) {
                    locking_server = (thread_t *)surface->native->lock_servers->data;
                    if (*locking_server != server)
                        return SERVER_WAIT;
                }
                else if (surface->native && ! surface->native->lock_servers)
                    return SERVER_FALSE;
            }
            head = head->next;
        }
        /* check image */
        images = _server_images (new_display);
        head = *images;
        while (head) {
            server_image_t *image = (server_image_t *) head->data;
            if (image->binding_context == new_egl_context) {
                if (image->locked_server != 0 &&
                    image->locked_server != server)
                    return SERVER_WAIT;
                /* check native */
                if (image->native &&
                    image->native->locked_server != 0 &&
                    image->native->locked_server != server)
                    return SERVER_WAIT;
                
                if (image->lock_servers) {
                    locking_server = (thread_t *)image->lock_servers->data;
                    if (*locking_server != server)
                        return SERVER_WAIT;
                }
                else
                    return SERVER_FALSE;

                if (image->native && image->native->lock_servers) {
                    locking_server = (thread_t *)image->native->lock_servers->data;
                    if (*locking_server != server)
                        return SERVER_WAIT;
                }
                else if (image->native && ! image->native->lock_servers)
                    return SERVER_FALSE;
            }
            head = head->next;
        }
    }

    /* let's check current resources */
    if (cur_display && cur_context) {
        if (! cur_context->lock_servers)
            return SERVER_FALSE;

        if (cur_draw) {
            if (! cur_draw->lock_servers)
                return SERVER_FALSE;
            if (cur_draw->native && ! cur_draw->native->lock_servers)
                return SERVER_FALSE;
        }
        if (cur_read) {
            if (! cur_read->lock_servers)
                return SERVER_FALSE;
            if (cur_read->native && ! cur_read->native->lock_servers)
                return SERVER_FALSE;
        }

        /* check bounded surface */
        surfaces = _server_surfaces (cur_display);
        head = *surfaces;
        while (head) {
            server_surface_t *surface = (server_surface_t *) head->data;
            if (surface->binding_context == cur_egl_context) {
                if (! surface->lock_servers)
                    return SERVER_FALSE;

                if (surface->native && ! surface->native->lock_servers)
                    return SERVER_FALSE;
            }
            head = head->next;
        }
        /* check image */
        images = _server_images (cur_display);
        head = *images;
        while (head) {
            server_image_t *image = (server_image_t *) head->data;
            if (image->binding_context == cur_egl_context) {
                if (! image->lock_servers)
                    return SERVER_FALSE;

                if (image->native && ! image->native->lock_servers)
                    return SERVER_FALSE;
            }
            head = head->next;
        }
    }
    return SERVER_TRUE;
}

void
_server_in_order_make_current (EGLDisplay new_egl_display,
                               EGLSurface new_egl_draw,
                               EGLSurface new_egl_read,
                               EGLContext new_egl_context,
                               EGLDisplay cur_egl_display,
                               EGLSurface cur_egl_draw,
                               EGLSurface cur_egl_read,
                               EGLContext cur_egl_context,
                               thread_t server)
{
    link_list_t **images;
    link_list_t **surfaces;
    link_list_t *head;

    server_context_group_t *group = NULL;
    
    if ((new_egl_display == EGL_NO_DISPLAY ||
         new_egl_context == EGL_NO_CONTEXT) &&
        (cur_egl_display == EGL_NO_DISPLAY ||
         cur_egl_context == EGL_NO_CONTEXT))
        return;

    server_display_list_t *new_display = NULL;
    server_display_list_t *cur_display = NULL;

    if (new_egl_display != EGL_NO_DISPLAY) {
        new_display = _server_display_find (new_egl_display);
        cur_display = new_display;
    }
    if (cur_egl_display != new_egl_display &&
        cur_egl_display != EGL_NO_DISPLAY)
        cur_display = _server_display_find (cur_egl_display);

    server_context_group_t *new_group = NULL;
    server_context_group_t *cur_group = NULL;
    server_context_t *new_context = NULL;
    server_context_t *cur_context = NULL;

    server_surface_t *new_draw = NULL;
    server_surface_t *cur_draw = NULL;
    server_surface_t *new_read = NULL;
    server_surface_t *cur_read = NULL;

    if (new_egl_context != EGL_NO_CONTEXT &&
        new_display) {
        new_group = _find_context_group (new_display, new_egl_context);
        new_context = _find_context (new_group, new_egl_context);
    }
    if (cur_egl_context != EGL_NO_CONTEXT &&
        cur_display) { 
        cur_group = _find_context_group (cur_display, cur_egl_context);
        cur_context = _find_context (cur_group, cur_egl_context);
    }

    if (new_egl_draw != EGL_NO_SURFACE &&
        new_display)
        new_draw = _server_surface_find (new_display, new_egl_draw);
    if (new_egl_read != EGL_NO_SURFACE &&
        new_display)
        new_read = _server_surface_find (new_display, new_egl_read);
    if (cur_egl_draw != EGL_NO_SURFACE &&
        cur_display)
        cur_draw = _server_surface_find (cur_display, cur_egl_draw);
    if (cur_egl_read != EGL_NO_SURFACE &&
        cur_display)
        cur_read = _server_surface_find (cur_display, cur_egl_read);

    thread_t *locking_server;

    /* we have done all checking, it is OK, we can preceed
     * first, we need to remove existing context, display, surface
     * if they have been mark for deletion
     */
    /* remove current draw */

    server_surface_t *touched_read = NULL;
    server_surface_t *touched_draw = NULL;

    if (cur_display) {
        if (cur_draw) {
            locking_server = (thread_t *) cur_draw->lock_servers->data;
            if (*locking_server == 0 && cur_draw->mark_for_deletion) {
                touched_draw = cur_draw;
                _server_surface_remove (cur_display, cur_egl_draw);
            }
        }
        if (cur_read != cur_draw && cur_read) {
            locking_server = (thread_t *) cur_read->lock_servers->data;
            if (*locking_server == 0 && cur_read->mark_for_deletion) {
                touched_read = cur_read;
                _server_surface_remove (cur_display, cur_egl_read);
            }
        }

        /* remove other images */
       if (cur_context) {
            images = _server_images (cur_display);
            head = *images;
            while (head) {
                server_image_t *image = (server_image_t *)head->data;
                locking_server = (thread_t *)image->lock_servers->data;
                if (*locking_server == 0 &&
                    image->mark_for_deletion == true &&
                    image->binding_context == cur_context->egl_context)
                    _server_image_remove (cur_display, image->egl_image);
                head = head->next;
            }
        }
        if (cur_context) {
            surfaces = _server_surfaces (cur_display);
            head = *surfaces;
            while (head) {
                server_surface_t *surface = (server_surface_t *)head->data;
                if (surface != touched_read && surface != touched_draw) {
                    locking_server = (thread_t *)surface->lock_servers->data;
                    if (*locking_server == 0 &&
                        surface->mark_for_deletion == true)
                        _server_surface_remove (cur_display, surface->egl_surface);
                }
                head = head->next;
            }
        }

        /* remove context ? */
        if (cur_context) {
            locking_server = (thread_t *)cur_context->lock_servers->data;
            if (cur_context->mark_for_deletion == true &&
                *locking_server == 0)
                _server_context_remove (cur_display, cur_context->egl_context);
        }

        /* remove display ?, before removing, we need to increase ref
         * for new_display
         */
        if (new_display)
            new_display->ref_count++;

        cur_display->ref_count--;

        if (cur_display->ref_count == 0)
            _server_display_remove (cur_display);
    }

    /* have done all removing, let's update locking status
     */
    if (cur_display && cur_context) {
        if (cur_draw) {
            locking_server = (thread_t *)cur_draw->lock_servers->data;
            if (*locking_server == 0) {
                link_list_delete_element (&cur_draw->lock_servers,
                                          cur_draw->lock_servers);
                cur_draw->locked_server = 0;

                if (cur_draw->native) {
                    cur_draw->native->locked_server = 0;
                    link_list_delete_element (&cur_draw->native->lock_servers,
                                              cur_draw->native->lock_servers);
                }
            }
        }
        if (cur_draw != cur_read && cur_read) {
            locking_server = (thread_t *)cur_read->lock_servers->data;
            if (*locking_server == 0) {
                link_list_delete_element (&cur_read->lock_servers,
                                          cur_read->lock_servers);
                cur_read->locked_server = 0;
                
                if (cur_read->native) {
                    cur_read->native->locked_server = 0;
                    link_list_delete_element (&cur_read->native->lock_servers,
                                              cur_read->native->lock_servers);
                }
            }
        }

        /* release all images */
        images = _server_images (cur_display);
        head = *images;

        while (head) {
            server_image_t *image = (server_image_t *)head->data;
            if (image->binding_context == cur_egl_context) {
                locking_server = (thread_t *)image->lock_servers->data;
                if (*locking_server == 0) {
                    link_list_delete_element (&image->lock_servers,
                                              image->lock_servers);
                    image->locked_server = 0;
                
                    if (image->native) {
                        image->native->locked_server = 0;
                        link_list_delete_element (&image->native->lock_servers,
                                                  image->native->lock_servers);
                    }
                }
            }
            head = head->next;
        }

        /* release all surface binding */
        surfaces = _server_surfaces (cur_display);
        head = *surfaces;

        while (head) {
            server_surface_t *surface = (server_surface_t *)head->data;
            if (surface->binding_context == cur_egl_context) {
                locking_server = (thread_t *)surface->lock_servers->data;
                if (*locking_server == 0) {
                    link_list_delete_element (&surface->lock_servers,
                                              surface->lock_servers);
                    surface->locked_server = 0;
                
                    if (surface->native) {
                        surface->native->locked_server = 0;
                        link_list_delete_element (&surface->native->lock_servers,
                                                  surface->native->lock_servers);
                    }
                }
            }
            head = head->next;
        }
    }

    if (new_display && new_context) {
        if (new_draw) {
            locking_server = (thread_t *)new_draw->lock_servers->data;
            if (*locking_server == server) {
                link_list_delete_element (&new_draw->lock_servers,
                                          new_draw->lock_servers);
                new_draw->locked_server = server;

                if (new_draw->native) {
                    new_draw->native->locked_server = server;
                    link_list_delete_element (&new_draw->native->lock_servers,
                                              new_draw->native->lock_servers);
                }
            }
        }
        if (new_draw != new_read && new_read) {
            locking_server = (thread_t *)new_read->lock_servers->data;
            if (*locking_server == server) {
                link_list_delete_element (&new_read->lock_servers,
                                          new_read->lock_servers);
                new_read->locked_server = server;
                
                if (new_read->native) {
                    new_read->native->locked_server = server;
                    link_list_delete_element (&new_read->native->lock_servers,
                                              new_read->native->lock_servers);
                }
            }
        }

        images = _server_images (new_display);
        head = *images;

        while (head) {
            server_image_t *image = (server_image_t *)head->data;
            if (image->binding_context == new_egl_context) {
                locking_server = (thread_t *)image->lock_servers->data;
                if (*locking_server == server) {
                    link_list_delete_element (&image->lock_servers,
                                              image->lock_servers);
                    image->locked_server = server;
                
                    if (image->native) {
                        image->native->locked_server = server;
                        link_list_delete_element (&image->native->lock_servers,
                                                  image->native->lock_servers);
                    }
                }
            }
            head = head->next;
        }

        surfaces = _server_surfaces (new_display);
        head = *surfaces;

        while (head) {
            server_surface_t *surface = (server_surface_t *)head->data;
            if (surface->binding_context == new_egl_context) {
                locking_server = (thread_t *)surface->lock_servers->data;
                if (*locking_server == server) {
                    link_list_delete_element (&surface->lock_servers,
                                              surface->lock_servers);
                    surface->locked_server = server;
                
                    if (surface->native) {
                        surface->native->locked_server = server;
                        link_list_delete_element (&surface->native->lock_servers,
                                                  surface->native->lock_servers);
                    }
                }
            }
            head = head->next;
        }
    }
}
