#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([safewrite], [1.03], [], [], [http://www.lingnu.com/opensource/libsafewrite.html])
AC_CONFIG_SRCDIR([safewrite.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

AC_USE_SYSTEM_EXTENSIONS

# Checks for programs.
AC_LANG_SOURCE([C])
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_MODE_T
AC_TYPE_SIZE_T

AC_CHECK_DECLS([O_NOFOLLOW], [], [AC_DEFINE([O_NOFOLLOW], [0], [Define to 0 if the system does not define this flag])],
        [#include <fcntl.h>])

# Checks for library functions.
AC_CHECK_FUNCS([realpath strrchr strtoul strdup])
AC_FUNC_MALLOC

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
