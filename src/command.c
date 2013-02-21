#include "config.h"
#include "command.h"
#include <stdbool.h>

size_t
command_get_size (command_type_t command_type)
{
    static size_t command_sizes[COMMAND_MAX_COMMAND];
    static bool initialized = false;
    if (!initialized) {
        command_sizes[COMMAND_NO_OP] = 0;
        command_sizes[COMMAND_SHUTDOWN] = sizeof (command_t);
        command_sizes[COMMAND_LOG] = sizeof (command_t);
        command_initialize_sizes (command_sizes);
        initialized = true;
    }

    return command_sizes[command_type];
}
