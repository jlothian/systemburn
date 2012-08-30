dnl @synopsis ACX_ORBT([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
dnl
dnl Configure ORBTimers.
dnl First, check for hardware timers (x86 or PPC) by looking at the host system.
dnl If the host does not appear to support either type of hardware timer,
dnl check for MPI_Wtime first, then check for gettimeofday.
dnl
dnl For the first discovered timer, the appropriate label will be defined to
dnl configure orbtimer.h. To manually specify a timer type, use $ORB_TIMER or
dnl pass --with-orbtimer=<timer>.
dnl
dnl ACTION-IF-FOUND is a list of shell commands to run if a timer
dnl is found, and ACTION-IF-NOT-FOUND is a list of commands to
dnl run if one is not found. If ACTION-IF-FOUND is not specified, the
dnl default action will define HAVE_TIMERS.
dnl
dnl @category InstalledPackages
dnl @author Ross Glandon <glandonsr@ornl.gov>
dnl @version 2012-07-19
dnl @license GPLWithACException

AC_DEFUN([ACX_ORBT], [
AC_PREREQ(2.50)
AC_REQUIRE([AC_CANONICAL_HOST])[]dnl
acx_orbt_ok=no

AC_ARG_WITH(orbtimer,
	[AC_HELP_STRING([--with-orbtimer=<timer>], [use ORBTimer <timer>])])
case $with_orbtimer in
	yes | "") ;;
	no) ;;
	TIMER_*) ORB_TIMER="$with_orbtimer" ;;
	*) ORB_TIMER="TIMER_$with_orbtimer" ;;
esac

# If ORB_TIMER environment variable is set, check it for a valid timer.
if test $acx_orbt_ok = no; then
if test "x$ORB_TIMER" != x; then
	AS_CASE([$ORB_TIMER],
		[TIMER_X86_64], [
			ACX_TIMER_X86_64([acx_orbt_ok=yes
					 AC_DEFINE([TIMER_X86_64], 1, [Define to 1 for the system to use x86_64 timers.])],
					 [ORB_TIMER=""])
		],
		[TIMER_PPC64], [
			ACX_TIMER_PPC64([acx_orbt_ok=yes
				AC_DEFINE([TIMER_PPC64], 1, [Define to 1 for the system to use PowerPC64 timers.])],
				[ORB_TIMER=""])
		],
		[TIMER_MPI_WTIME], [
			# Test for MPI_Wtime().
			save_LIBS="$LIBS"
			save_CC="$CC"
			if test "x$MPICC" != x; then
				CC="$MPICC"
			fi
			if test "x$MPI_LIBS" != x; then
				LIBS="$MPI_LIBS $LIBS"
			fi
			AC_CHECK_FUNC([MPI_Wtime], [
				acx_orbt_ok=yes
				AC_DEFINE([TIMER_MPI_WTIME], 1, [Define to 1 for the system to use MPIWtime timers.])],
				[ORB_TIMER=""])
			LIBS="$save_LIBS"
			CC="$save_CC"
		],
		[TIMER_GTD], [
			# Test for getttimeofday().
			AC_CHECK_FUNC([gettimeofday], [
				acx_orbt_ok=yes
				AC_DEFINE([TIMER_GTD], 1, [Define to 1 for the system to use GTD timers.])],
				[ORB_TIMER=""])
		],
		[
			AC_MSG_WARN([specified timer, $ORB_TIMER, unavailable, checking for others])
		]
	)
fi
fi

# Check for x86 hardware timers.
if test "$acx_orbt_ok" = no; then
	ACX_TIMER_X86_64([acx_orbt_ok=yes
			 AC_DEFINE([TIMER_X86_64], 1, [Define to 1 for the system to use x86_64 timers.])],
			 [ORB_TIMER=""])
fi

# Check for PPC hardware timers.
if test "$acx_orbt_ok" = no; then
	ACX_TIMER_PPC64([acx_orbt_ok=yes
			AC_DEFINE([TIMER_PPC64], 1, [Define to 1 for the system to use PowerPC64 timers.])],
			[ORB_TIMER=""])
fi

# Check for MPI_Wtime() using pre-defined values $MPICC and $MPI_LIBS
if test "$acx_orbt_ok" = no; then
	# Test for MPI_Wtime().
	save_LIBS="$LIBS"
	LIBS="$MPI_LIBS $LIBS"
	save_CC="$CC"
	CC="$MPICC"
	AC_CHECK_FUNC([MPI_Wtime], [
		acx_orbt_ok=yes
		AC_DEFINE([TIMER_MPI_WTIME], 1, [Define to 1 for the system to use MPIWtime timers.])],
		[ORB_TIMER=""]
	)
	LIBS="$save_LIBS"
	CC="$save_CC"
fi

# Check for gettimeofday().
if test "$acx_orbt_ok" = no; then
	# Test for getttimeofday().
	AC_CHECK_FUNC([gettimeofday], [
		acx_orbt_ok=yes
		AC_DEFINE([TIMER_GTD], 1, [Define to 1 for the system to use GTD timers.])],
		[ORB_TIMER=""]
	)
fi

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$acx_orbt_ok" = xyes; then
        ifelse([$1],,AC_DEFINE(HAVE_TIMERS,1,[Define if you have a valid timer.]),[$1])
        :
else
        acx_orbt_ok=no
        $2
fi
])dnl ACX_ORBT


dnl
dnl Checks for x86_64 timers.
dnl Checks that the system is an x86 machine, then attempts to compile
dnl a small program using the rdtsc assembly instruction.
dnl
AC_DEFUN([ACX_TIMER_X86_64], [
AC_PREREQ(2.50)
AC_REQUIRE([AC_CANONICAL_HOST])[]dnl

acx_timer_x86_64_ok=no

# Test for x86_64 hardware timers.
AC_MSG_CHECKING([for x86_64 hardware timers])

# Check if the host system is an x86_64 machine.
AS_CASE([$host_cpu],
	[x86 | i*86 | x86_64],

dnl A Note on the test program below:
dnl Adding inline assembly seems to require some rather messy things for it to get through both autoconf/m4 and the shell...
dnl First, I've replaced the '#' on the define statement with '@%:@' so that the shell cannot mistake it as a comment.
dnl Also, the line "salq $32, %%rdx \n\n" has been modified so that neither autoconf nor shell try to do argument
dnl substitution with the '$3'. So, autoconf converts the '@S|@' to '$' and '\' then escapes evaluation by the shell.

	# Test that the required assembly instructions are available by compiling a small test program.
	[AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM(
			[[@%:@define ORB_read(T)  __asm__ __volatile__ ( "  \n\t" \
							"mfence             \n\t"  \
							"rdtsc              \n\t"  \
							"movl %%eax,%%eax   \n\t"  \
							"salq \@S|@32,%%rdx \n\t"  \
							"orq %%rdx,%%rax    \n\t" : "=a" (T) : : "%rdx")]],
			[[unsigned long long T1; ORB_read(T1);]])],
		[acx_timer_x86_64_ok=yes], [])],
	[])

# Print result of check.
AC_MSG_RESULT([$acx_timer_x86_64_ok])

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$acx_timer_x86_64_ok" = xyes; then
        ifelse([$1],,AC_DEFINE([TIMER_X86_64], 1, [Define to 1 for the system to use x86_64 timers.]),[$1])
        :
else
        $2
fi
])dnl ACX_TIMER_X86_64


dnl
dnl Checks for PowerPC timers.
dnl Checks that the system is an ppc machine, then attempts to compile
dnl a small program using the mftb assembly instruction.
dnl
AC_DEFUN([ACX_TIMER_PPC64], [
AC_PREREQ(2.50)
AC_REQUIRE([AC_CANONICAL_HOST])[]dnl

acx_timer_ppc64_ok=no

# Test for PowerPC64 hardware timers.
AC_MSG_CHECKING([for PowerPC64 hardware timers])

# Check if the host system is a powerpc machine.
AS_CASE([$host_cpu],
	[powerpc | powerpc64 | powerpc64le | powerpcle | ppcbe],
	[AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM(
			[[@%:@define ORB_read(T)  __asm__ __volatile__ ( "  \n\t" \
							"mftb %0            \n\t" : "=r" (T) )]],
			[[unsigned long T1; ORB_read(T1);]])],
		[acx_timer_ppc64_ok=yes], [])],
	[])

# Print result of check.
AC_MSG_RESULT([$acx_timer_ppc64_ok])

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$acx_timer_ppc64_ok" = xyes; then
        ifelse([$1],,AC_DEFINE([TIMER_PPC64], 1, [Define to 1 for the system to use PowerPC64 timers.]),[$1])
        :
else
        $2
fi
])dnl ACX_TIMER_PPC64