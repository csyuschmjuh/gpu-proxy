AC_PREREQ(2.60)
define([VERSION_NUMBER], [0.0.0]) # For sharing the version number
AC_INIT([GPUPROCESSS], [VERSION_NUMBER], [http://www.samsung.com])

AM_SILENT_RULES([yes])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11 -Wall no-define  no-dist-gzip dist-xz])
LT_INIT

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_DISABLE_STATIC
AC_PROG_MKDIR_P

AC_ARG_ENABLE(opengl,
	      AS_HELP_STRING([--enable-opengl],
			    [build with OpenGL support]),
			    [opengl_arg="$enableval"])

AS_IF([test "$opengl_arg" = "yes"], [
    PKG_CHECK_MODULES(GL, gl,
		      [use_glx="yes"],
		      [use_glx="no (GLX headers not found)"])
])

AC_ARG_ENABLE([opengles],
	      [AS_HELP_STRING([--enable-opengles]
                              [build with OpenGLESv2 support])],
			      [opengles_arg="$enableval"])

AS_IF([test "$opengles_arg" = "yes"], [
    PKG_CHECK_MODULES(EGL, egl,
		      [use_gles="yes"],
		      [use_gles="no (EGL headers not found)"])

    PKG_CHECK_MODULES(GLES, glesv2,
		      [use_gles="yes"],
		      [use_gles="no (GLES2 headers not found)"])
])

AC_ARG_ENABLE([tests],
	      [AS_HELP_STRING([--enable-tests]
			      [enable tests])],
			      [tests="$enableval"])


AM_CONDITIONAL(HAS_GL, test "$use_glx" = yes)
AM_CONDITIONAL(HAS_GLES2, test "$use_gles" = yes)
AM_CONDITIONAL(HAS_UNIT_TESTS, test "$tests" = yes)

if test "$use_glx" = "yes"; then
    AC_DEFINE(HAS_GL, 1, [Define to 1 if you have GLX available])
    GPUPROCESS_CFLAGS="$GL_CFLAGS"
fi

if test "$use_gles" = "yes"; then
    AC_DEFINE(HAS_GLES2, 1, [Define to 1 if you have GLES2 available])
    GPUPROCESS_CFLAGS="$EGL_CFLAGS $GLES2_CFLAGS"
fi

if test "$use_gles" != "yes" && test "$use_glx" != "yes"; then
    AC_MSG_ERROR([Must select either GLX/Desktop GL or EGL/GLES2.])
fi

if test "$tests" = "yes"; then
    AC_DEFINE([static],,[HACK: make the tests work without modify the code])
fi

AC_SUBST([GPUPROCESS_CFLAGS])

AC_CONFIG_FILES([
Makefile
src/Makefile
tests/srv/Makefile])

AC_OUTPUT
