AC_PREREQ(2.60)
define([VERSION_NUMBER], [0.0.0]) # For sharing the version number
AC_INIT([GPUPROCESSS], [VERSION_NUMBER], [http://www.samsung.com])

AM_SILENT_RULES([yes])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11 -Wall no-define no-dist-gzip dist-xz])
LT_INIT

AC_PROG_CC_STDC
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_DISABLE_STATIC
AC_PROG_MKDIR_P

AC_ARG_ENABLE([opengles],
              [AS_HELP_STRING([--enable-opengles=@<:@yes/no@:>@],
                              [Enable GLES2 @<:@default=yes@:>@])],
              [], [enable_opengles=yes])
AC_ARG_ENABLE([opengl],
              [AS_HELP_STRING([--enable-opengl=@<:@yes/no@:>@],
                              [Enable Desktop GL @<:@default=no@:>@])],
              [], [enable_opengl=no])

AC_ARG_ENABLE([profiling],
              [AS_HELP_STRING([--enable-profiling=@<:@yes/no@:>@],
                              [Enable profiling output@<:@default=no@:>@])],
              [], [enable_profiling=no])

AS_IF([test "x$enable_opengl" = "xyes"],
      [enable_opengles=no;
      PKG_CHECK_MODULES(GL, gl)
      AC_DEFINE(HAS_GL, 1, [Define to 1 if you have GLX available])
      GPUPROCESS_CFLAGS="$GL_CFLAGS"])

AS_IF([test "x$enable_opengles" = "xyes"],
    [PKG_CHECK_MODULES(GLES, [glesv2])
     AC_DEFINE(HAS_GLES2, 1, [Define to 1 if you have GLES2 available])
     GPUPROCESS_CFLAGS="$EGL_CFLAGS $GLES2_CFLAGS"])

AS_IF([test "x$enable_profiling" = "xyes"],
      [AC_DEFINE(ENABLE_PROFILING, 1, [Define to 1 to enable profiling output])])

AM_CONDITIONAL(HAS_GL, test "x$enable_opengl" = "xyes")
AM_CONDITIONAL(HAS_GLES2, test "x$enable_opengles" = "xyes")
AM_CONDITIONAL(ENABLE_PROFILING, test "x$enable_profiling" = "xyes")

AC_CONFIG_FILES([
Makefile
src/Makefile
tests/server/Makefile
tests/client/Makefile
])

AC_OUTPUT
