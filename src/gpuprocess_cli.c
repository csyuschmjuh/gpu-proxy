#include "gpuprocess_cli_private.h"
#include <stdlib.h>

void 
_gpuprocess_cli_init ()
{
    cli_states.vertex_attribs.count = 0;

    cli_states.pack_alignment = cli_states.unpack_alignment = 4;
}

void 
_gpuprocess_cli_destroy ()
{
    int i;

    v_vertex_attrib_list_t *attribs = &cli_states.vertex_attribs;

    if (attribs->count == 0)
	return;

    if (attribs->count > NUM_EMBEDDED) 
	free (attribs->attribs);
}

    
