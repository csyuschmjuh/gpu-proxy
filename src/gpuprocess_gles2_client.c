#include "gpuprocess_gles2_client_private.h"
#include <stdlib.h>

/* thread local client states */
__thread  gl_client_states_t cli_states 
    __attribute__(( tls_model ("initial-exec")));
extern __thread v_link_list_t *active_state;

void 
_gpuprocess_client_init ()
{
    cli_states.vertex_attribs.count = 0;

    cli_states.pack_alignment = cli_states.unpack_alignment = 4;
}

void 
_gpuprocess_client_destroy ()
{
    int i;

    v_vertex_attrib_list_t *attribs = &cli_states.vertex_attribs;

    if (attribs->count == 0)
        return;

    if (attribs->count > NUM_EMBEDDED) 
        free (attribs->attribs);
}

    
