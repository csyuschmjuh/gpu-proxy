#ifndef GPUPROCESS_TEST_H
#define GPUPROCESS_TEST_H

#include <stddef.h>
#include <string.h>

#define GPUPROCESS_START_TEST(_testname) \
        static void _testname (void)

#define GPUPROCESS_END_TEST

#define GPUPROCESS_FAIL_IF(expr, ...) \
        _gpuprocess_fail_if (expr, __FILE__, __LINE__, \
                             "failure '"#expr"' occured", __VA_ARGS__)

#define GPUPROCESS_FAIL_UNLESS(expr, ...) \
        _gpuprocess_fail_if (! expr, __FILE__, __LINE__, \
                             "failure '"#expr"' occured", __VA_ARGS__)

typedef void (*gpuprocess_func_t) (void);

typedef struct _gpuprocess_suite gpuprocess_suite_t;
typedef struct _gpuprocess_testcase gpuprocess_testcase_t;

gpuprocess_suite_t *
gpuprocess_suite_create (const char *name);

void
gpuprocess_suite_destroy (gpuprocess_suite_t *suite);

void
gpuprocess_suite_add_testcase (gpuprocess_suite_t *suite, gpuprocess_testcase_t *test);

void
gpuprocess_suite_run_all (gpuprocess_suite_t *suite);

gpuprocess_testcase_t *
gpuprocess_testcase_create (const char *name);

void
gpuprocess_testcase_add_test (gpuprocess_testcase_t *test_case, gpuprocess_func_t func);

void
gpuprocess_testcase_add_fixture (gpuprocess_testcase_t *test_case,
                                 gpuprocess_func_t setup,
                                 gpuprocess_func_t teardown);

void
_gpuprocess_fail_if (int result,
                     const char *file, int line,
                     const char *format, ...);
#endif
