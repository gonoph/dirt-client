# Dirt - Mud Client for Unix (mcl fork)
# process this file with autoconf to produce a configure script.
# Copyright (C) 2018 Billy Holmes

AC_PREREQ([2.69])
m4_include([m4/dirt_version.m4])
AC_INIT([dirt], DIRT_VERSION, [https://github.com/gonoph/dirt-client/issues], [], [https://github.com/gonoph/dirt-client])
AC_COPYRIGHT([2018 (C) Billy Holmes])
AC_REVISION([$Revision: DIRT_RELEASE$])
AC_CONFIG_SRCDIR([src/main.cc])
AC_CONFIG_AUX_DIR(admin)
AC_CONFIG_HEADERS([src/h/config.h])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_MACRO_DIR([m4])

# Initialize Automake.
AM_INIT_AUTOMAKE([-Wall -Werror gnu std-options 1.13])
dnl AM_SILENT_RULES([yes])

# Configure language.
AC_LANG(C++)
AC_PROG_CXX([clang++ g++])

# Initialize libtool.
LT_PREREQ([2.4.2])
LT_INIT([shared disable-static])
AC_SUBST([LIBTOOL_DEPS])

# Configure package
AC_ARG_ENABLE([debug],  [AS_HELP_STRING([--enable-debug],  [enable debugging @<:@default=no@:>@])])
AC_ARG_ENABLE([efence], [AS_HELP_STRING([--enable-efence], [enable electric fence malloc debugger @<:@default=no@:>@])])
AC_ARG_ENABLE([perl],   [AS_HELP_STRING([--enable-perl],   [enable perl scripting support @<:@default=yes@:>@])], [], [enable_perl=yes])
AC_ARG_ENABLE([python], [AS_HELP_STRING([--enable-python], [enable python scripting support @<:@default=yes@:>@])], [], [enable_python=yes])
AM_CONDITIONAL([WITH_PERL],   [ test x$enable_perl = xyes ])
AM_CONDITIONAL([WITH_PYTHON], [ test x$enable_python = xyes ])
AM_CONDITIONAL([DEBUG],       [ test x$enable_debug = xyes ])

# checkfor sed path
EMC_PATH_PROG([sedpath], [sed], [], [AC_MSG_ERROR("Cannot find sed!")], [sed path])

dnl Rules
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_CANONICAL_HOST
AC_PROG_LN_S

dnl compiler headers and type
AC_CHECK_HEADERS([ext/hash_map hash_map], [break])
AC_CHECK_TYPE([__gnu_cxx::hash<std::string>],
	[AC_DEFINE([HAVE_HASH_STRING], [1], [hash<string> is defined])],
	[AC_MSG_ERROR(`hash<string> is required!`)],
	[
AC_INCLUDES_DEFAULT
[#include <string>
#if HAVE_EXT_HASH_MAP
#include <ext/hash_map>
#else
#include <hash_map>
#endif
]])
		
dnl libraries
dnl AC_CHECK_LIB(m, main, [], [AC_MSG_ERROR("Required library was missing!")])
dnl AC_CHECK_LIB(dl, dlopen, [], [AC_MSG_ERROR([Unable to dynamically load a symbole!])])
dnl AC_CHECK_LIB(stdc++, main, [], [AC_MSG_ERROR([Required library was missing!])])
AC_CHECK_LIB(ncurses, endwin, [], [AC_MSG_ERROR([ncurses library is required!])])
AC_CHECK_LIB(z, zlibVersion, [], [AC_MSG_ERROR([libz library is required!])])
AC_MSG_CHECKING([if we can natively use pthreads])
AC_RUN_IFELSE(
	[AC_LANG_PROGRAM([[
#include <pthread.h>
]], [[return pthread_self();]])],
	[], [AC_MSG_ERROR("Unable to run posix threads!")])
AC_MSG_RESULT([yes])

AS_IF([test x$enable_efence = xyes],
	AC_CHECK_LIB(efence, main, [AC_MSG_ERROR([efence was enabled, but was not found])])
)
AM_CONDITIONAL([EFENCE], [ test x$enable_efence = xyes ])

dnl do we have perl installed in this system?
case "x$enable_perl" in
	"xyes")
	AC_PATH_PROGS(PERL,perl5 perl,no)
        ;;
	"x")
	AC_PATH_PROGS(PERL,perl5 perl,no)
        ;;
        "xno")
	echo "perl scripting support disabled in command line"
	WITH_PERL=""
        ;;
        *)
	echo "using $enable_perl as the perl binary"
        ;;
esac

if test "x$enable_perl" != "xno" -a "x$PERL" != "xno"; then
	AC_MSG_CHECKING(for perl version)
	$PERL -e 'require 5.008' > /dev/null 2>&1
	if test $? -ne 0 ; then
		echo "perl 5.008 (or greater) is required"
                echo "disabling perl scripting support"
                PERL="no"
		WITH_PERL=""
	else
            AC_PATH_PROG(perlpath, perl)
            AC_MSG_CHECKING(for Perl compile flags)
            PERL_CFLAGS=`$perlpath -MExtUtils::Embed -e ccopts 2>/dev/null`
            PERL_CPPFLAGS=`$perlpath -MExtUtils::Embed -e ccopts 2>/dev/null`
            if test "_$PERL_CFLAGS" = _ ; then
                    AC_MSG_RESULT([not found, building without perl.])
                    perl=no
            else
                    PERL_LDFLAGS=`$perlpath -MExtUtils::Embed -e ldopts |$sedpath 's/-lgdbm //'`
                    PERL_LDFLAGS=`echo $PERL_LDFLAGS |$sedpath 's/-ldb //'`
                    PERL_LDFLAGS=`echo $PERL_LDFLAGS |$sedpath 's/-lndbm //'`
                    if test "$system" = "Linux"; then
                            PERL_LDFLAGS=`echo $PERL_LDFLAGS |$sedpath 's/-lnsl //'`
                            PERL_LDFLAGS=`echo $PERL_LDFLAGS |$sedpath 's/-lposix //'`
                    fi
                    PERL_LDFLAGS=`echo $PERL_LDFLAGS |$sedpath 's/-lc //'`
                    AC_MSG_RESULT(ok)
                    CFLAGS="$CFLAGS $PERL_CFLAGS"
                    LIBS="$LIBS $PERL_LDFLAGS"
                    AC_CHECK_FUNCS(eval_pv)
            fi

#		perl_version=`$PERL -e 'print $]'`
#		AC_MSG_RESULT($perl_version)
#                PERL_CPPFLAGS=`$PERL -MExtUtils::Embed -e perl_inc`
#                PERL_LDFLAGS=`$PERL -MExtUtils::Embed -e ldopts`
                dnl | $PERL -pe 's/-l(gdbm|ndbm|posix|db)\b//g'`
		WITH_PERL="perl"
	fi
else
	WITH_PERL=""
	echo "could not find a valid perl interpreter"
	echo "disabling perl scripting support"
fi

if test "$perl" = yes; then
        AC_PATH_PROG(perlpath, perl)
        AC_MSG_CHECKING(for Perl compile flags)
        PERL_CFLAGS=`$perlpath -MExtUtils::Embed -e ccopts 2>/dev/null`
        if test "_$PERL_CFLAGS" = _ ; then
                AC_MSG_RESULT([not found, building without perl.])
                perl=no
        else
                PERL_LDFLAGS=`$perlpath -MExtUtils::Embed -e ldopts |$sedpath 's/-lgdbm //'`
                PERL_LDFLAGS=`echo $PERL_LDFLAGS |$sedpath 's/-ldb //'`
                PERL_LDFLAGS=`echo $PERL_LDFLAGS |$sedpath 's/-lndbm //'`
                if test "$system" = "Linux"; then
                        PERL_LDFLAGS=`echo $PERL_LDFLAGS |$sedpath 's/-lnsl //'`
                        PERL_LDFLAGS=`echo $PERL_LDFLAGS |$sedpath 's/-lposix //'`
                fi
                PERL_LDFLAGS=`echo $PERL_LDFLAGS |$sedpath 's/-lc //'`
                AC_MSG_RESULT(ok)
                CFLAGS="$CFLAGS $PERL_CFLAGS"
                LIBS="$LIBS $PERL_LDFLAGS"
                AC_CHECK_FUNCS(eval_pv)
        fi
fi


dnl do we have python installed in this system
case "x$enable_python" in
	"xyes")
	AC_PATH_PROGS(PYTHON,python python2.2 python2.1 python2.0 python1.6 python1.5 python1.4 python1.3,no)
        ;;
        "xno")
	echo "python scripting support disabled in command line"
	WITH_PYTHON=""
        ;;
esac

if test "x$enable_python" = "xyes" -o "x$PYTHON" = "xyes"; then
	dnl python {cpp,ld}flags
	changequote(, )dnl
	PYTHON_CPPFLAGS=`$PYTHON -c '
import sys
print "-I%s/include/python%s" % (sys.prefix, sys.version[:3])'`
	PYTHON_LDFLAGS=`$PYTHON -c '
import sys
print "-L%s/lib/python%s/config -lpython%s" % (sys.prefix, sys.version[:3], sys.version[:3])'`
	changequote([, ])dnl
	WITH_PYTHON="python"
	PYTHON_CPPFLAGS+=" -fPIC"
        echo "PYTHON_CPPFLAGS=$PYTHON_CPPFLAGS"
        echo "PYTHON_LDFLAGS=$PYTHON_LDFLAGS"
else
	WITH_PYTHON=""
	echo "disabling python scripting support"
fi

dnl check if this system support the flag -fno-rtti
save_CFLAGS="$CFLAGS"
CFLAGS="-fno-rtti"
AC_MSG_CHECKING(if the flag -fno-rtti is supported by the system compiler)
AC_CACHE_VAL(ac_cv_flag_no_rtti,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[int a;]], [[int a;]])],[ac_cv_flag_no_rtti=yes],[ac_cv_flag_no_rtti=no])])
AC_MSG_RESULT($ac_cv_flag_no_rtti)
CFLAGS="$save_CFLAGS"
case "x$ac_cv_flag_no_rtti" in
	"xyes")
        HAVE_FLAG_NO_RTTI="-fno-rtti"
        ;;
        "xno")
        HAVE_FLAG_NO_RTTI=""
        ;;
        *)
        HAVE_FLAG_NO_RTTI=""
        dnl this should never happen
        ;;
esac

dnl check if this system support the flag -fno-exceptions
save_CFLAGS="$CFLAGS"
CFLAGS="-fno-exceptions"
AC_MSG_CHECKING(if the flag -fno-exceptions is supported by the system compiler)
AC_CACHE_VAL(ac_cv_flag_no_exceptions,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[int a;]], [[int a;]])],[ac_cv_flag_no_exceptions=yes],[ac_cv_flag_no_exceptions=no])])
AC_MSG_RESULT($ac_cv_flag_no_exceptions)
CFLAGS="$save_CFLAGS"
case "x$ac_cv_flag_no_exceptions" in
	"xyes")
        HAVE_FLAG_NO_EXCEPTIONS="-fno-exceptions"
        ;;
        "xno")
        HAVE_FLAG_NO_EXCEPTIONS=""
        ;;
        *)
        HAVE_FLAG_NO_EXCEPTIONS=""
        dnl this should never happen
        ;;
esac

case "$host" in
	*-*-linux*)
	LOCAL_CFLAGS="-fPIC -Wall -pipe -Wno-write-strings -Wno-deprecated -Wno-narrowing"
	LOCAL_CPPFLAGS="-Ih"
	LOCAL_CXXFLAGS="-W $LOCAL_CFLAGS"

	dnl Which version of gcc and g++ are we using?
	dnl if ((gcc > 2.7) && (g++ > 2.7)) we do not
	dnl need to add even more compile flags
	cat > conftest.c <<EOF
#if __GNUC__ > 2
	yes;
#endif
#if __GNUC__ > 1
#if __GNUC_MINOR__ > 7
	yes;
#endif
#endif
EOF
	ac_good_gcc_try='${CC-cc} -E conftest.c'
	if eval "$ac_good_gcc_try" | egrep yes >/dev/null 2>&1; then
		have_good_gcc=yes
	fi
	
	ac_good_gpp_try='${CXX-cc} -E conftest.c'
	if eval "$ac_good_gpp_try" | egrep yes >/dev/null 2>&1; then
		have_good_gpp=yes
	fi
	rm conftest.c

	if test "x$have_good_gcc" = "xyes" -a "x$have_good_gpp" = "xyes"; then
		LOCAL_LDFLAGS="-rdynamic"
		LOCAL_LIBS=""
	else
		LOCAL_LDFLAGS="-nodefaultlibs -rdynamic"
		LOCAL_LIBS="-lc -lgcc"
	fi

	;;
	*)
	LOCAL_CFLAGS=""
	LOCAL_CPPFLAGS="-Ih"
	LOCAL_CXXFLAGS="$LOCAL_CFLAGS"
	LOCAL_LDFLAGS=""
	LOCAL_LIBS=""
	;;
esac

COMPILED_BY=`whoami`@`uname -n`

AC_OUTPUT
