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

private void
name_handler_init ();

private void
name_handler_alloc_names (GLsizei n,
                          GLuint *buffers);
private void
name_handler_delete_names (GLsizei n,
                           const GLuint *buffers);

#endif /* CLIENT_STATE_H */
