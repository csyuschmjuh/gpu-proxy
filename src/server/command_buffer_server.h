
#ifndef GPUPROCESS_COMMAND_BUFFER_SERVER_H
#define GPUPROCESS_COMMAND_BUFFER_SERVER_H

#include "command.h"
#include "compiler_private.h"
#include "dispatch_private.h"
#include "egl_states.h"
#include "ring_buffer.h"
#include "thread_private.h"
#include "types_private.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _command_buffer_server command_buffer_server_t;
typedef struct _command_buffer_server {
    server_dispatch_table_t dispatch;

    buffer_t *buffer;
    /* FIXME: do we this?  We have thread local variable for client thread
     * in egl_api.c.
     * We also have thread local variable for server thread in
     * egl_server.c and gles2_server.c.
     *
     * both variable are kept as same value in
     * egl_api.c - eglMakeCurrent () and eglReleaseThread () on
     * the client side, and
     * egl_server.c - _egl_make_current () and
     * _egl_release_thread ()
     */
    link_list_t *active_state;
    /* FIXME: Create a wrapper to avoid thread dependency. */
    thread_t thread;
    bool threaded;
} command_buffer_server_t;

private void
command_buffer_server_init (command_buffer_server_t *server,
                            buffer_t *buffer,
                            bool threaded);

private command_buffer_server_t *
command_buffer_server_new (buffer_t *buffer);

private bool
command_buffer_server_destroy (command_buffer_server_t *command_buffer_server);

private void
command_buffer_server_set_active_state (command_buffer_server_t *command_buffer_server,
                                        link_list_t *active_state);

#ifdef __cplusplus
}
#endif

#endif /* GPUPROCESS_COMMAND_BUFFER_SERVER_H */

