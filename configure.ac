#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([libwab], [060831], [Sean Loaring sloaring@tec-man.com] )

AM_INIT_AUTOMAKE(libwab, 060901)
AC_CONFIG_SRCDIR([libwab.c])
AM_CONFIG_HEADER(config.h)

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

AM_ICONV

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([malloc.h stdlib.h string.h strings.h unistd.h iconv.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([memchr memmove memset strchr strerror strstr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
