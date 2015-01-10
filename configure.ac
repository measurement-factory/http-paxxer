#                                               -*- Autoconf -*-
#  @configure_input@
#
#  Configuration scipt for eCAP Library
#
# Process this file with autoconf to produce a configure script.

AC_INIT([HTTP Paxxer], [0.0.x], [], [HttpPaxxer])
HTTP_PAXXER_LIBRARY_VERSION=0:0:0
AC_SUBST(HTTP_PAXXER_LIBRARY_VERSION)
AC_CONFIG_AUX_DIR(cfgaux)
AC_CONFIG_MACRO_DIR(cfgaux)
AC_CONFIG_SRCDIR([src/HttpPaxxer/Paxxer.h])
AC_CONFIG_HEADERS(src/HttpPaxxer/ac-autoconf.h)
AX_PREFIX_CONFIG_H(src/HttpPaxxer/autoconf.h, HTTP_PAXXER)

AM_INIT_AUTOMAKE([-Wall foreign nostdinc])
AM_MAINTAINER_MODE
AM_PROG_AR

LT_INIT

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST

# Checks for library functions.

# Allow the pkg-config directory to be set
AC_ARG_WITH(pkgconfigdir,
	AC_HELP_STRING([--with-pkgconfigdir],
	[Use the specified pkgconfig dir (default is libdir/pkgconfig)]),
	[pkgconfigdir=${withval}],
	[pkgconfigdir='${libdir}/pkgconfig'])

AC_SUBST([pkgconfigdir])
AC_MSG_NOTICE([pkgconfig directory is ${pkgconfigdir}])

AC_CONFIG_FILES([\
        HttpPaxxer.pc \
        Makefile \
        src/Makefile \
        src/HttpPaxxer/Makefile
])
AC_OUTPUT
