typedef struct _command_gldrawelements {
    command_t header;
    GLenum mode;
    GLsizei count;
    GLenum type;
    void* indices;
} command_gldrawelements_t;

typedef struct _command_gldrawarrays {
    command_t header;
    GLenum mode;
    GLint first;
    GLsizei count;
} command_gldrawarrays_t;

typedef struct _command_registercontext {
    command_t header;
    EGLDisplay dpy;
    EGLContext context;
    pthread_t client_id;
    bool result;
} command_registercontext_t;

typedef struct _command_registersurface {
    command_t header;
    EGLDisplay dpy;
    EGLSurface surface;
    pthread_t client_id;
    bool result;
} command_registersurface_t;

typedef struct _command_registerimage {
    command_t header;
    EGLDisplay dpy;
    EGLImageKHR image;
    pthread_t client_id;
    bool result;
} command_registerimage_t;

#include "thread_private.h"

void
command_registersurface_init (command_t *command,
                              EGLDisplay display, EGLSurface surface,
                              thread_t client_id);
 
void
command_registercontext_init (command_t *command,
                              EGLDisplay display, EGLContext context,
                              thread_t client_id);

void
command_registerimage_init (command_t *command,
                              EGLDisplay display, EGLImageKHR image,
                              thread_t client_id);
