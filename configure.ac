AC_PREREQ(2.60)
define([VERSION_NUMBER], [0.1]) # For sharing the version number
AC_INIT([GPUPROCESSS], [VERSION_NUMBER], [http://www.samsung.com])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11 -Wall no-define  no-dist-gzip dist-xz])
LT_INIT

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_DISABLE_STATIC

AC_CHECK_HEADER(GL/glx.h, use_glx="yes", [use_glx="no (GLX headers not found)"])
AC_CHECK_HEADER(EGL/egl.h, use_gles2="yes", [use_gles2="no (GLES2 headers not found)"])
AM_CONDITIONAL(HAS_GL, test "$use_glx" = yes)
AM_CONDITIONAL(HAS_GLES2, test "$use_gles2" = yes)

if test "x$use_glx" = "xyes"; then
    AC_DEFINE(HAS_GL, 1, [Define to 1 if you have GLX available])
fi

if test "x$use_gles2" = "xyes"; then
    AC_DEFINE(HAS_GLES2, 1, [Define to 1 if you have GLES2 available])
fi

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
