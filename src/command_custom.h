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
