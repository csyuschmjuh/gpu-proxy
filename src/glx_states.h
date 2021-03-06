#ifndef GPUPROCESS_GLX_STATE_H
#define GPUPROCESS_GLX_STATE_H

#include <GL/glx.h>
#include <GL/glxext.h>
#include "gl_states.h"

typedef struct glx_state {
    GLXContext                context;        /* active context, initial NULL */
    Display                *display;        /* active display, initial NULL */
    gl_state_t                state;                /* the cached states associated
                                         * with this context
                                         */
    GLXDrawable                drawable;        /* active draw drawable, initial None */
    GLXDrawable                readable;        /* active read drawable, initial None */

} glx_state_t;

#endif /* GPUPROCESS_GLX_STATE_H */
