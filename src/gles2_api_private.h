#ifndef GLES_API_PRIVATE_H
#define GLES_API_PRIVATE_H

#include "config.h"

#ifndef HAS_GLES2
#include <GLES/gl.h>
#include <GLES/glext.h>
#else
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif
#include <string.h>
#include <stdlib.h>

#include "client_state.h"
#include "command.h"
#include "command_buffer.h"
#include "types_private.h"
#include "thread_private.h"
#include "egl_states.h"

private bool
_is_error_state (void);

#endif
