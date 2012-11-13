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

typedef enum resource_namespace {
    RESOURCE_INVALID,
    RESOURCE_GEN_BUFFERS,
    RESOURCE_GEN_FRAMEBUFFERS,
    RESOURCE_GEN_TEXTURES,
    RESOURCE_GEN_RENDERBUFFERS,
    RESOURCE_MAX
} resource_namespace_t;

/* FIXME: We have to allow to reuse names for some cases and. */
typedef struct name_handler {
    unsigned int last_name[RESOURCE_MAX];
    HashTable *used_names[RESOURCE_MAX];
} name_handler_t;

private name_handler_t *
name_handler_create ();

private void
name_handler_destroy (name_handler_t *name_handler);

private void
name_handler_alloc_names (name_handler_t *name_handler,
                          resource_namespace_t namespace,
                          GLsizei n,
                          GLuint *buffers);
void
name_handler_delete_names (name_handler_t *name_handler,
                           resource_namespace_t namespace,
                           GLsizei n,
                           const GLuint *buffers);

#endif /* CLIENT_STATE_H */
