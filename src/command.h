#ifndef GPUPROCESS_COMMAND_H
#define GPUPROCESS_COMMAND_H

#include "compiler_private.h"
#include "types_private.h"
#include "thread_private.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

typedef enum command_type {
    COMMAND_NO_OP,
    COMMAND_LOG,
    COMMAND_SHUTDOWN,

#include "generated/command_types_autogen.h"

    COMMAND_MAX_COMMAND
} command_type_t;

typedef struct command {
    command_type_t type;
    size_t size;

    /* The token is used for making synchronous calls. */
    unsigned int token;

    long timestamp;
    bool use_timestamp;
    thread_t server_id;
} command_t;

private void
command_initialize_sizes (size_t* sizes);

private size_t
command_get_size (command_type_t command_type);

#include "command_custom.h"
#include "generated/command_autogen.h"

#endif /* GPUPROCESS_COMMAND_H */
