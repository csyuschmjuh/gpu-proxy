#include <dlfcn.h>

#include "gpuprocess_dispatch.h"

//These lib names should be defined by autotools
#ifdef HAS_GL
#define LIBNAME_GL "libGL.so"
#else
#define LIBNAME_GL "libGLESv2.so"
#define LIBNAME_EGL "libEGL.so"
#endif

typedef enum gpuprocess_sym_load_type {
    DLSYM,
    GETPROCADDR
} gpuprocess_sym_load_type_t;

static void
gpuprocess_dispatch_init_entries(gpuprocess_dispatch_t *dispatch,
				 gpuprocess_dispatch_entry_t *entries,
				 gpuprocess_sym_load_type_t type,
				 const char* lib_name)
{
   void *handle = dlopen (lib_name, RTLD_LAZY);
   gpuprocess_dispatch_entry_t *entry = entries;
   char *error;

   while (entry->name[GPUPROCESS_GL_DISPATCH_NAME_CORE] != NULL) {
      int i;
      GenericFunc func = 0;
      void *dispatch_ptr = &((char *) dispatch)[entry->offset];

      for (i = 0; i < GPUPROCESS_GL_DISPATCH_NAME_COUNT; i++) {
          const char *name = entry->name[i];
          if (!name)
              break;

          if (type == DLSYM)
              func = dlsym(handle, name);
          else {
	      func = dispatch->GetProcAddress(name);
              if (!func)
                  func = dlsym(handle, name);
          }
	  
	  if (func)
	      break;
      }
      *((GenericFunc *) dispatch_ptr) = func;

      ++entry;
   }
}

void
gpuprocess_dispatch_init(gpuprocess_dispatch_t *dispatch)
{
    // We should always load dispatch_glx/egl_entries first
#ifdef HAS_GL
    gpuprocess_dispatch_init_entries(dispatch, dispatch_glx_entries, DLSYM, LIBNAME_GL);
#else
    gpuprocess_dispatch_init_entries(dispatch, dispatch_egl_entries, DLSYM, LIBNAME_EGL);
#endif
    gpuprocess_dispatch_init_entries(dispatch, dispatch_opengl_entries, GETPROCADDR, LIBNAME_GL);
}
