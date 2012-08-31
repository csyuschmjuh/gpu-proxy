
#ifndef GPUPROCESS_COMMAND_BUFFER_SERVICE_H
#define GPUPROCESS_COMMAND_BUFFER_SERVICE_H

#include <pthread.h>

#include "gpuprocess_command.h"
#include "gpuprocess_egl_server_private.h"
#include "gpuprocess_ring_buffer.h"
#include "gpuprocess_types_private.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct command_buffer_service {
    buffer_t *buffer;
    gl_server_states_t *states;
    /* FIXME: Create a wrapper to avoid thread dependency. */
    pthread_t *thread;
} command_buffer_service_t;

command_buffer_service_t *
command_buffer_service_initialize();
v_bool_t
command_buffer_service_destroy(command_buffer_service_t *command_buffer_service);

#ifdef __cplusplus
}
#endif

#endif /* GPUPROCESS_COMMAND_BUFFER_SERVICE_H */

