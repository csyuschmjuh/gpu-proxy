#include "gpuprocess_test.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct _gpuprocess_node {
    void *data;
    struct _gpuprocess_node *next;
} gpuprocess_node_t;

typedef struct _gpuprocess_list
{
    gpuprocess_node_t *head;
    gpuprocess_node_t *tail;
} gpuprocess_list_t;

struct _gpuprocess_testcase {
    gpuprocess_list_t *tests;
    const char *name;
    gpuprocess_func_t setup;
    gpuprocess_func_t teardown;
};

struct _gpuprocess_suite {
    gpuprocess_list_t *testcases;
    const char *name;
    gpuprocess_func_t setup;
    gpuprocess_func_t teardown;
};

static void
_dummy_setup_teardown (void)
{
}

static gpuprocess_node_t *
gpuprocess_node_create (void *data)
{
    gpuprocess_node_t *node;

    node = (gpuprocess_node_t *) malloc (sizeof (gpuprocess_node_t));
    node->data = data;
    node->next = NULL;

    return node;
}

static gpuprocess_list_t *
gpuprocess_list_create ()
{
    gpuprocess_list_t *list;
    list = (gpuprocess_list_t *) malloc (sizeof (gpuprocess_list_t));
    list->head = NULL;
    list->tail = NULL;

    return list;
}

static void
gpuprocess_list_destroy (gpuprocess_list_t *list)
{
    gpuprocess_node_t *node = list->head;
    while (node) {
        gpuprocess_node_t *next = node->next;
        free (node);
        node = next;
    }

    free (list);
}

static void
gpuprocess_list_add_node (gpuprocess_list_t *list, gpuprocess_node_t *node)
{
    gpuprocess_node_t *current_node;

    if (! list || ! node)
        return;

    if (! list->head || ! list->tail) {
        list->head = node;
        list->tail = node;
        return;
    }

    current_node = list->tail;
    current_node->next = node;
    list->tail = node;
}

gpuprocess_testcase_t *
gpuprocess_testcase_create (const char *name)
{
    gpuprocess_testcase_t *testcase;

    testcase = (gpuprocess_testcase_t *) malloc (sizeof (gpuprocess_testcase_t));
    testcase->name = name;

    testcase->tests = gpuprocess_list_create ();

    testcase->setup = _dummy_setup_teardown;
    testcase->teardown = _dummy_setup_teardown;

    return testcase;
}

void
gpuprocess_testcase_add_test (gpuprocess_testcase_t *testcase, gpuprocess_func_t func)
{
    gpuprocess_list_add_node (testcase->tests, gpuprocess_node_create (func));
}

void
gpuprocess_testcase_add_fixture (gpuprocess_testcase_t *testcase,
                                 gpuprocess_func_t setup,
                                 gpuprocess_func_t teardown)
{
    testcase->setup = setup;
    testcase->teardown = teardown;
}

gpuprocess_suite_t *
gpuprocess_suite_create (const char *name)
{
    gpuprocess_suite_t *suite = NULL;

    suite = (gpuprocess_suite_t *) malloc (sizeof (gpuprocess_suite_t));
    suite->name = name;

    suite->testcases = gpuprocess_list_create ();

    suite->setup = _dummy_setup_teardown;
    suite->teardown = _dummy_setup_teardown;

    return suite;
}

void
gpuprocess_suite_destroy (gpuprocess_suite_t *suite)
{
    gpuprocess_node_t *suite_node;
    gpuprocess_node_t *testcase_node;

    for (suite_node = suite->testcases->head; suite_node; suite_node = suite_node->next) {
         gpuprocess_testcase_t *testcase = suite_node->data;
         gpuprocess_list_destroy (testcase->tests);
         free (testcase);
    }

    gpuprocess_list_destroy (suite->testcases);
    free (suite);
}

void
gpuprocess_suite_add_testcase (gpuprocess_suite_t *suite, gpuprocess_testcase_t *test)
{
    gpuprocess_list_add_node (suite->testcases, gpuprocess_node_create (test));
}

void
gpuprocess_suite_run_all (gpuprocess_suite_t *suite)
{
    gpuprocess_node_t *suite_node;
    gpuprocess_node_t *testcase_node;

    for (suite_node = suite->testcases->head; suite_node; suite_node = suite_node->next) {
        gpuprocess_testcase_t *testcase = suite_node->data;
        suite->setup ();
        for (testcase_node = testcase->tests->head; testcase_node; testcase_node = testcase_node->next) {
            gpuprocess_func_t func = testcase_node->data;
            testcase->setup ();
            func ();
            testcase->teardown ();
        }
        suite->teardown ();
    }
    printf ("All tests from suite %s passed\n", suite->name);
}

void
gpuprocess_suite_add_fixture (gpuprocess_suite_t *suite,
                              gpuprocess_func_t setup,
                              gpuprocess_func_t teardown)
{
    suite->setup = setup;
    suite->teardown = teardown;
}

void
_gpuprocess_fail_if (int result,
                     const char *file, int line,
                     const char *format, ...)
{
    if (result) {
        const int size = 100;
        char buffer [size];
        va_list args;

        va_start(args, format);
        vsnprintf (buffer, size, format, args);
        va_end(args);

        printf("%s:%d %s\n", file, line, format);
        exit(EXIT_FAILURE);
    }

}
