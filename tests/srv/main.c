#include <check.h>
#include <stdlib.h>

#include "tst_egl.h"

static void run_and_clean(SRunner *runner)
{
    int number_failed;
    if (!runner)
            return;

    srunner_run_all(runner, CK_VERBOSE);
    number_failed = srunner_ntests_failed(runner);
    srunner_free(runner);
}

int main(void)
{
    Suite *suite;
    SRunner *egl_runner;

    suite = egl_testsuite_create ();
    egl_runner = srunner_create (suite);
    srunner_set_fork_status (egl_runner, CK_NOFORK);

    run_and_clean (egl_runner);

    return EXIT_SUCCESS;
}
