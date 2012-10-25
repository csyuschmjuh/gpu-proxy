#ifndef GPUPROCESS_EGL_SERVER_PRIVATE_H
#define GPUPROCESS_EGL_SERVER_PRIVATE_H

#include "server.h"
#include "compiler_private.h"
#include "egl_states.h"
#include "hash.h"
#include "types_private.h"
#include <EGL/egl.h>

#define NUM_CONTEXTS 4

#define CACHING_SERVER(object) ((caching_server_t *) (object))
typedef struct _caching_server {
    server_t super;

    /* We maintain a copy of the superclass' dispatch table here, so
     * that we can chain up to the superclass. The process of subclassing
     * overrides the original dispatch table. */
    dispatch_table_t super_dispatch;

    /* The state of the active GL context. */
    link_list_t *active_state;
    HashTable *names_cache;
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
caching_server_init (caching_server_t *server, buffer_t *buffer);

private caching_server_t *
caching_server_new (buffer_t *buffer);

private void
caching_server_destroy (caching_server_t *server);

#endif /* GPUPROCESS_EGL_SERVER_PRIVATE_H */
