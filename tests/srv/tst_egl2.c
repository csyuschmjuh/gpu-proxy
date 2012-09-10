#include "tst_egl.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <EGL/egl.h>

#include <stdlib.h>

#include <stdio.h>

extern EGLint
_egl_get_error (void);

EGLDisplay
_egl_get_display (EGLNativeDisplayType display_id);

extern EGLBoolean
_egl_initialize (EGLDisplay dpy, EGLint *major, EGLint *minor);

extern EGLBoolean
_egl_terminate (EGLDisplay dpy);

extern EGLContext
_egl_create_context (EGLDisplay dpy, EGLConfig config,
                     EGLContext share_context,
                     const EGLint *attrib_list);

extern EGLSurface
_egl_create_pixmap_surface (EGLDisplay dpy, EGLConfig config,
                            EGLNativePixmapType pixmap,
                            const EGLint *attrib_list);
extern EGLSurface
_egl_create_pbuffer_surface (EGLDisplay dpy, EGLConfig config,
                             const EGLint *attrib_list);
extern EGLSurface
_egl_create_window_surface (EGLDisplay dpy, EGLConfig config,
                            EGLNativeWindowType win,
                            const EGLint *attrib_list);

typedef struct _egl_test {
    Display *dpy;
    EGLDisplay *egl_dpy;
    EGLContext context;
    EGLSurface first_surface;
    EGLSurface second_surface;
} egl_test;

/*
 * first_test_info:
 * context: pixmap
 * first_surface: pixmap surface
 * second_surface: pbuffer surface
 */
egl_test first_test_info;

/*
 * second_test_info:
 * context: window
 * first_surface: window surface
 * second_surface: pbuffer surface
 */

egl_test second_test_info;

static EGLSurface
egl_create_pixmap (Display *dpy, EGLDisplay egl_dpy, EGLConfig cfg,
                   int width, int height)
{
    EGLSurface surf;
    Pixmap pixmap = XCreatePixmap (dpy, DefaultRootWindow(dpy),
                                   width, height, 24);

    surf = _egl_create_pixmap_surface (egl_dpy, cfg,
                                       pixmap, NULL);

    return surf;
}

static XVisualInfo *
ChooseWindowVisual(Display *dpy, EGLDisplay egl_dpy, EGLConfig cfg)
{
    XVisualInfo template, *vinfo;
    EGLint id;
    unsigned long mask;
    int screen = DefaultScreen(dpy);
    Window root_win = RootWindow(dpy, screen);
    int count;

    if (! _egl_get_config_attrib (egl_dpy,
                                  cfg, EGL_NATIVE_VISUAL_ID, &id)) {
        fprintf (stderr, "eglGetConfigAttrib() failed\n");
        exit (-1);
    }

    template.visualid = id;
    vinfo = XGetVisualInfo (dpy, VisualIDMask, &template, &count);
    if (count != 1) {
        fprintf (stderr, "XGetVisualInfo() failed\n");
        exit (-1);
    }

    return vinfo;
}

static Window
CreateWindow(Display *dpy, XVisualInfo *visinfo,
             int width, int height, const char *name)
{
    int screen = DefaultScreen(dpy);
    Window win;
    XSetWindowAttributes attr;
    unsigned long mask;
    Window root;

    root = RootWindow(dpy, screen);

    /* window attributes */
    attr.background_pixel = 0;
    attr.border_pixel = 0;
    attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
    attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
    mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

    win = XCreateWindow(dpy, root, 0, 0, width, height,
                        0, visinfo->depth, InputOutput,
                        visinfo->visual, mask, &attr);
    if (win) {
        XSizeHints sizehints;
        sizehints.width  = width;
        sizehints.height = height;
        sizehints.flags = USSize;
        XSetNormalHints(dpy, win, &sizehints);
        XSetStandardProperties(dpy, win, name, name,
                               None, (char **)NULL, 0, &sizehints);

        XMapWindow(dpy, win);
    }
    return win;
}

static void
setup (void)
{
    EGLConfig config;
    EGLint config_list_length;
    static const EGLint ctx_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    static const EGLint pixmap_buffer_attribs[] = {
        EGL_SURFACE_TYPE, EGL_PIXMAP_BIT,
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
    static const EGLint pbuffer_attribs[] = {
        EGL_WIDTH, 1,
        EGL_HEIGHT, 1,
        EGL_NONE
    };

    static config_attribs[] = {
        EGL_CONFIG_ID, 0,
        EGL_NONE
    };

    Window win;
    EGLBoolean result;
    XVisualInfo *vinfo;

    first_test_info.dpy = XOpenDisplay (NULL);
    GPUPROCESS_FAIL_IF (first_test_info.dpy == NULL, "XOpenDisplay should work");

    first_test_info.egl_dpy = _egl_get_display (first_test_info.dpy);
    GPUPROCESS_FAIL_UNLESS (first_test_info.egl_dpy, "_egl_get_display failed");

    result = _egl_initialize (first_test_info.egl_dpy, NULL, NULL);
    GPUPROCESS_FAIL_IF (!result, "_egl_initialize failed");

    result = _egl_bind_api (EGL_OPENGL_ES_API);
    GPUPROCESS_FAIL_IF (!result, "_egl_bind_api failed");

    result = _egl_choose_config (first_test_info.egl_dpy,
                                 pixmap_buffer_attribs,
                                 &config,
                                 1,
                                 &config_list_length);
    GPUPROCESS_FAIL_IF (!result, "_egl_choose_config failed");

    first_test_info.context = _egl_create_context (first_test_info.egl_dpy,
                                                   config,
                                                   EGL_NO_CONTEXT,
                                                   ctx_attribs);
    GPUPROCESS_FAIL_IF (!first_test_info.context, "_egl_create_context failed");

    first_test_info.first_surface = egl_create_pixmap (first_test_info.dpy,
                                                       first_test_info.egl_dpy,
                                                       config,
                                                       400, 400);
    GPUPROCESS_FAIL_UNLESS (first_test_info.first_surface, "_egl_create_pixmap_surface failed");

    first_test_info.second_surface = _egl_create_pbuffer_surface (first_test_info.egl_dpy,
                                                                  config,
                                                                  pbuffer_attribs);

    /* Second EGL context */

    second_test_info.dpy = XOpenDisplay (NULL);
    GPUPROCESS_FAIL_IF (second_test_info.dpy == NULL, "XOpenDisplay should work");

    second_test_info.egl_dpy = _egl_get_display (second_test_info.dpy);
    GPUPROCESS_FAIL_IF (second_test_info.egl_dpy == EGL_NO_DISPLAY, "_egl_get_display failed");

    result = _egl_initialize (second_test_info.egl_dpy, NULL, NULL);
    GPUPROCESS_FAIL_IF (!result, "_egl_initialize failed");

    result = _egl_choose_config (second_test_info.egl_dpy,
                                 window_attribs,
                                 &config,
                                 1,
                                 &config_list_length);
    GPUPROCESS_FAIL_IF (! result, "_egl_choose_config failed");

    second_test_info.context = _egl_create_context (second_test_info.egl_dpy,
                                                    config,
                                                    EGL_NO_CONTEXT,
                                                    ctx_attribs);
    GPUPROCESS_FAIL_IF (! second_test_info.context, "context should be created");
    vinfo = ChooseWindowVisual(second_test_info.dpy, second_test_info.egl_dpy, config);
    win = CreateWindow(second_test_info.dpy, vinfo, 400, 400, "gpu_proxy test");

    second_test_info.first_surface = _egl_create_window_surface (second_test_info.egl_dpy,
                                                                 config,
                                                                 win,
                                                                 NULL);
    GPUPROCESS_FAIL_IF (second_test_info.first_surface == EGL_NO_SURFACE, "_egl_create_window_surface failed");

    second_test_info.second_surface = _egl_create_pbuffer_surface (second_test_info.egl_dpy,
                                                                  config,
                                                                  pbuffer_attribs);

    GPUPROCESS_FAIL_IF (second_test_info.second_surface == EGL_NO_SURFACE, "_egl_create_pbuffer_surface failed");
}

static void
teardown (void)
{
    XCloseDisplay(first_test_info.dpy);
    XCloseDisplay(second_test_info.dpy);
}

GPUPROCESS_START_TEST
(test_egl_make_current1)
{
    EGLBoolean result;
    EGLint value;
    EGLSurface current_surface;
    EGLContext current_context;

    result = _egl_make_current (first_test_info.egl_dpy,
				first_test_info.first_surface, first_test_info.first_surface,
				first_test_info.context);
    GPUPROCESS_FAIL_IF (result == EGL_FALSE, "_egl_make_current failed");

    /* we destroy first surface, since it is in the current context,
     * it should not be destroyed, we query current surface to check
     * it validity
     */

    result = _egl_destroy_surface (first_test_info.egl_dpy, first_test_info.first_surface);

    current_surface = (EGLSurface)_egl_get_current_surface (EGL_DRAW);
    GPUPROCESS_FAIL_IF (current_surface != first_test_info.first_surface, "_egl_query_surface should not fail because the surface is valid");

    result = _egl_make_current (first_test_info.egl_dpy,
                                first_test_info.second_surface, first_test_info.second_surface,
                                first_test_info.context);
    GPUPROCESS_FAIL_IF (result != EGL_TRUE, "_egl_make_current failed");

    /* we have switched surface, we check the previously destroyed surface
     * was indeeded destroyed
     */
    result = _egl_query_surface(first_test_info.egl_dpy, first_test_info.first_surface, EGL_CONFIG_ID, &value);
    GPUPROCESS_FAIL_IF (result == EGL_TRUE, "Surface should not exist");

    value = _egl_get_error ();
    GPUPROCESS_FAIL_IF (value != EGL_BAD_SURFACE, "value should be EGL_BAD_SURFACE");

    result = _egl_terminate (first_test_info.egl_dpy);
    GPUPROCESS_FAIL_IF (result != EGL_TRUE, "_egl_terminate failed");

    /* we destroyed the current display, we check whether current context
     * is valid or not, - it should be valid since the display we destroyed
     * in current
     */
    current_context = (EGLContext)_egl_get_current_context ();
    GPUPROCESS_FAIL_IF (current_context != first_test_info.context, "value should not be EGL_FALSE");

    result = _egl_make_current (second_test_info.egl_dpy,
                                second_test_info.first_surface, second_test_info.first_surface,
                                second_test_info.context);
    GPUPROCESS_FAIL_IF (result != EGL_TRUE, "_egl_make_current failed");

    /* we switched to the second display, the first display should be
     * terminated
     */
    result = _egl_query_context (first_test_info.egl_dpy, first_test_info.context, EGL_CONFIG_ID, &value);
    value = _egl_get_error ();
    GPUPROCESS_FAIL_IF (value != EGL_BAD_CONTEXT, "value should not be EGL_BAD_CONTEXT");

    /* query for the second context, it should be valid
     */
    result = _egl_query_context (second_test_info.egl_dpy, second_test_info.context, EGL_CONFIG_ID, &value);
    value = _egl_get_error ();
    GPUPROCESS_FAIL_IF (value != EGL_SUCCESS, "value should not be EGL_BAD_CONTEXT");
}
GPUPROCESS_END_TEST

gpuprocess_suite_t *
egl_testsuite_make_current_create (void)
{
    gpuprocess_suite_t *s;
    gpuprocess_testcase_t *tc = NULL;

    tc = gpuprocess_testcase_create("egl_test_make_current");
    gpuprocess_testcase_add_fixture (tc, setup, teardown);

    gpuprocess_testcase_add_test (tc, test_egl_make_current1);

    s = gpuprocess_suite_create ("egl_make_current");
    gpuprocess_suite_add_testcase (s, tc);

    return s;
}
