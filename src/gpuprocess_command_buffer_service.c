
#include "gpuprocess_command_buffer_service.h"
#include "gpuprocess_egl_server_private.h"
#include "gpuprocess_thread_private.h"

gpuprocess_mutex_static_init (service_thread_started_mutex);

static void
command_buffer_service_handle_set_token(command_buffer_service_t *command_buffer_service,
                                        command_set_token_t *command)
{
    command_buffer_service->buffer->last_token = command->token;
}

static void
command_buffer_service_handle_command(command_buffer_service_t *command_buffer_service,
                                      command_t *command)
{
    switch (command->id) {
        case COMMAND_NO_OP: break;
        case COMMAND_SET_TOKEN:
            command_buffer_service_handle_set_token(command_buffer_service,
                                                    (command_set_token_t *)command);
            break;
        }
}

static void *
service_thread_func (void *ptr)
{
    command_buffer_service_t *command_buffer_service = (command_buffer_service_t *)ptr;

    /* populate dispatch table, create global egl_states structures */
    _gpuprocess_server_init ();
    /* This signals the producer thread to start producing. */
    gpuprocess_mutex_unlock (service_thread_started_mutex);

    /* FIXME: initialize GL state and start consuming commands in the loop. */
    /* FIXME: add exit condition of the loop. */
    while (true) {
        size_t data_left_to_read;
        command_t *read_command = (command_t *) buffer_read_address (command_buffer_service->buffer,
                                                                     &data_left_to_read);

        // The buffer is empty, so wait until there's something to read.
        while (! read_command) {
            sleep_nanoseconds (100);
            read_command = (command_t *) buffer_read_address (command_buffer_service->buffer,
                                                              &data_left_to_read);
        }

        command_buffer_service_handle_command(command_buffer_service, read_command);

        buffer_read_advance (command_buffer_service->buffer, read_command->size);
    }
}

command_buffer_service_t *
command_buffer_service_initialize(buffer_t *buffer)
{
    command_buffer_service_t *command_buffer_service;

    command_buffer_service = (command_buffer_service_t *)malloc(sizeof(command_buffer_service_t));
    command_buffer_service->buffer = buffer;

    /* We use a mutex here to wait until the thread has started. */
    gpuprocess_mutex_lock (service_thread_started_mutex);
    pthread_create(command_buffer_service->thread, NULL, service_thread_func, command_buffer_service);
    gpuprocess_mutex_lock(service_thread_started_mutex);

    return command_buffer_service;
}

bool
command_buffer_service_destroy(command_buffer_service_t *command_buffer_service)
{
    command_buffer_service->buffer = NULL;
    free(command_buffer_service);

    pthread_join(*command_buffer_service->thread, NULL);
    /* FIXME: GL termination. */
    if (command_buffer_service->active_state) {
        _gpuprocess_server_remove_state (command_buffer_service->active_state);
        command_buffer_service->active_state = NULL;
    }

    return true;
}

void
command_buffer_service_set_active_state (
                           command_buffer_service_t *command_buffer_service,
                           v_link_list_t            *active_state)
{
    command_buffer_service->active_state = active_state;
}
