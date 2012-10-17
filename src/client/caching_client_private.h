#ifndef CACHING_CLIENT_PRIVATE_H
#define CACHING_CLIENT_PRIVATE_H

#include "compiler_private.h"
#include "egl_states.h"
#include "types_private.h"
#include "caching_client.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_CONTEXTS 4

#define CACHING_CLIENT(object) ((caching_client_t *) (object))

typedef struct gl_states
{
    bool             initialized;
    int              num_contexts;
    link_list_t      *states;
} gl_states_t;

#endif /* CACHING_CLIENT_PRIVATE_H */
