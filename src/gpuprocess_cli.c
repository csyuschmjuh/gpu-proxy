#include "gpuprocess_cli_private.h"

void 
_gpuprocess_cli_init ()
{
    cli_states.count = 0;

    cli_states.pack_alignment = cli_states.unpack_alignment = 4;
}
