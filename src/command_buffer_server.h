
#ifndef GPUPROCESS_COMMAND_BUFFER_SERVER_H
#define GPUPROCESS_COMMAND_BUFFER_SERVER_H

#include <pthread.h>

#include "compiler_private.h"
#include "command.h"
#include "egl_server_private.h"
#include "ring_buffer.h"
#include "types_private.h"
#include "thread_private.h"
#include "egl_states.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct command_buffer_server {
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
    v_link_list_t *active_state;
    /* FIXME: Create a wrapper to avoid thread dependency. */
    thread_t *thread;
} command_buffer_server_t;

private command_buffer_server_t *
command_buffer_server_initialize ();

private bool
command_buffer_server_destroy (command_buffer_server_t *command_buffer_server);

private void
command_buffer_server_set_active_state (command_buffer_server_t *command_buffer_server,
                                        v_link_list_t *active_state);

#ifdef __cplusplus
}
#endif

#endif /* GPUPROCESS_COMMAND_BUFFER_SERVER_H */

