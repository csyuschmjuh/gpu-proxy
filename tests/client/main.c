#include "basic_test.h"
#include "gpuprocess_test.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    gpuprocess_suite_t *client_suite = gpuprocess_suite_create ("basic");

    add_basic_testcases(client_suite);

    gpuprocess_suite_run_all(client_suite);
    gpuprocess_suite_destroy(client_suite);

    return EXIT_SUCCESS;
}
