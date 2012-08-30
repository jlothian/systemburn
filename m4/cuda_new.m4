AC_DEFUN([ACX_CUDA], [
AC_PREREQ(2.50)
acx_cuda_ok=no

try_func=cudaMalloc

AC_ARG_WITH(cuda,
	[AC_HELP_STRING([--with-cuda=<lib>], [use CUDA library <lib>])
AC_HELP_STRING([--without-cuda], [disable CUDA library]) ])
case $with_cuda in
	yes | "") ;;
	no) acx_cuda_ok=disable ;;
	-* | */* | *.a | *.so | *.so.* | *.o) CUDA_LIBS="$with_cuda" ;;
	*) CUDA_LIBS="-l$with_cuda" ;;
esac

acx_cuda_save_LIBS="$LIBS"

#find the version we have
ARCH=`uname -m`
if [[ $ARCH == "x86_64" ]]; then
	SUFFIX="64"
else
	SUFFIX=""
fi

END_ONE="/libcudart.so"
END_TWO="/libcublas.so"

# default installation point for cuda
if test "x$acx_cuda_ok" != xdisable; then
if test "x$CUDA_LIBS" = x; then
	CUDA_LIBS="/usr/local/cuda/lib$SUFFIX"
fi
fi

#Check CUDA_LIBS environment variable
if test $acx_cuda_ok = no; then
if test "x$CUDA_LIBS" != x; then
	save_LIBS="$LIBS"; LIBS="$CUDA_LIBS$END_ONE $LIBS"
	AC_MSG_CHECKING([for $try_func in $CUDA_LIBS$END_ONE])
	AC_TRY_LINK_FUNC($try_func, [acx_cuda_ok=yes], [CUDA_LIBS=""])
	AC_MSG_RESULT($acx_cuda_ok)
	LIBS="$save_LIBS"
fi
fi

#Check CUDA_LIB environment variable
if test $acx_cuda_ok = no; then
if test "x$CUDA_LIB" != x; then
	save_LIBS="$LIBS"; LIBS="$CUDA_LIB$END_ONE $LIBS"
	AC_MSG_CHECKING([for $try_func in $CUDA_LIB$END_ONE])
	AC_TRY_LINK_FUNC($try_func, [acx_cuda_ok=yes; CUDA_LIBS="$CUDA_LIB"], [CUDA_LIBS=""])
	AC_MSG_RESULT($acx_cuda_ok)
	LIBS="$save_LIBS"
fi
fi

#Linked by default?
if test $acx_cuda_ok = no; then
	save_LIBS="$LIBS"; LIBS="$LIBS"
	AC_CHECK_FUNC($try_func, [acx_cuda_ok=yes])
	LIBS="$save_LIBS"
fi

#Generic?
if test $acx_cuda_ok = no; then
	AC_CHECK_LIB(cuda, $try_func, [acx_cuda_ok=yes; CUDA_LIBS="-lcuda"])
fi

if test $acx_cuda_ok = yes; then
	CUDA_LIBS="-Wl,-rpath,$CUDA_LIBS $CUDA_LIBS$END_ONE $CUDA_LIBS$END_TWO "
fi

AC_SUBST(CUDA_LIBS)
LIBS="$acx_cuda_save_LIBS"

if test x"$acx_cuda_ok" = xyes; then
	ifelse([$1],,AC_DEFINE(HAVE_CUBLAS,1,[Define if you have a CUDA library.]),[$1])
	:
else
	acx_cuda_ok=no
	$2
fi
])
