#include "config.h"
#include "program.h"
#include <stdlib.h>

program_t*
program_new (GLuint id)
{
    program_t *new_program = (program_t *)malloc (sizeof (program_t));
    new_program->id = id;
    new_program->mark_for_deletion = false;
    new_program->uniform_location_cache = id ? NewHashTable(free) : NULL;
    new_program->attrib_location_cache = id ? NewHashTable(free) : NULL;
    return new_program;
}
