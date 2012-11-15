#ifndef CACHING_CLIENT_H
#define CACHING_CLIENT_H

#include "client.h"

/* Initialize the commands in the buffer using the following sequence
 * of calls:
 *   COMMAND_NAME_t *command =
 *       COMMAND_NAME_T(client_get_space_for_command (command_type));
 *   COMMAND_NAME_initialize (command, parameter1, parameter2, ...);
 *   client_write_command (command);
 */
typedef struct caching_client {
    client_t super;

    /* We maintain a copy of the superclass' dispatch table here, so
     * that we can chain up to the superclass. The process of subclassing
     * overrides the original dispatch table. */
    dispatch_table_t super_dispatch;
} caching_client_t;

private caching_client_t *
caching_client_new ();

private void
caching_client_destroy (caching_client_t *client);


#endif /* CACHING_CLIENT_H */
