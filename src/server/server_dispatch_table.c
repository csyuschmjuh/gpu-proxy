#include "config.h"
#include "server.h"
#include "types_private.h"
#include "thread_private.h"
#include <dlfcn.h>
#include <stdlib.h>

static void *
libgl_handle ()
{
    static void *handle = NULL;
    if (handle)
        return handle;

    static const char *default_libgl_name = "libGLESv2.so";
    const char *libgl = getenv ("GPUPROCESS_LIBGLES_PATH");

    handle = dlopen (libgl ? libgl : default_libgl_name, RTLD_LAZY);
    return handle;
}

static void *
libegl_handle ()
{
    static void *handle = NULL;
    if (handle)
        return handle;

    static const char *default_libegl_name = "libEGL.so";
    const char *libegl = getenv ("GPUPROCESS_LIBEGL_PATH");

    handle = dlopen (libegl ? libegl : default_libegl_name, RTLD_LAZY);
    return handle;
}

#include "server_dispatch_table_autogen.c"

mutex_static_init (dispatch_table_mutex);

server_dispatch_table_t *
server_dispatch_table_get_base ()
{
    static server_dispatch_table_t dispatch;
    static bool table_initialized = false;
    if (table_initialized)
        return &dispatch;

    mutex_lock (dispatch_table_mutex);

    /* In case two threads got here at the same time, we want to return
     * the dispatch table if another thread already initialized it */
    if (table_initialized) {
        mutex_unlock (dispatch_table_mutex);
        return &dispatch;
    }

    server_dispatch_table_fill_base(&dispatch);
    table_initialized = true;
    mutex_unlock (dispatch_table_mutex);

    return &dispatch;
}
