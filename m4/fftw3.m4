dnl @synopsis ACX_FFTW3([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
dnl
dnl This macro looks for the FFTW3 libraries.
dnl To link to FFTW3 libraries, you should use $FFTW3_LIBS
dnl The user may also specify --with-fftw3 or --without-fftw3.
dnl
dnl ACTION-IF-FOUND is a list of shell commands to run if a FFTW3
dnl library is found, and ACTION-IF-NOT-FOUND is a list of commands to
dnl run it if it is not found. If ACTION-IF-FOUND is not specified, the
dnl default action will define HAVE_FFTW3.
dnl
dnl @category InstalledPackages
dnl @author Jonathan D. Dobson <dobsonjd@ornl.gov>
dnl @version 2011-06-01
dnl @license GPLWithACException

AC_DEFUN([ACX_FFTW3], [
AC_PREREQ(2.50)
acx_fftw3_ok=no

dnl
dnl function to try for linking
dnl
try_func=fftw_malloc

AC_ARG_WITH(fftw3,
	[AC_HELP_STRING([--with-fftw3=<lib>], [use FFTW3 library <lib>])
AC_HELP_STRING([--without-fftw3], [disable FFTW3 library]) ])
case $with_fftw3 in
	yes | "") ;;
	no) acx_fftw3_ok=disable ;;
	-* | */* | *.a | *.so | *.so.* | *.o) FFTW3_LIBS="$with_fftw3" ;;
	*) FFTW3_LIBS="-l$with_fftw3" ;;
esac

acx_fftw3_save_LIBS="$LIBS"

# First, check FFTW3_LIBS environment variable
if test $acx_fftw3_ok = no; then
if test "x$FFTW3_LIBS" != x; then
	save_LIBS="$LIBS"; LIBS="$FFTW3_LIBS $LIBS"
	AC_MSG_CHECKING([for $try_func in $FFTW3_LIBS])
	AC_TRY_LINK_FUNC($try_func, [acx_fftw3_ok=yes], [FFTW3_LIBS=""])
	AC_MSG_RESULT($acx_fftw3_ok)
	LIBS="$save_LIBS"
fi
fi

# also check FFTW3_LIB environment variable
if test $acx_fftw3_ok = no; then
if test "x$FFTW3_LIB" != x; then
	save_LIBS="$LIBS"; LIBS="$FFTW3_LIB $LIBS"
	AC_MSG_CHECKING([for $try_func in $FFTW3_LIB])
	AC_TRY_LINK_FUNC($try_func, [acx_fftw3_ok=yes; FFTW3_LIBS="$FFTW3_LIB"], [FFTW3_LIBS=""])
	AC_MSG_RESULT($acx_fftw3_ok)
	LIBS="$save_LIBS"
fi
fi

# FFTW3 linked to by default?  (happens on some supercomputers)
if test $acx_fftw3_ok = no; then
	save_LIBS="$LIBS"; LIBS="$LIBS"
	AC_CHECK_FUNC($try_func, [acx_fftw3_ok=yes])
	LIBS="$save_LIBS"
fi

# Generic FFTW3 library?
if test $acx_fftw3_ok = no; then
	AC_CHECK_LIB(fftw3, $try_func, [acx_fftw3_ok=yes; FFTW3_LIBS="-lfftw3"])
fi

AC_SUBST(FFTW3_LIBS)
LIBS="$acx_fftw3_save_LIBS"

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$acx_fftw3_ok" = xyes; then
        ifelse([$1],,AC_DEFINE(HAVE_FFTW3,1,[Define if you have a FFTW3 library.]),[$1])
        :
else
        acx_fftw3_ok=no
        $2
fi
])dnl ACX_FFTW3
