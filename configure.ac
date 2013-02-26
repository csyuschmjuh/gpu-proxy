AC_PREREQ(2.60)
define([VERSION_NUMBER], [0.0.0]) # For sharing the version number
AC_INIT([GPUPROCESSS], [VERSION_NUMBER], [http://www.samsung.com])

AM_SILENT_RULES([yes])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11 -Wall no-define no-dist-gzip dist-xz])
LT_INIT

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_DISABLE_STATIC
AC_PROG_MKDIR_P

AC_ARG_ENABLE([profiling],
              [AS_HELP_STRING([--enable-profiling=@<:@yes/no@:>@],
                              [Enable profiling output@<:@default=no@:>@])],
              [], [enable_profiling=no])

PKG_CHECK_MODULES(GLES, [glesv2])

AS_IF([test "x$enable_profiling" = "xyes"],
      [AC_DEFINE(ENABLE_PROFILING, 1, [Define to 1 to enable profiling output])])

AM_CONDITIONAL(ENABLE_PROFILING, test "x$enable_profiling" = "xyes")

AC_CONFIG_FILES([
Makefile
src/Makefile
])

AC_OUTPUT
