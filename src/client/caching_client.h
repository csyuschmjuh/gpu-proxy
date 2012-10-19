#ifndef CACHING_CLIENT_H
#define CACHING_CLIENT_H

#include "client.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the commands in the buffer using the following sequence
 * of calls:
 *   COMMAND_NAME_t *command =
 *       COMMAND_NAME_T(client_get_space_for_command (command_type));
 *   COMMAND_NAME_initialize (command, parameter1, parameter2, ...);
 *   client_write_command (command);
 */
typedef struct caching_client {
    client_t base;
} caching_client_t;

private client_t *
caching_client_new ();

#ifdef __cplusplus
}
#endif

#endif /* CACHING_CLIENT_H */
