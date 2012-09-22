#include "test_egl.h"
#include "egl_server_tests.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <EGL/egl.h>
#include <stdlib.h>

Display *dpy = NULL;
EGLDisplay egl_dpy;
command_buffer_server_t *server;

static void
setup (void)
{
    buffer_t *first_buffer = malloc (sizeof (buffer_t));
    buffer_create (first_buffer);
    server = (command_buffer_server_t *) caching_server_new (first_buffer);

    dpy = XOpenDisplay (NULL);
    GPUPROCESS_FAIL_IF (dpy == NULL, "XOpenDisplay should work");

    egl_dpy = _egl_get_display (server, dpy);
    GPUPROCESS_FAIL_IF (egl_dpy == EGL_NO_DISPLAY, "_egl_get_display failed");
}

static void
teardown (void)
{
   XCloseDisplay(dpy);
}

GPUPROCESS_START_TEST
(test_egl_srv_initialize)
{
    EGLint major;
    EGLint minor;
    EGLint get_error_result;
    EGLBoolean result;

    result = _egl_initialize (server, NULL, &major, &minor);
    GPUPROCESS_FAIL_IF (result, "_egl_initialize can not be created by an invalid display");

    get_error_result = _egl_get_error (server);
    GPUPROCESS_FAIL_UNLESS (get_error_result == EGL_BAD_DISPLAY, "_egl_get_error should return EGL_BAD_DISPLAY");

    major = -1;
    minor = -1;
    result = _egl_initialize (server, egl_dpy, &major, &minor);
    GPUPROCESS_FAIL_IF (!result, "_egl_initialize failed");
    GPUPROCESS_FAIL_IF (major < 0 && minor < 0, "_egl_initialize returned wrong major and minor values");

}
GPUPROCESS_END_TEST

GPUPROCESS_START_TEST
(test_egl_srv_terminate)
{
    EGLint major;
    EGLint minor;
    EGLBoolean result;
    EGLint get_error_result;

    result = _egl_terminate (server, NULL);
    GPUPROCESS_FAIL_IF (result, "_egl_terminate can not terminate an invalid EGL display connection");

    get_error_result = _egl_get_error (server);
    GPUPROCESS_FAIL_UNLESS (get_error_result == EGL_BAD_DISPLAY, "_egl_get_error should return EGL_BAD_DISPLAY");

    result = _egl_terminate (server, egl_dpy);
    GPUPROCESS_FAIL_IF (!result, "_egl_terminate failed");
}
GPUPROCESS_END_TEST

GPUPROCESS_START_TEST
(test_egl_get_configs)
{
    EGLBoolean result;
    EGLint config_list_length;
    EGLint old_config_list_length;
    EGLConfig *config_list = NULL;
    EGLint get_error_result;

    result = _egl_get_configs (server, egl_dpy, NULL, 0, &config_list_length);
    GPUPROCESS_FAIL_IF (!result, "_egl_get_configs failed");
    GPUPROCESS_FAIL_IF (config_list_length <= 0, "config list should not be <= 0");

    config_list = (EGLConfig *) malloc (config_list_length * sizeof (EGLConfig));
    GPUPROCESS_FAIL_IF (!config_list, "error allocating config_list");

    old_config_list_length = config_list_length;
    result = _egl_get_configs (server, egl_dpy, config_list, config_list_length, &config_list_length);
    GPUPROCESS_FAIL_IF (!result, "_egl_get_configs failed");
    GPUPROCESS_FAIL_UNLESS (config_list_length == old_config_list_length, "They should have the same lenght");

    result = _egl_get_configs (server, NULL, NULL, 0, &config_list_length);
    GPUPROCESS_FAIL_IF (result, "_egl_get_configs should return false");

    get_error_result = _egl_get_error (server);
    GPUPROCESS_FAIL_UNLESS (get_error_result == EGL_BAD_DISPLAY, "_egl_get_error should return EGL_BAD_DISPLAY");

    free (config_list);
}
GPUPROCESS_END_TEST

GPUPROCESS_START_TEST
(test_egl_bind_api)
{
    EGLBoolean result;

    result = _egl_bind_api (server, EGL_OPENGL_ES_API);
    GPUPROCESS_FAIL_IF (!result, "_egl_bind_api failed");
}
GPUPROCESS_END_TEST

GPUPROCESS_START_TEST
(test_egl_create_destroy_context)
{
    EGLBoolean result;
    EGLint config_list_length;
    EGLint old_config_list_length;
    EGLConfig config;
    EGLint get_error_result;
    EGLContext pbuffer_context, window_context;
    static const EGLint ctx_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    static const EGLint pbuffer_attribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_NONE
    };
    static const EGLint window_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_NONE
    };

    result = _egl_choose_config (server, egl_dpy, pbuffer_attribs, &config, 1, &config_list_length);
    GPUPROCESS_FAIL_IF (!result, "_egl_choose_config failed");

    pbuffer_context = _egl_create_context (server, egl_dpy, config, EGL_NO_CONTEXT, ctx_attribs);
    GPUPROCESS_FAIL_IF (!pbuffer_context, "_egl_create_context failed");

    result = _egl_choose_config (server, egl_dpy, window_attribs, &config, 1, &config_list_length);
    GPUPROCESS_FAIL_IF (!result, "_egl_choose_config failed");

    window_context = _egl_create_context (server, egl_dpy, config, EGL_NO_CONTEXT, ctx_attribs);
    GPUPROCESS_FAIL_IF (! window_context, "context should be created");

    result = _egl_destroy_context (server, egl_dpy, window_context);
    GPUPROCESS_FAIL_IF (!result, "_egl_destroy_context failed");

    result = _egl_destroy_context (server, egl_dpy, pbuffer_context);
    GPUPROCESS_FAIL_IF (!result, "_egl_destroy_context failed");

    get_error_result = _egl_get_error (server);
    GPUPROCESS_FAIL_IF (get_error_result != EGL_SUCCESS, "_egl_get_error should fail");

}
GPUPROCESS_END_TEST

gpuprocess_suite_t *
egl_testsuite_create (void)
{
    gpuprocess_suite_t *s;
    gpuprocess_testcase_t *tc = NULL;

    tc = gpuprocess_testcase_create("egl");

    gpuprocess_testcase_add_test (tc, test_egl_srv_initialize);
    gpuprocess_testcase_add_test (tc, test_egl_bind_api);
    gpuprocess_testcase_add_test (tc, test_egl_get_configs);
    gpuprocess_testcase_add_test (tc, test_egl_create_destroy_context);
    gpuprocess_testcase_add_test (tc, test_egl_srv_terminate);

    s = gpuprocess_suite_create ("egl");
    gpuprocess_suite_add_fixture (s, setup, teardown);
    gpuprocess_suite_add_testcase (s, tc);

    return s;
}
