#include "config.h"
#include "server.h"

#include "ring_buffer.h"
#include "dispatch_table.h"
#include "thread_private.h"
#include "server_state.h"
#include <time.h>

__thread EGLDisplay current_display
    __attribute__(( tls_model ("initial-exec"))) = EGL_NO_DISPLAY;

__thread EGLContext current_context
    __attribute__(( tls_model ("initial-exec"))) = EGL_NO_CONTEXT;

__thread EGLSurface current_draw
    __attribute__(( tls_model ("initial-exec"))) = EGL_NO_SURFACE;

__thread EGLSurface current_read_
    __attribute__(( tls_model ("initial-exec"))) = EGL_NO_SURFACE;

mutex_static_init (shared_resources_mutex);

/* This method is auto-generated into server_autogen.c
 * and included at the end of this file. */
static void
server_fill_command_handler_table (server_t *server);

void
server_start_work_loop (server_t *server)
{
    while (true) {
        size_t data_left_to_read;
        command_t *read_command = (command_t *) buffer_read_address (server->buffer,
                                                                     &data_left_to_read);
        /* The buffer is empty, so wait until there's something to read. */
         /*int times_slept = 0;
         while (! read_command && times_slept < 20) {
             sleep_nanoseconds (500);
             read_command = (command_t *) buffer_read_address (server->buffer,
                                                           &data_left_to_read);
             times_slept++;
         }*/

        /* We ran out of hot cycles, try a more lackadaisical approach. */
        while (! read_command) {
            sem_wait (server->server_signal);
            read_command = (command_t *) buffer_read_address (server->buffer,
                                                              &data_left_to_read);
        }

        if (read_command->type == COMMAND_SHUTDOWN)
            break;

        server->handler_table[read_command->type](server, read_command);
        buffer_read_advance (server->buffer, read_command->size);

        if (read_command->token) {
            server->buffer->last_token = read_command->token;
            sem_post (server->client_signal);
        }
    }
}

server_t *
server_new (buffer_t *buffer)
{
    server_t *server = malloc (sizeof (server_t));
    server_init (server, buffer);
    return server;
}

static void
server_handle_no_op (server_t *server,
                     command_t *command)
{
    return;
}

mutex_static_init (name_mapping_mutex);
static HashTable *name_mapping = NULL;

static void
server_handle_glgenbuffers (server_t *server,
                                   command_t *abstract_command)
{
    INSTRUMENT();

    command_glgenbuffers_t *command =
        (command_glgenbuffers_t *)abstract_command;

    GLuint *server_buffers = (GLuint *)malloc (command->n * sizeof (GLuint));
    server->dispatch.glGenBuffers (server, command->n, server_buffers);

    int i;
    mutex_lock (name_mapping_mutex);
    for (i = 0; i < command->n; i++) {
        GLuint *data = (GLuint *)malloc (sizeof (GLuint));
        *data = server_buffers[i];
        hash_insert (name_mapping, command->buffers[i], data);
    }
    mutex_unlock (name_mapping_mutex);

    free (server_buffers);
    command_glgenbuffers_destroy_arguments (command);
}

static void
server_handle_gldeletebuffers (server_t *server, command_t *abstract_command)
{
    INSTRUMENT();

    command_gldeletebuffers_t *command =
        (command_gldeletebuffers_t *)abstract_command;

    int i;
    mutex_lock (name_mapping_mutex);
    for (i = 0; i < command->n; i++) {
        GLuint *entry = hash_take (name_mapping, command->buffers[i]);
        if (entry) {
            command->buffers[i] = *entry;
            free (entry);
        }
    }
    mutex_unlock (name_mapping_mutex);

    server->dispatch.glDeleteBuffers (server, command->n, command->buffers);

    command_gldeletebuffers_destroy_arguments (command);
}

static void
server_handle_glgenframebuffers (server_t *server, command_t *abstract_command)
{
    INSTRUMENT();

    command_glgenframebuffers_t *command =
        (command_glgenframebuffers_t *)abstract_command;

    GLuint *server_framebuffers = (GLuint *)malloc (command->n * sizeof (GLuint));
    server->dispatch.glGenFramebuffers (server, command->n, server_framebuffers);

    int i;
    mutex_lock (name_mapping_mutex);
    for (i = 0; i < command->n; i++) {
        GLuint *data = (GLuint *)malloc (sizeof (GLuint));
        *data = server_framebuffers[i];
        hash_insert (name_mapping, command->framebuffers[i], data);
    }
    mutex_unlock (name_mapping_mutex);

    free (server_framebuffers);
    command_glgenframebuffers_destroy_arguments (command);
}

static void
server_handle_gldeleteframebuffers (server_t *server, command_t *abstract_command)
{
    INSTRUMENT();

    command_gldeleteframebuffers_t *command =
        (command_gldeleteframebuffers_t *)abstract_command;

    int i;
    mutex_lock (name_mapping_mutex);
    for (i = 0; i < command->n; i++) {
        GLuint *entry = hash_take (name_mapping, command->framebuffers[i]);
        if (entry) {
            command->framebuffers[i] = *entry;
            free (entry);
        }
    }
    mutex_unlock (name_mapping_mutex);

    server->dispatch.glDeleteFramebuffers (server, command->n, command->framebuffers);

    command_gldeleteframebuffers_destroy_arguments (command);
}

static void
server_handle_glgentextures (server_t *server, command_t *abstract_command)
{
    INSTRUMENT();

    command_glgentextures_t *command =
        (command_glgentextures_t *)abstract_command;

    GLuint *server_textures = (GLuint *)malloc (command->n * sizeof (GLuint));
    server->dispatch.glGenTextures (server, command->n, server_textures);

    int i;
    mutex_lock (name_mapping_mutex);
    for (i = 0; i < command->n; i++) {
        GLuint *data = (GLuint *)malloc (sizeof (GLuint));
        *data = server_textures[i];
        hash_insert (name_mapping, command->textures[i], data);
    }
    mutex_unlock (name_mapping_mutex);

    free (server_textures);
    command_glgentextures_destroy_arguments (command);
}

static void
server_handle_gldeletetextures (server_t *server, command_t *abstract_command)
{
    INSTRUMENT();

    command_gldeletetextures_t *command =
        (command_gldeletetextures_t *)abstract_command;

    int i;
    mutex_lock (name_mapping_mutex);
    for (i = 0; i < command->n; i++) {
        GLuint *entry = hash_take (name_mapping, command->textures[i]);
        if (entry) {
            command->textures[i] = *entry;
            free (entry);
        }
    }
    mutex_unlock (name_mapping_mutex);

    server->dispatch.glDeleteTextures (server, command->n, command->textures);

    command_gldeletetextures_destroy_arguments (command);
}

static void
server_handle_glgenrenderbuffers (server_t *server,
                                         command_t *abstract_command)
{
    INSTRUMENT();

    command_glgenrenderbuffers_t *command =
        (command_glgenrenderbuffers_t *)abstract_command;

    GLuint *server_renderbuffers = (GLuint *)malloc (command->n * sizeof (GLuint));
    server->dispatch.glGenRenderbuffers (server, command->n, server_renderbuffers);

    int i;
    mutex_lock (name_mapping_mutex);
    for (i = 0; i < command->n; i++) {
        GLuint *data = (GLuint *)malloc (sizeof (GLuint));
        *data = server_renderbuffers[i];
        hash_insert (name_mapping, command->renderbuffers[i], data);
    }
    mutex_unlock (name_mapping_mutex);

    free (server_renderbuffers);
    command_glgenrenderbuffers_destroy_arguments (command);
}

static void
server_handle_gldeleterenderbuffers (server_t *server, command_t *abstract_command)
{
    INSTRUMENT();

    command_gldeleterenderbuffers_t *command =
        (command_gldeleterenderbuffers_t *)abstract_command;

    int i;
    mutex_lock (name_mapping_mutex);
    for (i = 0; i < command->n; i++) {
        GLuint *entry = hash_take (name_mapping, command->renderbuffers[i]);
        if (entry) {
            command->renderbuffers[i] = *entry;
            free (entry);
        }
    }
    mutex_unlock (name_mapping_mutex);

    server->dispatch.glDeleteBuffers (server, command->n, command->renderbuffers);
    command_gldeleterenderbuffers_destroy_arguments (command);
}

static void
server_handle_glcreateprogram (server_t *server, command_t *abstract_command)
{
    INSTRUMENT ();

    command_glcreateprogram_t *command =
            (command_glcreateprogram_t *)abstract_command;
    GLuint *program = (GLuint *)malloc (sizeof (GLuint));

    *program = server->dispatch.glCreateProgram (server);

    mutex_lock (name_mapping_mutex);
    hash_insert (name_mapping, command->result, program);
    mutex_unlock (name_mapping_mutex);

}

static void
server_handle_gldeleteprogram (server_t *server, command_t *abstract_command)
{
    INSTRUMENT ();

    command_gldeleteprogram_t *command =
            (command_gldeleteprogram_t *)abstract_command;

    mutex_lock (name_mapping_mutex);
    GLuint *program = hash_take (name_mapping, command->program);
    mutex_unlock (name_mapping_mutex);

    if (program)
        server->dispatch.glDeleteProgram (server, *program);

    command_gldeleteprogram_destroy_arguments (command);
}

static void
server_handle_glcreateshader (server_t *server, command_t *abstract_command)
{
    INSTRUMENT ();

    command_glcreateshader_t *command =
            (command_glcreateshader_t *)abstract_command;
    GLuint *shader = (GLuint *)malloc (sizeof (GLuint));

    *shader = server->dispatch.glCreateShader (server, command->type);

    mutex_lock (name_mapping_mutex);
    hash_insert (name_mapping, command->result, shader);
    mutex_unlock (name_mapping_mutex);
}

static void
server_handle_gldeleteshader (server_t *server, command_t *abstract_command)
{
    INSTRUMENT ();

    command_gldeleteshader_t *command =
            (command_gldeleteshader_t *)abstract_command;

    mutex_lock (name_mapping_mutex);
    GLuint *shader = hash_take (name_mapping, command->shader);
    mutex_unlock (name_mapping_mutex);

    if (shader)
        server->dispatch.glDeleteShader (server, *shader);
    else
        /*XXX: This call should return INVALID_VALUE */
        server->dispatch.glDeleteShader (server, 0xffffffff);

    command_gldeleteshader_destroy_arguments (command);
}

/* pilot server handles these out-of-order requests from all servers.
 * These requests must be handled when client requests it because they are
 * not tied to a specific context.  We must make theses APIs out-of-order
 * because it is possible that two threads runs concurrently, thread A
 * creates a context, and thread B can immediately swith on to the newly
 * created context.  If we make them in-order, then it might be case where
 * thread A server has not received create context command yet while thread
 * B has already received eglMakeCurrent on new context, which result in
 * error
 */
static void
out_of_order_server_handle_eglgetdisplay (server_t *server, command_t *abstract_command)
{
    INSTRUMENT ();

    command_eglgetdisplay_t *command =
        (command_eglgetdisplay_t *)abstract_command;

    /* open a separate display */
    Display *display = XOpenDisplay (NULL);
    if (display == NULL)
        command->result = EGL_NO_DISPLAY;

    command->result = server->dispatch.eglGetDisplay (server, display);
    if (command->result == EGL_NO_DISPLAY) {
        XCloseDisplay (display);
        return;
    }

    mutex_lock (shared_resources_mutex);
    _server_display_add (display, command->display_id, command->result);
    mutex_unlock (shared_resources_mutex);
}

static void
out_of_order_server_handle_eglinitialize (server_t *server, command_t *abstract_command)
{
    INSTRUMENT ();

    command_eglinitialize_t *command =
        (command_eglinitialize_t *)abstract_command;

    command->result = server->dispatch.eglInitialize (server, command->dpy, command->major, command->minor);
}

static void
out_of_order_server_handle_eglbindapi (server_t *server, command_t *abstract_command)
{
    INSTRUMENT ();
    command_eglbindapi_t *command =
            (command_eglbindapi_t *)abstract_command;
    command->result = server->dispatch.eglBindAPI (server, command->api);
}

static void
out_of_order_server_handle_eglcreatecontext (server_t *server,
                                             command_t *abstract_command)
{
    INSTRUMENT ();

    command_eglcreatecontext_t *command = 
        (command_eglcreatecontext_t *) abstract_command;

    command->result = server->dispatch.eglCreateContext (server,
                                                         command->dpy,
                                                         command->config,
                                                         command->share_context,
                                                         command->attrib_list);
    if (command->result != EGL_NO_CONTEXT &&
        command->share_context != EGL_NO_CONTEXT) {
        mutex_lock (shared_resources_mutex);
        /* create contexts in shared resources */
        _server_context_create (command->dpy, (EGLContext) command->result,
                                command->share_context);
        mutex_unlock (shared_resources_mutex);
    }
}

static void
out_of_order_server_handle_eglcreatewindowsurface (server_t *server,
                                                   command_t *abstract_command)
{
    INSTRUMENT ();

    command_eglcreatewindowsurface_t *command = 
        (command_eglcreatewindowsurface_t *) abstract_command;

    command->result = server->dispatch.eglCreateWindowSurface (server,
                                                               command->dpy,
                                                               command->config,
                                                               command->win,
                                                               command->attrib_list);

    if (command->result != EGL_NO_SURFACE) {
        mutex_lock (shared_resources_mutex);
        _server_surface_add (command->dpy, command->result,
                             (void *)command->win, WINDOW_SURFACE);
        mutex_unlock (shared_resources_mutex);
    }
}

static void
out_of_order_server_handle_eglcreatepixmapsurface (server_t *server, command_t *abstract_command)
{
    INSTRUMENT ();

    command_eglcreatepixmapsurface_t *command = 
        (command_eglcreatepixmapsurface_t *) abstract_command;

    command->result = server->dispatch.eglCreatePixmapSurface (server,
                                                               command->dpy,
                                                               command->config,
                                                               command->pixmap,
                                                               command->attrib_list);

    if (command->result != EGL_NO_SURFACE) {
        mutex_lock (shared_resources_mutex);
        _server_surfaces_add (command->dpy, command->result,
                              (void *)command->pixma, PIXMAP_SURFACE);
        mutex_unlock (shared_resources_mutex);
    }
}

static void
out_of_order_server_handle_eglcreatepbuffersurface (server_t *server, command_t *abstract_command)
{
    INSTRUMENT ();

    command_eglcreatepbuffersurface_t *command = 
        (command_eglcreatepbuffersurface_t *) abstract_command;

    command->result = server->dispatch.eglCreatePbufferSurface (server,
                                                                command->dpy,
                                                                command->config,
                                                                command->attrib_list);

    if (command->result != EGL_NO_SURFACE) {
        mutex_lock (shared_resources_mutex);
        _server_surfaces_add (command->dpy, command->result,
                              (void *)NULL, PBUFFER_SURFACE);
        mutex_unlock (shared_resources_mutex);
    }
}

static void
out_of_order_server_handle_eglcreatepbufferfromclientbuffer (
    server_t *server, command_t *abstract_command)
{
    INSTRUMENT ();
    command_eglcreatepbufferfromclientbuffer_t *command =
            (command_eglcreatepbufferfromclientbuffer_t *)abstract_command;
    command->result = server->dispatch.eglCreatePbufferFromClientBuffer (server, command->dpy, command->buftype, command->buffer, command->config, command->attrib_list);
    
    if (command->result != EGL_NO_SURFACE) {
        mutex_lock (shared_resources_mutex);
        _server_surfaces_add (command->dpy, command->result,
                              (void *)command->bufffer, PBUFFER_SURFACE);
        mutex_unlock (shared_resources_mutex);
    }
}

static void
out_of_order_server_handle_eglcreatepixmapsurfacehi (
    server_t *server, command_t *abstract_command)
{
    INSTRUMENT ();
    command_eglcreatepixmapsurfacehi_t *command =
            (command_eglcreatepixmapsurfacehi_t *)abstract_command;
    command->result = server->dispatch.eglCreatePixmapSurfaceHI (server, command->dpy, command->config, command->pixmap);
    
    if (command->result != EGL_NO_SURFACE) {
        mutex_lock (shared_resources_mutex);
        _server_surfaces_add (command->dpy, command->result,
                              (void *)command->pixmap, PIXMAP_SURFACE);
        mutex_unlock (shared_resources_mutex);
    }
}

static void
out_of_order_server_handle_eglcreateimagekhr (server_t *server, command_t *abstract_command)
{
    INSTRUMENT ();
    command_eglcreateimagekhr_t *command =
            (command_eglcreateimagekhr_t *)abstract_command;
    command->result = server->dispatch.eglCreateImageKHR (server, command->dpy, command->ctx, command->target, command->buffer, command->attrib_list);

    if (command->result != EGL_NO_IMAGE_KHR) {
        mutex_lock (shared_resources_mutex);
        _server_image_add (command->dpy, (EGLImageKHR) command->result, command->buffer);
        mutex_unlock (shared_resources_mutex);
    }
}

static void
out_of_order_server_handle_eglcreatedrmimagemesa (
    server_t *server, command_t *abstract_command)
{
    INSTRUMENT ();
    command_eglcreatedrmimagemesa_t *command =
            (command_eglcreatedrmimagemesa_t *)abstract_command;
    command->result = server->dispatch.eglCreateDRMImageMESA (server, command->dpy, command->attrib_list);
    
    if (command->result != EGL_NO_IMAGE_KHR) {
        mutex_lock (shared_resources_mutex);
        _server_image_add (command->dpy, (EGLImageKHR) command->result, NULL);
        mutex_unlock (shared_resources_mutex);
    }
}

static void
out_of_order_server_handle_egllocksurfacekhr (server_t *server, command_t *abstract_command)
{
    INSTRUMENT ();
    command_egllocksurfacekhr_t *command =
            (command_egllocksurfacekhr_t *)abstract_command;
    command->result = server->dispatch.eglLockSurfaceKHR (server, command->display, command->surface, command->attrib_list);
}

static void
out_of_order_server_handle_eglunlocksurfacekhr (
    server_t *server, command_t *abstract_command)
{
    INSTRUMENT ();
    command_eglunlocksurfacekhr_t *command =
            (command_eglunlocksurfacekhr_t *)abstract_command;
    command->result = server->dispatch.eglUnlockSurfaceKHR (server, command->display, command->surface);
}

/* server handles out-of-order request, mark for deletion for shared
 * resources
 */
static void
server_handle_eglterminate_request (server_t *server, command_t *abstract_command)
{
    INSTRUMENT ();

    command_eglterminate_t *command =
        (command_eglterminate_t *) abstract_command;

    mutex_lock (shared_resources_mutex);
    _server_display_mark_for_deletion (command->dpy);
    mutex_unlock (shared_resources_mutex);
    command->result = EGL_TRUE;
}

static void
server_handle_egldestroysurface_request (server_t *server, command_t *abstract_command)
{
    INSTRUMENT ();
    command_egldestroysurface_t *command =
        (command_egldestroysurface_t *)abstract_command;

    mutex_lock (shared_resources_mutex);
    _server_surface_mark_for_deletion (command->dpy, (EGLSurface) command->surface);
    mutex_unlock (shared_resources_mutex);
    command->result = EGL_TRUE;
}

static void
server_handle_egldestroycontext_request (server_t *server, command_t *abstract_command)
{
    INSTRUMENT ();
    command_egldestroycontext_t *command =
        (command_egldestroycontext_t *)abstract_command;

    mutex_lock (shared_resources_mutex);
    _server_context_mark_for_deletion (command->dpy, (EGLContext) command->ctx);
    mutex_unlock (shared_resources_mutex);
    command->result = EGL_TRUE;
}

/*
static void
server_handle_eglreleasethread_request (server_t *server, command_t *abstract_command)
{
    INSTRUMENT ();

    command_eglreleasethread *command =
        (command_eglreleasethreadrequest_t *)abstract_command;

    mutex_lock (shared_resources_mutex);
    _server_surface_unlock (command->dpy, command->draw, 0);
    if (command->draw != command->read)
        _server_surface_unlock (command->dpy, command->read, 0);
    _server_context_unlock (command->dpy, command->ctx, 0);
    _server_display_unreference (command->dpy);
    mutex_unlock (shared_resources_mutex);
    return EGL_TRUE;
}
*/

/* the out-of-order eglMakeCurrent is only sent if display, read, draw or
 * context is valid
 */
static void
server_handle_eglmakecurrent_reuquest (server_t *server, command_t *abstract_command)
{
    INSTRUMENT ();
    command_eglmakecurrent_t *command =
        (command_eglmakecurrent_t *)abstract_command;

    command->result = EGL_TRUE;
    if (command->dpy == EGL_NO_DISPLAY ||
        command->ctx == EGL_NO_CONTEXT) 
        return;
    
    mutex_lock (shared_resources_mutex);
    if (command->ctx != EGL_NO_CONTEXT)
        _server_context_lock (command->dpy, command->ctx); 


static void
server_handle_egldestroyimagekhr (server_t *server, command_t *abstract_command)
{
    INSTRUMENT ();
    command_egldestroyimagekhr_t *command = 
        (command_egldestroyimagekhr_t *)abstract_command;

    command->result = server->dispatch.eglDestroyImageKHR (server, command->dpy, command->image);

    if (command->result == EGL_TRUE) {
        mutex_lock (shared_resources_mutex);
        _server_eglimage_remove (command->dpy, (EGLImageKHR) command->image);
        mutex_unlock (shared_resources_mutex);
    }
}

void
server_init (server_t *server,
             buffer_t *buffer)
{
    server->buffer = buffer;
    server->dispatch = *dispatch_table_get_base();
    server->command_post_hook = NULL;

    server->handler_table[COMMAND_NO_OP] = server_handle_no_op;
    server_fill_command_handler_table (server);

    server->handler_table[COMMAND_GLGENBUFFERS] =
        server_handle_glgenbuffers;
    server->handler_table[COMMAND_GLDELETEBUFFERS] =
        server_handle_gldeletebuffers;
    server->handler_table[COMMAND_GLDELETEFRAMEBUFFERS] =
        server_handle_gldeleteframebuffers;
    server->handler_table[COMMAND_GLGENFRAMEBUFFERS] =
        server_handle_glgenframebuffers;
    server->handler_table[COMMAND_GLGENTEXTURES] =
        server_handle_glgentextures;
    server->handler_table[COMMAND_GLDELETETEXTURES] =
        server_handle_gldeletetextures;
    server->handler_table[COMMAND_GLGENRENDERBUFFERS] =
        server_handle_glgenrenderbuffers;
    server->handler_table[COMMAND_GLDELETERENDERBUFFERS] =
        server_handle_gldeleterenderbuffers;
    server->handler_table[COMMAND_GLCREATEPROGRAM] =
        server_handle_glcreateprogram;
    server->handler_table[COMMAND_GLDELETEPROGRAM] =
        server_handle_gldeleteprogram;
    server->handler_table[COMMAND_GLCREATESHADER] =
        server_handle_glcreateshader;
    server->handler_table[COMMAND_GLDELETESHADER] =
        server_handle_gldeleteshader;

    server->handler_table[COMMAND_EGLCREATECONTEXT] = 
        server_handle_eglcreatecontext;
    server->handler_table[COMMAND_EGLCREATEWINDOWSURFACE] = 
        server_handle_eglcreatewindowsurface;
    server->handler_table[COMMAND_EGLCREATEPIXMAPSURFACE] = 
        server_handle_eglcreatepixmapsurface;
    server->handler_table[COMMAND_EGLDESTROYSURFACE] = 
        server_handle_egldestroysurface;
    server->handler_table[COMMAND_EGLDESTROYCONTEXT] = 
        server_handle_egldestroycontext;
    server->handler_table[COMMAND_EGLTERMINATE] = 
        server_handle_eglterminate;
    server->handler_table[COMMAND_EGLGETDISPLAY] = 
        server_handle_eglgetdisplay;
    server->handler_table[COMMAND_EGLDESTROYIMAGEKHR] = 
        server_handle_egldestroyimagekhr;
    server->handler_table[COMMAND_EGLCREATEIMAGEKHR] = 
        server_handle_eglcreateimagekhr;

    mutex_lock (name_mapping_mutex);
    if (name_mapping) {
	mutex_unlock (name_mapping_mutex);
        return;
    }
    name_mapping = new_hash_table(free);
    mutex_unlock (name_mapping_mutex);
}

bool
server_destroy (server_t *server)
{
    free (server);
    return true;
}

#include "server_autogen.c"
