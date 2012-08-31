
#include "gpuprocess_command_buffer_service.h"

pthread_mutex_t service_thread_started_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *
service_thread_func (void *ptr)
{
    /* This signals the producer thread to start producing. */
    pthread_mutex_unlock (&service_thread_started_mutex);

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
    pthread_mutex_lock(&service_thread_started_mutex);
    pthread_create(command_buffer_service->thread, NULL, service_thread_func, NULL);
    pthread_mutex_lock(&service_thread_started_mutex);

    return command_buffer_service;
}

v_bool_t
command_buffer_service_destroy(command_buffer_service_t *command_buffer_service)
{
    command_buffer_service->buffer = NULL;
    free(command_buffer_service);

    pthread_join(*command_buffer_service->thread, NULL);
    /* FIXME: GL termination. */

    return TRUE;
}
