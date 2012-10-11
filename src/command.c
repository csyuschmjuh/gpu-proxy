#include "config.h"
#include "command.h"
#include <stdbool.h>

void
command_set_token_init (command_set_token_t *command, unsigned int token)
{
    command->token = token;
}

size_t
command_get_size (command_type_t command_type)
{
    static size_t command_sizes[COMMAND_MAX_COMMAND];
    static bool initialized = false;
    if (!initialized) {
        command_sizes[COMMAND_NO_OP] = 0;
        command_sizes[COMMAND_SET_TOKEN] = sizeof (command_set_token_t);
        command_initialize_sizes (command_sizes);
        initialized = true;
    }

    return command_sizes[command_type];
}
