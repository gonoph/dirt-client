# Dirt - Mud Client for Unix (mcl fork)
# process this file with autoconf to produce a configure script.
# Copyright (C) 2018 Billy Holmes

AC_PREREQ([2.69])
m4_include([m4/dirt_version.m4])
AC_INIT([dirt], DIRT_VERSION, [https://github.com/gonoph/dirt-client/issues], [], [https://github.com/gonoph/dirt-client])
AC_COPYRIGHT(DIRT_COPYRIGHT)
AC_DEFINE_UNQUOTED([COPYRIGHT], ["DIRT_COPYRIGHT"], [Copyright notice])
AC_REVISION([$Revision: DIRT_RELEASE$])
AC_CONFIG_SRCDIR([src/main.cc])
AC_CONFIG_AUX_DIR(admin)
AC_CONFIG_HEADERS([src/h/config.h])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_MACRO_DIR([m4])

# Initialize Automake.
AM_INIT_AUTOMAKE([-Wall -Werror gnu std-options 1.13])
AM_PROG_AR
dnl AM_SILENT_RULES([yes])

# Configure language.
AC_LANG(C++)
AC_PROG_CXX([clang++ g++])

# Initialize libtool.
LT_PREREQ([2.4.2])
LT_INIT([shared disable-static])
AC_SUBST([LIBTOOL_DEPS])

# Configure package
AC_ARG_ENABLE([debug],  AS_HELP_STRING([--enable-debug],  [enable debugging (default=no)]))
AC_ARG_ENABLE([efence], AS_HELP_STRING([--enable-efence], [enable electric fence malloc debugger (default=no)]))
AC_ARG_ENABLE([perl],   AS_HELP_STRING([--enable-perl],   [enable perl scripting support (default=yes)]), [], [enable_perl=yes])
AC_ARG_ENABLE([python], AS_HELP_STRING([--enable-python], [enable python scripting support (default=yes)]), [], [enable_python=yes])

AM_CONDITIONAL([WITH_PERL],   [ test _$enable_perl = _yes ])
AM_CONDITIONAL([WITH_PYTHON], [ test _$enable_python = _yes ])
AM_CONDITIONAL([DEBUG],       [ test _$enable_debug = _yes ])

dnl Rules
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_CANONICAL_HOST
AC_PROG_LN_S
AC_PROG_SED

dnl compiler headers and type
AC_CHECK_HEADERS([ext/hash_map hash_map], [break])
AC_CHECK_TYPE([hash<string>],
	AC_DEFINE([HAVE_HASH_STRING], [1], [hash<string> is defined]),
	AC_MSG_ERROR([hash<string> is required!]),
	[
[#include <string>
#if HAVE_EXT_HASH_MAP
#include <ext/hash_map>
#else
#include <hash_map>
#endif
using namespace std;
using namespace __gnu_cxx;
]])
		
dnl libraries
dnl AC_CHECK_LIB(m, main, [], [AC_MSG_ERROR("Required library was missing!")])
AC_CHECK_LIB(dl, dlopen, [], [AC_MSG_ERROR([Unable to dynamically load a symbole!])])
dnl AC_CHECK_LIB(stdc++, main, [], [AC_MSG_ERROR([Required library was missing!])])
AC_CHECK_LIB(ncurses, endwin, [], [AC_MSG_ERROR([ncurses library is required!])])
AC_CHECK_LIB(z, zlibVersion, [], [AC_MSG_ERROR([libz library is required!])])
AC_MSG_CHECKING([if we can natively use pthreads])
AC_RUN_IFELSE([AC_LANG_PROGRAM([
	AC_INCLUDES_DEFAULT
	[#include <pthread.h>]
	], [[return pthread_self();]])], [], [AC_MSG_ERROR("Unable to run posix threads!")])
AC_MSG_RESULT([yes])

AS_IF([test _$enable_efence = _yes],
	AC_CHECK_LIB(efence, main, [AC_MSG_ERROR([efence was enabled, but was not found])])
)
AM_CONDITIONAL([EFENCE], [ test _$enable_efence = _yes ])

dnl do we have perl installed in this system?
AS_IF([test _$enable_perl = _yes],
[
	AC_PATH_PROGS(PERL,[perl5 perl])
	AS_IF([test _$PERL = _], AC_MSG_ERROR([[perl plugin was enabled, but perl was not found!]]))
	AS_IF([$PERL -e 'require 5.008' > /dev/null 2>&1], [], AC_MSG_ERROR([perl 5.008 or greater is required!]))
	AS_IF([$PERL -MExtUtils::Embed -e ''], [], AC_MSG_ERROR([Need perl module ExtUtils::Embed!]))
	PERL_CFLAGS=`$PERL -MExtUtils::Embed -e ccopts`
	PERL_LDFLAGS=`$PERL -MExtUtils::Embed -e ldopts |$SED -e 's/-l(gdbm|db|ndbm|c) //'`
	AS_IF([test _$system = _Linux],
		[PERL_LDFLAGS=`echo $PERL_LDFLAGS | $SED -e 's/-l(nsl|posix) //'`]
	)
	save_CFLAGS=$CFLAGS
	save_LIBS=$LIBS
	CFLAGS="$save_CFLAGS $PERL_CFLAGS"
	LIBS="$save_LIBS $PERL_LDFLAGS"
	AC_CHECK_FUNCS([Perl_eval_pv], [], AC_MSG_ERROR([Unable to link to embedded perl!]))
	CFLAGS=$save_CFLAGS
	LIBS=$save_LIBS
	AS_UNSET(save_CFLAGS)
	AS_UNSET(save_LIBS)
])
AC_ARG_VAR([PERL_CFLAGS], [C Compiler flags from Perl ExtUtils::Embed])
AC_ARG_VAR([PERL_LDFLAGS], [Linker flags from Perl ExtUtils::Embed])

dnl do we have python installed in this system
AS_IF([test _$enable_python = _yes],
[
	AC_PATH_PROGS(PYTHON, [python python2.7 python3])
	AS_IF([test _$PYTHON = _], AC_MSG_ERROR([[python plugin was enabled, but python was not found!]]))
	[PYTHON_CFLAGS=`$PYTHON <<< "from __future__ import print_function
import sys
print('-I%s/include/python%s' % (sys.prefix, sys.version[:3]))"`]
	[PYTHON_LDFLAGS=`$PYTHON <<< "from __future__ import print_function
import sys
print('-L%s/lib/python%s/config -lpython%s' % (sys.prefix, sys.version[:3], sys.version[:3]))"`]
])
AC_ARG_VAR([PYTHON_CFLAGS], [C++ compiler flags for Python])
AC_ARG_VAR([PYTHON_LDFLAGS], [Linker flags for Python])

AC_OUTPUT
