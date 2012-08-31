
#include "gpuprocess_command_buffer_service.h"

gpu_mutex_static_init (service_thread_started_mutex);

static void *
service_thread_func (void *ptr)
{
    /* This signals the producer thread to start producing. */
    gpu_mutex_unlock (service_thread_started_mutex);

    /* FIXME: initialize GL state and start consuming commands in the loop. */
    while (1) {}
}

command_buffer_service_t *
command_buffer_service_initialize(buffer_t *buffer)
{
    command_buffer_service_t *command_buffer_service;

    command_buffer_service = (command_buffer_service_t *)malloc(sizeof(command_buffer_service_t));
    command_buffer_service->buffer = buffer;

    /* We use a mutex here to wait until the thread has started. */
    gpu_mutex_lock (service_thread_started_mutex);
    pthread_create(command_buffer_service->thread, NULL, service_thread_func, NULL);
    /* FIXME:  Alex - is it a bug here to lock again without unlock ? */
    gpu_mutex_lock(service_thread_started_mutex);
    gpu_mutex_unlock (service_thread_started_mutex);

    return command_buffer_service;
}

bool
command_buffer_service_destroy(command_buffer_service_t *command_buffer_service)
{
    command_buffer_service->buffer = NULL;
    free(command_buffer_service);

    pthread_join(*command_buffer_service->thread, NULL);
    /* FIXME: GL termination. */

    return true;
}

void
command_buffer_service_set_active_state (
                           command_buffer_service_t *command_buffer_service,
                           v_link_list_t            *active_state)
{
    command_buffer_service->active_state = active_state;
}
