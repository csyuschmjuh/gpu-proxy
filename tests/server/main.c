#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include "test_egl.h"
#include "test_egl2.h"
#include "test_gles.h"

static void run_and_clean(gpuprocess_suite_t *suite)
{
    gpuprocess_suite_run_all(suite);
    gpuprocess_suite_destroy(suite);
}

int main(int argc, char *argv[])
{
    gpuprocess_suite_t *suite_egl;
    gpuprocess_suite_t *suite_egl_make_current;
    gpuprocess_suite_t *suite_gles;
    char *test_name = "gles";
    int c;

    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"test", required_argument, 0, 't'}
        };
        c = getopt_long(argc, argv, "t:h",
                        long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 't': 
            test_name = optarg;
            break;
        }   
    }

    if (strcmp (test_name, "egl") == 0) {
        suite_egl = egl_testsuite_create ();
        run_and_clean (suite_egl);
    } else if (strcmp (test_name, "egl_make_current") == 0) {
        suite_egl_make_current = egl_testsuite_make_current_create ();
        run_and_clean (suite_egl_make_current);
    } else if (strcmp (test_name, "gles") == 0) {
        suite_gles = gles_testsuite_create();
        run_and_clean (suite_gles);
    } else if (strcmp (test_name, "all") == 0) {
        suite_egl = egl_testsuite_create ();
        run_and_clean (suite_egl);

        suite_egl_make_current = egl_testsuite_make_current_create ();
        run_and_clean (suite_egl_make_current);

        suite_gles = gles_testsuite_create();
        run_and_clean (suite_gles);
    }

    return EXIT_SUCCESS;
}
