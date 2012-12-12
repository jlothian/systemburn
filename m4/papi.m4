AC_DEFUN([ACX_PAPI], [
AC_PREREQ(2.50)
acx_papi_ok=no

try_func=PAPI_start

AC_ARG_WITH(papi,
	[AC_HELP_STRING([--with-papi=<lib>], [use PAPI library <lib>])
AC_HELP_STRING([--without-papi], [disable PAPI library]) ])
case $with_papi in
	yes | "") ;;
	no) acx_papi_ok=disable ;;
	-* | */* | *.a | *.so | *.so.* | *.o) PAPI_LIBS="$with_papi" ;;
	*) PAPI_LIBS="-l$with_papi" ;;
esac

acx_papi_save_LIBS="$LIBS"

#find the version we have
ARCH=`uname -m`
if [[ $ARCH == "x86_64" ]]; then
	SUFFIX="64"
else
	SUFFIX=""
fi

# default installation point for papi
if test "x$acx_papi_ok" != xdisable; then
if test "x$PAPI_LIBS" = x; then
	PAPI_LIBS="-lpapi"
fi
fi

#Check PAPI_LIBS environment variable
if test $acx_papi_ok = no; then
if test "x$PAPI_LIBS" != x; then
	save_LIBS="$LIBS"; LIBS="$PAPI_LIBS $LIBS"
	AC_MSG_CHECKING([for $try_func in $PAPI_LIBS])
	AC_TRY_LINK_FUNC($try_func, [acx_papi_ok=yes], [PAPI_LIBS=""])
	AC_MSG_RESULT($acx_papi_ok)
	LIBS="$save_LIBS"
fi
fi

#Check PAPI_LIB environment variable
if test $acx_papi_ok = no; then
if test "x$PAPI_LIB" != x; then
	save_LIBS="$LIBS"; LIBS="$PAPI_LIB $LIBS"
	AC_MSG_CHECKING([for $try_func in $PAPI_LIB])
	AC_TRY_LINK_FUNC($try_func, [acx_papi_ok=yes; PAPI_LIBS="$PAPI_LIB"], [PAPI_LIBS=""])
	AC_MSG_RESULT($acx_papi_ok)
	LIBS="$save_LIBS"
fi
fi

#Linked by default?
if test $acx_papi_ok = no; then
	save_LIBS="$LIBS"; LIBS="$LIBS"
	AC_CHECK_FUNC($try_func, [acx_papi_ok=yes])
	LIBS="$save_LIBS"
fi

#Generic?
if test $acx_papi_ok = no; then
	AC_CHECK_LIB(papi, $try_func, [acx_papi_ok=yes; PAPI_LIBS="-lpapi"])
fi

if test $acx_papi_ok = yes; then
	PAPI_LIBS="-Wl,-rpath,$PAPI_LIBS $PAPI_LIBS$"
fi

AC_SUBST(PAPI_LIBS)
LIBS="$acx_papi_save_LIBS"

if test x"$acx_papi_ok" = xyes; then
	ifelse([$1],,AC_DEFINE(HAVE_PAPI,1,[Define if you have a PAPI library.]),[$1])
	:
else
	acx_papi_ok=no
	$2
fi
])
