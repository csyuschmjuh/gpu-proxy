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
    EGLContext context;
} command_registercontext_t;

typedef struct _command_registersurface {
    command_t header;
    EGLSurface surface;
} command_registersurface_t;

typedef struct _command_registereglimage {
    command_t header;
    EGLImageKHR eglimage;
} command_registereglimage_t;
