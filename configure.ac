
dnl
dnl Process this file with autoconf to produce a configure script
dnl
AC_INIT(SystemBurn,1.0.4)

dnl
dnl Determine source and build directories
dnl
CONFIG_BUILD_DIR=`pwd`/build
CONFIG_PROJECT_RELATIVE_DIR="$srcdir"
CONFIG_PROJECT_DIR=`cd $srcdir; pwd`
AC_SUBST(CONFIG_BUILD_DIR)
AC_SUBST(CONFIG_PROJECT_DIR)

dnl
dnl Load macros
dnl
m4_include(m4/blas.m4)
m4_include(m4/fftw3.m4)
m4_include(m4/mpi.m4)
m4_include(m4/pthread.m4)
m4_include(m4/shmem.m4)
#m4_include(m4/cuda.m4)
#m4_include(m4/cuda_sdk.m4)
#m4_include(m4/cuda_alt.m4)
m4_include(m4/cuda_new.m4)
m4_include(m4/orbt.m4)

dnl
dnl Check for C compiler 
dnl
AC_PROG_CC(gcc)
AC_LANG_C
AC_SUBST(CC)

dnl
dnl Check for C dependency generator
dnl
depc_exists="no"
AC_PATH_PROG([depc_exists],
	[gcc],
	AC_MSG_ERROR(GCC is required to generate C code dependencies.))
DEPC_PATH=$depc_exists
AC_SUBST(DEPC_PATH)

dnl 
dnl Check for C headers
dnl
AC_HEADER_STDC
AC_CHECK_HEADER([stdio.h],
		[],
		AC_MSG_ERROR(Must include headers for libc))
AC_CHECK_HEADER([stdlib.h],
		[],
		AC_MSG_ERROR(Must include headers for libc))
AC_CHECK_HEADER([limits.h],
		[],
		AC_MSG_ERROR(Must include headers for libc))
AC_CHECK_HEADER([signal.h],
		[],
		AC_MSG_ERROR(Must include headers for libc))
AC_CHECK_HEADER([errno.h],
		[],
		AC_MSG_ERROR(Must include headers for libc))
AC_CHECK_HEADER([string.h],
		[],
		AC_MSG_ERROR(Must include headers for libc))
AC_CHECK_HEADER([math.h],
		[],
		AC_MSG_ERROR(Must include headers for libc))
AC_CHECK_HEADER([fcntl.h],
		[],
		AC_MSG_ERROR(Must include headers for fcntl.h))
AC_CHECK_HEADER([unistd.h],
		[],
		AC_MSG_ERROR(Must include headers for unistd.h))
AC_CHECK_HEADER([sys/types.h],
		[],
		AC_MSG_ERROR(Must include headers for sys/types.h))
AC_CHECK_HEADER([sys/stat.h],
		[],
		AC_MSG_ERROR(Must include headers for sys/stat.h))

dnl
dnl Check for OpenMP, MPI, and Pthreads
dnl
AC_OPENMP
ACX_MPI([],AC_MSG_ERROR(MPI C compiler not found.))
ACX_PTHREAD([],AC_MSG_ERROR(Pthreads library not found.))
if test "x$PTHREAD_CC" != "x" ; then
	CC="$PTHREAD_CC"
fi

dnl
dnl Checks for typedefs, structures, and compiler characteristics.
dnl
AC_TYPE_SIZE_T

dnl
dnl Check for SHMEM communication libraries.
dnl
dnl     ACX_SHMEM replaces....
dnl	AC_CHECK_LIB([sma], [_num_pes], [SHMEM_LIBS="-lsma -lmpi"], [], [-lmpi])
dnl     or
dnl	AC_CHECK_LIB([sma], [shmem_put], [SHMEM_LIBS="-lsma"], [], [])
dnl	if test x = x"$SHMEM_LIBS"; then
dnl		AC_MSG_FAILURE([--with-shmem was given, but the SHMEM library was not detected.])
dnl	else
dnl		AC_DEFINE([HAVE_SHMEM], [1], [Define if you have the SHMEM communication library.])
dnl	fi

SHMEM_LIBS=""
ENABLE_SHMEM=0
AC_ARG_WITH([shmem],
	[AS_HELP_STRING([--with-shmem],
	[enable use of the SHMEM communication routines.])],
	[],
	[with_shmem=no])
AS_IF([test "x$with_shmem" != xno], [
	ACX_SHMEM([ENABLE_SHMEM=1
		AC_DEFINE(HAVE_SHMEM,1,[Define if you have a SHMEM library.]) ], [
		AC_MSG_WARN(SHMEM library not found.  Disabling modules that require it.)])
	AC_SUBST(ENABLE_SHMEM)
])

dnl
dnl Checks for BLAS and FFTW3 libraries.
dnl
ENABLE_BLAS=0
ENABLE_FFTW=0
ACX_BLAS([ENABLE_BLAS=1
	AC_DEFINE(HAVE_BLAS,1,[Define if you have a BLAS library.]) ],
	AC_MSG_WARN(BLAS library not found.  Disabling modules that require it.))
[BLAS_INC=`echo "$BLAS_LIBS" | sed 's/\(-l[^ \t\n$]*\)\|\(-L[^ \t\n$]*\)//g'`]
ACX_FFTW3([ENABLE_FFTW=1
	AC_DEFINE(HAVE_FFTW3,1,[Define if you have a FFTW3 library.]) ],
	AC_MSG_WARN(FFTW3 library not found.  Disabling modules that require it.))
[FFTW3_INC=`echo "$FFTW3_LIBS" | sed 's/\(-l[^ \t\n$]*\)\|\(-L[^ \t\n$]*\)//g'`]
AC_SUBST(ENABLE_BLAS)
AC_SUBST(ENABLE_FFTW)

dnl
dnl Checking for CUDA
dnl
ENABLE_CUDA=0
#AC_ARG_WITH([cuda],
#	[AS_HELP_STRING([--without-cuda],
#	[disable CUDA routines.])],
#	[],
#	with_cuda=yes])
#AS_IF([test "x$with_cuda" != xyes], [
	ACX_CUDA([ENABLE_CUDA=1
		AC_DEFINE(HAVE_CUBLAS,1,[Define if you have a CUDA library.]) ],
		AC_MSG_WARN(CUDA library not found. Disabling modules that require it.))
AS_IF([test "$ENABLE_CUDA" != 0], [
	CUDA_INC="-I/usr/local/cuda/include"
])
	#CUDA_INC=`echo $CUDA_LIBS | sed 's/-L.*//g'`
	AC_SUBST(ENABLE_CUDA)
#])

#ENABLE_CUDA=1
#AX_CHECK_CUDA([ENABLE_CUDA=1
#CT_CHECK_CUDA_API([ENABLE_CUDA=1
#	AC_DEFINE(HAVE_CUBLAS,1,[Define for CUDA capability.]) ],
#	AC_MSG_WARN(CUDA library not found.))
#AC_DEFINE([HAVE_CUBLAS],1,[CUDA])
#AC_SUBST(ENABLE_CUDA)
#CT_CHECK_CUDA_SDK
#CDINCPATH = "$CDINCPATH $CUDA_CFLAGS"
#CDLIBPATH = "$CDLIBPATH $CUDA_LDFLAGS"


#CUDA_LIBS="/usr/local/cuda/include"

dnl
dnl Link to the math library
dnl
AC_CHECK_LIB([m],[cos],MATH_LIBS="-lm")
AC_CHECK_LIB([m],[pow],MATH_LIBS="-lm")

dnl
dnl Check for library functions
dnl
AC_CHECK_FUNCS([memset])

dnl
dnl Check for sched_getcpu and sched_setaffinity support
dnl
utmpx_headers="no"
sched_headers="no"
sched_getcpu_support="no"
sched_setaffinity_support="no"
AC_CHECK_HEADERS([utmpx.h],
	[utmpx_headers="yes"],
	[utmpx_headers="no"])
AC_CHECK_HEADERS([sched.h],
	[sched_headers="yes"],   
	[sched_headers="no"])
AC_TRY_LINK([#define _GNU_SOURCE
	#include <utmpx.h>],
	[sched_getcpu();],
	[sched_getcpu_support="yes"])
AC_TRY_LINK([#define _GNU_SOURCE
	#include <sched.h>], 
	[sched_setaffinity(0,0,0);],  
	[sched_setaffinity_support="yes"])
if test "x$utmpx_headers" = "xyes" -a "x$sched_getcpu_support" = "xyes" ; then
	if test "x$sched_headers" = "xyes" -a "x$sched_setaffinity_support" = "xyes" ; then
		AC_DEFINE(LINUX_PLACEMENT,1,[Define if you have support for sched_getcpu() and sched_setaffinity().])
	else
		AC_MSG_WARN(sched_setaffinity() support not found.  Disabling LINUX_PLACEMENT.)
	fi
else
	AC_MSG_WARN(sched_getcpu() support not found.  Disabling LINUX_PLACEMENT.)
fi

dnl
dnl Configure ORBTimers
dnl
ACX_ORBT([], [AC_MSG_ERROR([no valid timer types available.], [$?])])

dnl
dnl Worker threads should be asynchronous
dnl
AC_DEFINE(ASYNC_WORKERS,1,[Define if you want the worker threads to launch asynchronously.])

dnl
dnl Option to enable debugging
dnl
enable_debug="no"
dnl AC_ARG_ENABLE(debug, [AC_HELP_STRING(--enable-debug,turn on debug messages)],
dnl	[ enable_debug="$enableval" ])
if test "x$enable_debug" = "xno"; then
	AC_DEFINE(NDEBUG,1,[Define to disable debugging assert()s.])
fi

dnl
dnl Sets the compilation options
dnl
CONFIG_CC="$CC"
CONFIG_MPICC="$MPICC"
CONFIG_CFLAGS="$CFLAGS $PTHREAD_CFLAGS $OPENMP_CFLAGS $FFTW3_INC $BLAS_INC $CUDA_INC"
CONFIG_LDFLAGS="$LDFLAGS"
CONFIG_LIBS="$MPI_LIBS $PTHREAD_LIBS $LDFLAGS $LIBS $BLAS_LIBS $FFTW3_LIBS $MATH_LIBS $SHMEM_LIBS $CUDA_LIBS"
AC_SUBST(CONFIG_CC)
AC_SUBST(CONFIG_MPICC)
AC_SUBST(CONFIG_CFLAGS)
AC_SUBST(CONFIG_LDFLAGS)
AC_SUBST(CONFIG_LIBS)

dnl
dnl Generate output
dnl
AC_CONFIG_HEADERS([config.h])
AC_OUTPUT(Makefile)

