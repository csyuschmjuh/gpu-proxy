#include <stdlib.h>

#include "tst_egl.h"

static void run_and_clean(gpuprocess_suite_t *suite)
{
    gpuprocess_suite_run_all(suite);
    gpuprocess_suite_destroy(suite);
}

int main(void)
{
    gpuprocess_suite_t *suite;

    suite = egl_testsuite_create ();
    run_and_clean (suite);

    return EXIT_SUCCESS;
}
