#ifndef NAME_HANDLER_H
#define NAME_HANDLER_H

#include "compiler_private.h"
#include "hash.h"
#include "types_private.h"

#include "config.h"

#if HAS_GLES2
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#else
#include <GL/gl.h>
#endif
#include <stdlib.h>

typedef struct name_handler {
    unsigned int last_name;
    link_list_t *reusable_names;
} name_handler_t;


private name_handler_t *
name_handler_create ();

private void
name_handler_destroy (name_handler_t *name_handler);

private void
name_handler_alloc_names (name_handler_t *name_handler,
                          GLsizei n,
                          GLuint *buffers);

void
name_handler_alloc_name (name_handler_t *name_handler,
                         GLuint buffer);

private void
name_handler_delete_names (name_handler_t *name_handler,
                           GLsizei n,
                           const GLuint *buffers);

#endif /* CLIENT_STATE_H */
