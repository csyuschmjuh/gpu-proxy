#include "tst_egl.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <EGL/egl.h>

Display *dpy = NULL;
EGLDisplay egl_dpy;

static void
setup (void)
{
    dpy = XOpenDisplay (NULL);
    fail_if (dpy == NULL, "XOpenDisplay should work");

    egl_dpy = _egl_get_display (dpy);
    fail_if (egl_dpy == EGL_NO_DISPLAY, "_egl_get_display failed");
}

static void
teardown (void)
{
   XCloseDisplay(dpy);
}

START_TEST
(test_egl_srv_initialize)
{
    EGLint major;
    EGLint minor;
    EGLint get_error_result;
    EGLBoolean result;

    result = _egl_initialize (NULL, &major, &minor);
    fail_if (result, "_egl_initialize can not be created by an invalid display");

    get_error_result = _egl_get_error ();
    fail_unless (get_error_result == EGL_BAD_DISPLAY, "_egl_get_error should return EGL_BAD_DISPLAY");

    major = -1;
    minor = -1;
    result = _egl_initialize (egl_dpy, &major, &minor);
    fail_if (!result, "_egl_initialize failed");
    fail_if (major < 0 && minor < 0, "_egl_initialize returned wrong major and minor values");

    result = _egl_terminate (egl_dpy);   
    fail_if (!result, "_egl_terminate failed");
}
END_TEST

START_TEST
(test_egl_srv_terminate)
{
    EGLint major;
    EGLint minor;
    EGLBoolean result;
    EGLint get_error_result;

    result = _egl_terminate (NULL);
    fail_if (result, "_egl_terminate can not terminate an invalid EGL display connection");

    get_error_result = _egl_get_error (); 
    fail_unless (get_error_result == EGL_BAD_DISPLAY, "_egl_get_error should return EGL_BAD_DISPLAY");

    result = _egl_initialize (egl_dpy, NULL, NULL);
    fail_if (!result, "_egl_initialize failed");

    result = _egl_terminate (egl_dpy);
    fail_if (!result, "_egl_terminate failed");
}
END_TEST

Suite *
egl_testsuite_create (void)
{
    Suite *s;
    TCase *tc = NULL;
    int timeout_seconds = 10;
    tc = tcase_create("egl");

    tcase_add_checked_fixture (tc, setup, teardown);
    tcase_set_timeout (tc, timeout_seconds);

    tcase_add_test (tc, test_egl_srv_initialize);
    tcase_add_test (tc, test_egl_srv_terminate);

    s = suite_create ("egl");
    suite_add_tcase (s, tc);

    return s;
}
