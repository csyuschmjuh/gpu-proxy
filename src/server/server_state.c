#include "server_state.h"
#include "thread_private.h"
#include <stdlib.h>
#include <stdio.h>

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
_server_eglimage_create (EGLDisplay display, EGLImageKHR image, EGLSurface surface)
{
    server_eglimage_t *eglimage = (server_eglimage_t *) malloc (sizeof (server_eglimage_t));

    eglimage->display = display;
    eglimage->image = image;
    eglimage->surface = surface;
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
_server_eglimage_add (EGLDisplay display, EGLImageKHR image, EGLSurface surface)
{
    link_list_t **eglimages = _server_eglimages ();
    server_eglimage_t *eglimage;
    link_list_t *head = *eglimages;

    while (head) {
        eglimage = (server_eglimage_t *)head->data;

        if (eglimage->display == display &&
            eglimage->image == image &&
            eglimage->surface == surface)
            return;

        head = head->next;
    }
    eglimage = _server_eglimage_create (display, image, surface);
    link_list_append (eglimages, (void *) eglimage, _destroy_eglimage);
}

void
_destroy_eglimage (void *abstract_image)
{
    free (abstract_image);
}
