#include <dlfcn.h>
#include <stdlib.h>

static void *
find_gl_symbol (void *handle,
                __eglMustCastToProperFunctionPointerType (*getProcAddress) (const char *procname),
                const char *symbol_name)
{
    if (getProcAddress) {
        void *symbol = getProcAddress (symbol_name);
        if (symbol)
            return symbol;
    }

    return dlsym (handle, symbol_name);
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
