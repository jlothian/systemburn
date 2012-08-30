dnl SYNOPSIS
dnl
dnl CT_CHECK_CUDA_API
dnl
dnl DESCRIPTION
dnl This macro tries to find the headers and libraries for the
dnl CUDA API to build client applications.

dnl If includes are found, the variable CDINCPATH will be set. If
dnl libraries are found, the variable CDLIBPATH will be set. if no check
dnl was successful, the script exits with a error message.

dnl LAST MODIFICATION

dnl 2011-01-04

dnl COPYLEFT
dnl
dnl Copyright © 2011 <www.platos-cave.org>

dnl Copying and distribution of this file, with or without
dnl modification, are permitted in any medium without royalty provided
dnl the copyright notice and this notice are preserved.

AC_DEFUN([CT_CHECK_CUDA_API], [

AC_ARG_WITH(cuda,
[ --with-cuda=PREFIX Prefix of your CUDA installation],
[cd_prefix=$withval], [cd_prefix=""])


AC_SUBST(CDINCPATH)
AC_SUBST(CDLIBPATH)
AC_SUBST(NVCC)

AC_CANONICAL_HOST

dnl find out what version we are running
ARCH=`uname -m`
if [[ $ARCH == "x86_64" ]];
then
SUFFIX="64"
else
SUFFIX=""
fi


AC_MSG_NOTICE([$cd_prefix, $withval])

dnl cd_prefix will be set to "yes" if --with-cuda is passed in with no value
#if test "$cd_prefix" == "yes"; then
#if test "$withval" == "yes"; then
cd_prefix="/usr/local/cuda"
#fi
#fi

if test "$cd_prefix" != ""; then

AC_MSG_CHECKING([for CUDA compiler in $cd_prefix/bin])
if test -f "$cd_prefix/bin/nvcc" ; then
NVCC="$cd_prefix/bin/nvcc"
AC_MSG_RESULT([yes])
else
AC_MSG_ERROR(nvcc not found)
fi

AC_MSG_CHECKING([for CUDA includes in $cd_prefix/include])
if test -f "$cd_prefix/include/cuda.h" ; then
CDINCPATH="-I$cd_prefix/include"
AC_MSG_RESULT([yes])
else
AC_MSG_ERROR(cuda.h not found)
fi

AC_MSG_CHECKING([for CUDA libraries in $cd_prefix/lib$SUFFIX])
case $host_os in
darwin*)
if test -f "$cd_prefix/lib$SUFFIX/libcudart.dylib" ; then
CDLIBPATH="-L$cd_prefix/lib$SUFFIX"
AC_MSG_RESULT([yes])
else
AC_MSG_ERROR(libcublas.dylib not found)
fi
;;
linux*)
#if test -f "$cd_prefix/lib$SUFFIX/libcudart.so" ; then
if test -f "$cd_prefix/lib$SUFFIX/libcublas.so" ; then
CDLIBPATH="-L$cd_prefix/lib$SUFFIX"
AC_MSG_RESULT([yes])
else
AC_MSG_ERROR(libcudart.so not found)
fi
;;
*)
dnl Default Case
AC_MSG_ERROR([Your platform is not currently supported]) ;;
esac
fi

if test "$CDINCPATH" = "" ; then
AC_CHECK_HEADER([cuda.h], [], AC_MSG_ERROR(cuda.h not found))
else
#ifelse([$1],,AC_DEFINE(HAVE_CUDA,1,[Define if you have a CUDA library.]),[$1])
#:
#AC_SUBST(CDINCPATH)
fi
if test "$CDLIBPATH" = "" ; then
# AC_CHECK_LIB(cublas, cudaSetDevice, [], AC_MSG_ERROR(libcublas.so not found))
AC_CHECK_LIB([cudart], [cudaMalloc])
else
#ifelse([$1],,AC_DEFINE([HAVE_CUDA],1,[Define if you have a CUDA library.]),[$1])
#:
#AC_SUBST(CDLIBPATH)
fi

])
