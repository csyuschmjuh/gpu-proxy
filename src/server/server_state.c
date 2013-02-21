#include "server_state.h"
#include "thread_private.h"
#include "types_private.h"
#include <stdlib.h>
#include <stdio.h>

/*****************************************************************
 * static functions 
 *****************************************************************/
/* get cached server side server_display_t list.  It is static such
 * that all servers share it
 */
static link_list_t **
_call_order_list ()
{
    static link_list_t *call_list = NULL;
    return &call_list;
}

link_list_t **
_server_displays ()
{
    static link_list_t *dpys = NULL;
    return &dpys;
}

/* destroy server_display_list_t, called by link_list_delete_element */
static void
_destroy_display (void *abstract_display)
{
    server_display_list_t *dpy = (server_display_list_t *) abstract_display;

    XCloseDisplay (dpy->server_display);

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
            server_dpy->mark_for_deletion == false) {
            server_dpy->ref_count++;
            return;
        }
 
        head = head->next;
    }

    server_dpy = _server_display_create (server_display, client_display, egl_display);
    link_list_append (dpys, (void *)server_dpy, _destroy_display);
}

/* remove a server_display from cache, called by in-order eglTerminate 
 * and by in-order eglMakeCurrent */
void
_server_display_remove (EGLDisplay egl_display)
{
    link_list_t **displays = _server_displays ();
    server_display_list_t *display = _server_display_find (egl_display);

    if (display) {
        if (display->ref_count > 1)
            display->ref_count--;

        if (display->ref_count == 0 &&
            display->mark_for_deletion == true)
            link_list_delete_first_entry_matching_data (displays, display);
    }
}

void
_server_display_reference (EGLDisplay egl_display)
{
    link_list_t **dpys = _server_displays ();
    server_display_list_t *server_dpy;
    link_list_t *head = *dpys;

    while (head) {
        server_dpy = (server_display_list_t *)head->data;

        if (server_dpy->egl_display == egl_display) {
            server_dpy->ref_count++;
            return;
        }
 
        head = head->next;
    }
}

/* mark display for removal, called by out-of-order eglTerminate */
void
_server_destroy_mark_for_deletion (server_display_list_t *display)
{
    link_list_t **displays = _server_displays ();

    if (! display)
        return;

    display->mark_for_deletion = true;
    if (display->ref_count > 1)
        display->ref_count--;

    if (display->ref_count == 0)
        link_list_delete_first_entry_matching_data (displays, display);
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

/*******************************************************
 * functions for lock/release list
 *******************************************************/
void
_call_order_list_append (thread_t server, double timestamp)
{
    link_list_t **list = _call_order_list ();

    server_log_t *log = (server_log_t *) malloc (sizeof (server_log_t));
    log->server = server;
    log->timestamp = timestamp;
    //printf ("append %ld, %0.5f\n", server, timestamp);
    link_list_append (list, log, free);
}

void
_call_order_list_remove ()
{
    link_list_t **list = _call_order_list ();

    if (*list != NULL) {
        //server_log_t *log = (server_log_t *)(*list)->data;
        //printf ("remove %ld, %0.5f\n", log->server, log->timestamp);
        link_list_delete_element (list, *list);
    }
}

bool
_call_order_list_head_is_server (thread_t server, double timestamp)
{
    link_list_t **list = _call_order_list ();

    if (*list) {
        server_log_t *log = (server_log_t *)(*list)->data;
        //printf ("compare %ld, %0.5f vs %ld, %0.5f\n", log->server, log->timestamp, server, timestamp);
        if (log->server == server &&
            log->timestamp == timestamp)
            return true;
    }

    return false;
}

/* release/lock functions */
void
_server_append_call_log (thread_t server, double timestamp)
{
    _call_order_list_append (server, timestamp);
}

bool
_server_allow_call (thread_t server, double timestamp)
{
    return _call_order_list_head_is_server (server, timestamp);
}

void
_server_remove_call_log ()
{
    _call_order_list_remove ();
}
    
