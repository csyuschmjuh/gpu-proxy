#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <string.h>
#include <stdlib.h>

#include "gpuprocess_types_private.h"
#include "gpuprocess_thread_private.h"
#include "gpuprocess_egl_states.h"

extern __thread v_link_list_t *active_state;
extern __thread gpu_thread *srv_thread;
extern __thread int unpack_alignment;

static inline v_bool_t
_is_error_state (void )
{
    egl_state_t *state;

    if (! active_state || ! srv_thread)
        return TRUE;

    state = (egl_state_t *) active_state->data;

    if (! state ||
        ! (state->display == EGL_NO_DISPLAY ||
           state->context == EGL_NO_CONTEXT) ||
	state->active == FALSE) {
        return TRUE;
    }

    return FALSE;
}

