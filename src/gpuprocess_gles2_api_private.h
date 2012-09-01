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

#include "gpuprocess_types_private.h"
#include "gpuprocess_thread_private.h"
#include "gpuprocess_egl_states.h"

extern __thread v_link_list_t *active_state;
extern __thread gpuprocess_thread_t *srv_thread;
extern __thread int unpack_alignment;

static inline bool
_is_error_state (void )
{
    egl_state_t *state;

    if (! active_state || ! srv_thread)
        return true;

    state = (egl_state_t *) active_state->data;

    if (! state ||
        ! (state->display == EGL_NO_DISPLAY ||
           state->context == EGL_NO_CONTEXT) ||
	state->active == false) {
        return true;
    }

    return false;
}

