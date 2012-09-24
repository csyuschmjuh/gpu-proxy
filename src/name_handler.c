
#include "name_handler.h"

name_handler_t *
name_handler_create () {
    name_handler_t *name_handler;
    int i;

    name_handler = (name_handler_t *)malloc (sizeof (name_handler_t));
    for (i = 0; i < RESOURCE_MAX; i++)
        name_handler->last_name[i] = 0;

    return name_handler;
}

void
name_handler_destroy (name_handler_t *name_handler) {
    free (name_handler);
}

GLuint *
name_handler_alloc_names (name_handler_t *name_handler,
                          resource_namespace_t namespace,
                          GLsizei n) {
    GLuint *buffers = (GLuint *)malloc (sizeof (GLuint)*n);
    int i;

    for (i=0; i<n; i++)
        buffers[i] = ++name_handler->last_name[namespace];

    /* FIXME: check overflow. */
    return buffers;
}

