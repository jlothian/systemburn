dnl @synopsis ACX_SHMEM([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
dnl
dnl This macro looks for the SHMEM libraries.
dnl To link to SHMEM libraries, you should use $SHMEM_LIBS
dnl The user may also specify --with-shmem or --without-shmem.
dnl
dnl ACTION-IF-FOUND is a list of shell commands to run if a SHMEM
dnl library is found, and ACTION-IF-NOT-FOUND is a list of commands to
dnl run it if it is not found. If ACTION-IF-FOUND is not specified, the
dnl default action will define HAVE_SHMEM.
dnl
dnl @category InstalledPackages
dnl @author Jeffery A. Kuehn <kuehn@ornl.gov>
dnl @version 2011-09-27
dnl @license GPLWithACException

AC_DEFUN([ACX_SHMEM], [
AC_PREREQ(2.50)
acx_shmem_ok=no

dnl
dnl function to try for linking
dnl

AC_ARG_WITH(shmem,
	[AC_HELP_STRING([--with-shmem=<lib>], [use SHMEM library <lib>])
AC_HELP_STRING([--without-shmem], [disable SHMEM library]) ])

case $with_shmem in
	yes | "") ;;
	no) acx_shmem_ok=disable ;;
	-* | */* | *.a | *.so | *.so.* | *.o) SHMEM_LIBS="$with_shmem" ;;
	*) SHMEM_LIBS="-l$with_shmem" ;;
esac

acx_shmem_save_LIBS="$LIBS"

try_func=_num_pes
dnl First, check SHMEM_LIBS environment variable
if test $acx_shmem_ok = no; then
if test "x$SHMEM_LIBS" != x; then
	save_LIBS="$LIBS"; LIBS="$SHMEM_LIBS $LIBS"
	AC_MSG_CHECKING([for $try_func in $SHMEM_LIBS])
	AC_TRY_LINK_FUNC($try_func, [acx_shmem_ok=yes], [SHMEM_LIBS=""])
	AC_MSG_RESULT($acx_shmem_ok)
	LIBS="$save_LIBS"
fi
fi
dnl also check SHMEM_LIB environment variable
if test $acx_shmem_ok = no; then
if test "x$SHMEM_LIB" != x; then
	save_LIBS="$LIBS"; LIBS="$SHMEM_LIB $LIBS"
	AC_MSG_CHECKING([for $try_func in $SHMEM_LIB])
	AC_TRY_LINK_FUNC($try_func, [acx_shmem_ok=yes; SHMEM_LIBS="$SHMEM_LIB"], [SHMEM_LIBS=""])
	AC_MSG_RESULT($acx_shmem_ok)
	LIBS="$save_LIBS"
fi
fi
dnl SHMEM linked to by default?  (happens on some supercomputers)
if test $acx_shmem_ok = no; then
	save_LIBS="$LIBS"; LIBS="$LIBS"
	AC_CHECK_FUNC($try_func, [acx_shmem_ok=yes])
	LIBS="$save_LIBS"
fi
dnl Generic SHMEM library?
if test $acx_shmem_ok = no; then
	save_LIBS="$LIBS"; LIBS="-lsma $LIBS"
	AC_MSG_CHECKING([for $try_func in -lsma])
	AC_TRY_LINK_FUNC($try_func, [acx_shmem_ok=yes; SHMEM_LIBS="-lsma"], [SHMEM_LIBS=""])
	AC_MSG_RESULT($acx_shmem_ok)
	LIBS="$save_LIBS"
fi
dnl Generic SHMEM library with MPI?
if test $acx_shmem_ok = no; then
	save_LIBS="$LIBS"; LIBS="-lsma -lmpi $LIBS"
	AC_MSG_CHECKING([for $try_func in -lsma -lmpi])
	AC_TRY_LINK_FUNC($try_func, [acx_shmem_ok=yes; SHMEM_LIBS="-lsma -lmpi"], [SHMEM_LIBS=""])
	AC_MSG_RESULT($acx_shmem_ok)
	LIBS="$save_LIBS"
fi

if test $acx_shmem_ok = yes; then
	AC_DEFINE(SHMEM_LEADING_UNDERSCORE,_,[Define if num_pes() and my_pe() need a leading underscore])
	AC_MSG_WARN(num_pes requires a leading underscore)
fi

try_func=num_pes
dnl First, check SHMEM_LIBS environment variable
if test $acx_shmem_ok = no; then
if test "x$SHMEM_LIBS" != x; then
	save_LIBS="$LIBS"; LIBS="$SHMEM_LIBS $LIBS"
	AC_MSG_CHECKING([for $try_func in $SHMEM_LIBS])
	AC_TRY_LINK_FUNC($try_func, [acx_shmem_ok=yes;AC_DEFINE(SLU,1,[Leading underscore for num_pes() and my_pe()])], [SHMEM_LIBS=""])
	AC_MSG_RESULT($acx_shmem_ok)
	LIBS="$save_LIBS"
fi
fi
dnl also check SHMEM_LIB environment variable
if test $acx_shmem_ok = no; then
if test "x$SHMEM_LIB" != x; then
	save_LIBS="$LIBS"; LIBS="$SHMEM_LIB $LIBS"
	AC_MSG_CHECKING([for $try_func in $SHMEM_LIB])
	AC_TRY_LINK_FUNC($try_func, [acx_shmem_ok=yes; SHMEM_LIBS="$SHMEM_LIB"; AC_DEFINE(SLU,1,[Leading underscore for num_pes() and my_pe()])], [SHMEM_LIBS=""])
	AC_MSG_RESULT($acx_shmem_ok)
	LIBS="$save_LIBS"
fi
fi
dnl SHMEM linked to by default?  (happens on some supercomputers)
if test $acx_shmem_ok = no; then
	save_LIBS="$LIBS"; LIBS="$LIBS"
	AC_CHECK_FUNC($try_func, [acx_shmem_ok=yes; AC_DEFINE(SLU,1,[Leading underscore for num_pes() and my_pe()])])
	LIBS="$save_LIBS"
fi
dnl Generic SHMEM library?
if test $acx_shmem_ok = no; then
	save_LIBS="$LIBS"; LIBS="-lsma $LIBS"
	AC_MSG_CHECKING([for $try_func in -lsma])
	AC_TRY_LINK_FUNC($try_func, [acx_shmem_ok=yes; SHMEM_LIBS="-lsma"; AC_DEFINE(SLU,1,[Leading underscore for num_pes() and my_pe()])], [SHMEM_LIBS=""])
	AC_MSG_RESULT($acx_shmem_ok)
	LIBS="$save_LIBS"
fi
dnl Generic SHMEM library with MPI?
if test $acx_shmem_ok = no; then
	save_LIBS="$LIBS"; LIBS="-lsma -lmpi $LIBS"
	AC_MSG_CHECKING([for $try_func in -lsma -lmpi])
	AC_TRY_LINK_FUNC($try_func, [acx_shmem_ok=yes; SHMEM_LIBS="-lsma -lmpi"; AC_DEFINE(SLU,1,[Leading underscore for num_pes() and my_pe()])], [SHMEM_LIBS=""])
	AC_MSG_RESULT($acx_shmem_ok)
	LIBS="$save_LIBS"
fi




AC_SUBST(SHMEM_LIBS)
LIBS="$acx_shmem_save_LIBS"

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$acx_shmem_ok" = xyes; then
        ifelse([$1],, AC_DEFINE(HAVE_SHMEM,1,[Define if you have a SHMEM library.]) ,[$1])
        :
else
        acx_shmem_ok=no
        $2
fi
])dnl ACX_SHMEM
