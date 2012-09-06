
#ifndef GPUPROCESS_COMMAND_BUFFER_SERVICE_H
#define GPUPROCESS_COMMAND_BUFFER_SERVICE_H

#include <pthread.h>

#include "gpuprocess_compiler_private.h"
#include "gpuprocess_command.h"
#include "gpuprocess_egl_server_private.h"
#include "gpuprocess_ring_buffer.h"
#include "gpuprocess_types_private.h"
#include "gpuprocess_thread_private.h"
#include "gpuprocess_egl_states.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct command_buffer_service {
    buffer_t *buffer;
    /* FIXME: do we this?  We have thread local variable for client thread
     * in gpuprocess_egl_api.c.
     * We also have thread local variable for server thread in 
     * gpuprocess_egl_server.c and gpuprocess_gles2_server.c.
     *
     * both variable are kept as same value in
     * gpuprocess_egl_api.c - eglMakeCurrent () and eglReleaseThread () on 
     * the client side, and
     * gpuprocess_egl_server.c - _egl_make_current () and 
     * _egl_release_thread ()
     */
    v_link_list_t *active_state;
    /* FIXME: Create a wrapper to avoid thread dependency. */
    gpuprocess_thread_t *thread;
} command_buffer_service_t;

gpuprocess_private command_buffer_service_t *
command_buffer_service_initialize ();

gpuprocess_private bool
command_buffer_service_destroy (command_buffer_service_t *command_buffer_service);

gpuprocess_private void
command_buffer_service_set_active_state (command_buffer_service_t *command_buffer_service,
                                         v_link_list_t *active_state);

#ifdef __cplusplus
}
#endif

#endif /* GPUPROCESS_COMMAND_BUFFER_SERVICE_H */

