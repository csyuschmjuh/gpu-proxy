#include "config.h"
#include "dispatch_table.h"

#include "types_private.h"
#include "thread_private.h"
#include <dlfcn.h>
#include <stdlib.h>

static void *
find_gl_symbol (void *handle,
                __eglMustCastToProperFunctionPointerType (*getProcAddress) (const char *procname),
                const char *symbol_name)
{
    void *symbol = dlsym (handle, symbol_name);
    if (symbol == NULL)
        symbol = getProcAddress (symbol_name);
    return symbol;
}

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

#include "dispatch_table_autogen.c"

dispatch_table_t *
dispatch_table_get_base ()
{
    static dispatch_table_t dispatch;
    static bool table_initialized = false;
    static bool initializing_table = false;
    if (table_initialized)
        return &dispatch;

    if (initializing_table) {
        return &dispatch;
    }

    /* In case two threads got here at the same time, we want to return
     * the dispatch table if another thread already initialized it */
    if (table_initialized) {
        return &dispatch;
    }

    initializing_table = true;

    FunctionPointerType *temp = NULL;
    temp = (FunctionPointerType *) &real_eglInitialize;
    *temp = dlsym (libegl_handle (), "eglInitialize");
    temp = (FunctionPointerType *) &real_eglGetDisplay;
    *temp = dlsym (libegl_handle (), "eglGetDisplay");

    real_eglInitialize (real_eglGetDisplay (EGL_DEFAULT_DISPLAY), NULL, NULL);
    dispatch_table_fill_base (&dispatch);

    initializing_table = false;
    table_initialized = true;

    return &dispatch;
}
