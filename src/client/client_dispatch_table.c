#include "config.h"
#include "client.h"
#include "types_private.h"
#include "thread_private.h"

#include "gles2_api_dispatch_autogen.c"

client_dispatch_table_t *
client_dispatch_table ()
{
    static client_dispatch_table_t dispatch;
    static bool table_initialized = false;
    if (table_initialized)
        return &dispatch;

    client_dispatch_table_fill_base(&dispatch);
    table_initialized = true;

    return &dispatch;
}
