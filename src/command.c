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
        command_initialize_sizes (command_sizes);

        /* custom size initialization */
        command_sizes[COMMAND_REGISTERCONTEXT] = sizeof (command_registercontext_t);
        command_sizes[COMMAND_REGISTERSURFACE] = sizeof (command_registersurface_t);
        command_sizes[COMMAND_REGISTERIMAGE] = sizeof (command_registerimage_t);
        initialized = true;
    }

    return command_sizes[command_type];
}
