#include <stdlib.h>

#include "test_egl.h"
#include "test_egl2.h"
#include "test_gles.h"

static void run_and_clean(gpuprocess_suite_t *suite)
{
    gpuprocess_suite_run_all(suite);
    gpuprocess_suite_destroy(suite);
}

int main(void)
{
    gpuprocess_suite_t *suite_egl;
    gpuprocess_suite_t *suite_egl_make_current;
    gpuprocess_suite_t *suite_gles;
/*
    suite_egl = egl_testsuite_create ();
    run_and_clean (suite_egl);

    suite_egl_make_current = egl_testsuite_make_current_create ();
    run_and_clean (suite_egl_make_current);
*/
    suite_gles = gles_testsuite_create();
    run_and_clean (suite_gles);

return EXIT_SUCCESS;
}
