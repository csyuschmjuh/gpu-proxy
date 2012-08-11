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

AC_ARG_ENABLE(opengl,
	      AS_HELP_STRING([--enable-opengl],
			     [build with OpenGL support]),
			     [opengl_arg="$enableval"])

AS_IF([test "x$opengl_arg" = "xyes"], [
    PKG_CHECK_MODULES(GL, gl,
		      [use_glx="yes"],
		      [use_glx="no (GLX headers not found)"])
])

AC_ARG_ENABLE([opengles],
	      [AS_HELP_STRING([--enable-opengles]
                              [build with OpenGLESv2 support])],
			      [opengles_arg="$enableval"])

AS_IF([test "x$opengles_arg" = "xyes"], [
    PKG_CHECK_MODULES(EGL, egl,
		      [use_gles="yes"],
		      [use_gles="no (EGL headers not found)"])

    PKG_CHECK_MODULES(GLES, glesv2,
		      [use_gles="yes"],
		      [use_gles="no (GLES2 headers not found)"])
])

AM_CONDITIONAL(HAS_GL, test "$use_glx" = yes)
AM_CONDITIONAL(HAS_GLES2, test "$use_gles" = yes)

if test "x$use_glx" = "xyes"; then
    AC_DEFINE(HAS_GL, 1, [Define to 1 if you have GLX available])
    GPUPROCESS_CFLAGS="$GL_CFLAGS"
fi

if test "x$use_gles" = "xyes"; then
    AC_DEFINE(HAS_GLES2, 1, [Define to 1 if you have GLES2 available])
    GPUPROCESS_CFLAGS="$EGL_CFLAGS $GLES2_CFLAGS"
fi

AC_SUBST([GPUPROCESS_CFLAGS])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
