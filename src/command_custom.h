typedef struct _command_gldrawelements {
    command_t header;
    GLenum mode;
    GLsizei count;
    GLenum type;
    void* indices;
    link_list_t *arrays_to_free;
} command_gldrawelements_t;

typedef struct _command_gldrawarrays {
    command_t header;
    GLenum mode;
    GLint first;
    GLsizei count;
    link_list_t *arrays_to_free;
} command_gldrawarrays_t;

typedef struct _command_log {
    command_t header;
    bool result;
} command_log_t;
