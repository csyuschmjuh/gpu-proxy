#ifndef GPUPROCESS_EGL_SERVER_PRIVATE_H
#define GPUPROCESS_EGL_SERVER_PRIVATE_H

#include "server.h"
#include "compiler_private.h"
#include "egl_states.h"
#include "types_private.h"
#include <EGL/egl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_CONTEXTS 4

#define CACHING_SERVER(object) ((caching_server_t *) (object))
typedef struct _caching_server {
    server_t super;

    /* We maintain a copy of the superclass' dispatch table here, so
     * that we can chain up to the superclass. The process of subclassing
     * overrides the original dispatch table. */
    server_dispatch_table_t super_dispatch;

} caching_server_t;

typedef struct gl_server_states
{
    bool             initialized;
    int              num_contexts;
    link_list_t      *states;
} gl_server_states_t;

/* global state variable */
extern gl_server_states_t        server_states;

private void
_server_init ();

#ifdef __cplusplus
}
#endif


#endif /* GPUPROCESS_EGL_SERVER_PRIVATE_H */
