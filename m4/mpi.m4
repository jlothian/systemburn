dnl @synopsis ACX_MPI([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
dnl
dnl @summary figure out how to compile/link code with MPI
dnl
dnl This macro tries to find out how to compile programs that use MPI
dnl (Message Passing Interface), a standard API for parallel process
dnl communication (see http://www-unix.mcs.anl.gov/mpi/)
dnl
dnl On success, it sets the MPICC, MPICXX, or MPIF77 output variable to
dnl the name of the MPI compiler, depending upon the current language.
dnl (This may just be $CC/$CXX/$F77, but is more often something like
dnl mpicc/mpiCC/mpif77.) It also sets MPI_LIBS to any libraries that are
dnl needed for linking MPI (e.g. -lmpi, if a special
dnl MPICC/MPICXX/MPIF77 was not found).
dnl
dnl If you want to compile everything with MPI, you should set:
dnl
dnl     CC="$MPICC" #OR# CXX="$MPICXX" #OR# F77="$MPIF77"
dnl     LIBS="$MPI_LIBS $LIBS"
dnl
dnl NOTE: The above assumes that you will use $CC (or whatever) for
dnl linking as well as for compiling. (This is the default for automake
dnl and most Makefiles.)
dnl
dnl The user can force a particular library/compiler by setting the
dnl MPICC/MPICXX/MPIF77 and/or MPI_LIBS environment variables.
dnl
dnl ACTION-IF-FOUND is a list of shell commands to run if an MPI
dnl library is found, and ACTION-IF-NOT-FOUND is a list of commands to
dnl run it if it is not found. If ACTION-IF-FOUND is not specified, the
dnl default action will define HAVE_MPI.
dnl
dnl @category InstalledPackages
dnl @author Steven G. Johnson <stevenj@alum.mit.edu>
dnl @author Julian Cummings <cummings@cacr.caltech.edu>
dnl @version 2006-10-13
dnl @license GPLWithACException

AC_DEFUN([ACX_MPI], [
AC_PREREQ(2.50) dnl for AC_LANG_CASE

AC_LANG_CASE([C], [
	AC_REQUIRE([AC_PROG_CC])
	AC_ARG_VAR(MPICC,[MPI C compiler command])
	AC_CHECK_PROGS(MPICC, mpicc hcc mpxlc_r mpxlc mpcc cmpicc, $CC)
	acx_mpi_save_CC="$CC"
	CC="$MPICC"
	AC_SUBST(MPICC)
],
[C++], [
	AC_REQUIRE([AC_PROG_CXX])
	AC_ARG_VAR(MPICXX,[MPI C++ compiler command])
	AC_CHECK_PROGS(MPICXX, mpic++ mpicxx mpiCC hcp mpxlC_r mpxlC mpCC cmpic++, $CXX)
	acx_mpi_save_CXX="$CXX"
	CXX="$MPICXX"
	AC_SUBST(MPICXX)
],
[Fortran 77], [
	AC_REQUIRE([AC_PROG_F77])
	AC_ARG_VAR(MPIF77,[MPI Fortran 77 compiler command])
	AC_CHECK_PROGS(MPIF77, mpif77 hf77 mpxlf mpf77 mpif90 mpf90 mpxlf90 mpxlf95 mpxlf_r cmpifc cmpif90c, $F77)
	acx_mpi_save_F77="$F77"
	F77="$MPIF77"
	AC_SUBST(MPIF77)
],
[Fortran], [
	AC_REQUIRE([AC_PROG_FC])
	AC_ARG_VAR(MPIFC,[MPI Fortran compiler command])
	AC_CHECK_PROGS(MPIFC, mpif90 hf90 mpxlf90 mpxlf95 mpf90 cmpifc cmpif90c, $FC)
	acx_mpi_save_FC="$FC"
	FC="$MPIFC"
	AC_SUBST(MPIFC)
])

if test x = x"$MPI_LIBS"; then
	AC_LANG_CASE([C], [AC_CHECK_FUNC(MPI_Init, [MPI_LIBS=" "])],
		[C++], [AC_CHECK_FUNC(MPI_Init, [MPI_LIBS=" "])],
		[Fortran 77], [AC_MSG_CHECKING([for MPI_Init])
			AC_LINK_IFELSE([AC_LANG_PROGRAM([],[      call MPI_Init])],[MPI_LIBS=" "
				AC_MSG_RESULT(yes)], [AC_MSG_RESULT(no)])],
		[Fortran], [AC_MSG_CHECKING([for MPI_Init])
			AC_LINK_IFELSE([AC_LANG_PROGRAM([],[      call MPI_Init])],[MPI_LIBS=" "
				AC_MSG_RESULT(yes)], [AC_MSG_RESULT(no)])])
fi
AC_LANG_CASE([Fortran 77], [
	if test x = x"$MPI_LIBS"; then
		AC_CHECK_LIB(fmpi, MPI_Init, [MPI_LIBS="-lfmpi"])
	fi
	if test x = x"$MPI_LIBS"; then
		AC_CHECK_LIB(fmpich, MPI_Init, [MPI_LIBS="-lfmpich"])
	fi
],
[Fortran], [
	if test x = x"$MPI_LIBS"; then
		AC_CHECK_LIB(fmpi, MPI_Init, [MPI_LIBS="-lfmpi"])
	fi
	if test x = x"$MPI_LIBS"; then
		AC_CHECK_LIB(mpichf90, MPI_Init, [MPI_LIBS="-lmpichf90"])
	fi
])
if test x = x"$MPI_LIBS"; then
	AC_CHECK_LIB(mpi, MPI_Init, [MPI_LIBS="-lmpi"])
fi
if test x = x"$MPI_LIBS"; then
	AC_CHECK_LIB(mpich, MPI_Init, [MPI_LIBS="-lmpich"])
fi

dnl We have to use AC_TRY_COMPILE and not AC_CHECK_HEADER because the
dnl latter uses $CPP, not $CC (which may be mpicc).
AC_LANG_CASE([C], [if test x != x"$MPI_LIBS"; then
	AC_MSG_CHECKING([for mpi.h])
	AC_TRY_COMPILE([#include <mpi.h>],[],[AC_MSG_RESULT(yes)], [MPI_LIBS=""
		AC_MSG_RESULT(no)])
fi],
[C++], [if test x != x"$MPI_LIBS"; then
	AC_MSG_CHECKING([for mpi.h])
	AC_TRY_COMPILE([#include <mpi.h>],[],[AC_MSG_RESULT(yes)], [MPI_LIBS=""
		AC_MSG_RESULT(no)])
fi],
[Fortran 77], [if test x != x"$MPI_LIBS"; then
	AC_MSG_CHECKING([for mpif.h])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[      include 'mpif.h'])],[AC_MSG_RESULT(yes)], [MPI_LIBS=""
		AC_MSG_RESULT(no)])
fi],
[Fortran], [if test x != x"$MPI_LIBS"; then
	AC_MSG_CHECKING([for mpif.h])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[      include 'mpif.h'])],[AC_MSG_RESULT(yes)], [MPI_LIBS=""
		AC_MSG_RESULT(no)])
fi])

AC_LANG_CASE([C], [CC="$acx_mpi_save_CC"],
	[C++], [CXX="$acx_mpi_save_CXX"],
	[Fortran 77], [F77="$acx_mpi_save_F77"],
	[Fortran], [FC="$acx_mpi_save_FC"])

AC_SUBST(MPI_LIBS)

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x = x"$MPI_LIBS"; then
        $2
        :
else
        ifelse([$1],,[AC_DEFINE(HAVE_MPI,1,[Define if you have the MPI library.])],[$1])
        :
fi
])dnl ACX_MPI
