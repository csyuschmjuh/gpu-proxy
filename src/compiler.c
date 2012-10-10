#include "config.h"
#include "compiler_private.h"

#if ENABLE_PROFILING
#include <stdio.h>
#include <sys/time.h>
#endif

#if ENABLE_PROFILING
unsigned long
get_time_in_milliseconds ()
{
    struct timeval time_value;
    gettimeofday(&time_value, 0);
    return time_value.tv_sec * (1000 * 1000) + time_value.tv_usec;
}

static int
instrumentation_level = 0;

void
start_instrumentation ()
{
    instrumentation_level++;
}

void
print_instrumentation (instrumentation_t *instrumentation)
{
    unsigned long end_time = get_time_in_milliseconds ();

    int i = 0;
    for (i = 1; i < instrumentation_level; i++)
        printf("  ");

    printf("%s: %lu\n",
           instrumentation->function_name,
           end_time - instrumentation->start_time);
    instrumentation_level--;
}
#endif

