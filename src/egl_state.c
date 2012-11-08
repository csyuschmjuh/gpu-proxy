#include "config.h"
#include "egl_state.h"
#include <stdlib.h>

void
egl_state_init (egl_state_t *egl_state)
{
    egl_state->context = EGL_NO_CONTEXT;
    egl_state->display = EGL_NO_DISPLAY;
    egl_state->drawable = EGL_NO_SURFACE;
    egl_state->readable = EGL_NO_SURFACE;

    egl_state->active = false;

    egl_state->destroy_dpy = false;
    egl_state->destroy_ctx = false;
    egl_state->destroy_draw = false;
    egl_state->destroy_read = false;

    gles2_state_init (&egl_state->state);
}

egl_state_t *
egl_state_new ()
{
    egl_state_t *new_state = malloc (sizeof (egl_state_t));
    egl_state_init (new_state);
    return new_state;
}

void
egl_state_destroy (egl_state_t *egl_state)
{
    gles2_state_destroy (&egl_state->state);
}
