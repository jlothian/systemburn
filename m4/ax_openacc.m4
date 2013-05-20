# openacc.m4 serial 4
dnl Copyright (C) 2006-2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl This file can be removed once we assume autoconf >= 2.62.

# _AC_LANG_OPENACC
# ---------------
# Expands to some language dependent source code for testing the presence of
# OpenACC.
AC_DEFUN([_AC_LANG_OPENACC],
[_AC_LANG_DISPATCH([$0], _AC_LANG, $@)])

# _AC_LANG_OPENACC(C)
# ------------------
m4_define([_AC_LANG_OPENACC(C)],
[
#ifndef _OPENACC
 choke me
#endif
#include <openacc.h>
int main () { return acc_get_device_type (); }
])

# _AC_LANG_OPENACC(C++)
# --------------------
m4_copy([_AC_LANG_OPENACC(C)], [_AC_LANG_OPENACC(C++)])

# _AC_LANG_OPENACC(Fortran 77)
# ---------------------------
m4_define([_AC_LANG_OPENACC(Fortran 77)],
[AC_LANG_FUNC_LINK_TRY([acc_get_device_type])])

# _AC_LANG_OPENACC(Fortran)
# ---------------------------
m4_copy([_AC_LANG_OPENACC(Fortran 77)], [_AC_LANG_OPENACC(Fortran)])

# AC_OPENACC
# ---------
# Check which options need to be passed to the C compiler to support OpenACC.
# Set the OPENACC_CFLAGS / OPENACC_CXXFLAGS / OPENACC_FFLAGS variable to these
# options.
# The options are necessary at compile time (so the #pragmas are understood)
# and at link time (so the appropriate library is linked with).
# This macro takes care to not produce redundant options if $CC $CFLAGS already
# supports OpenACC. It also is careful to not pass options to compilers that
# misinterpret them; for example, most compilers accept "-openacc" and create
# an output file called 'penmp' rather than activating OpenACC support.
AC_DEFUN([AX_OPENACC],
[
  OPENACC_[]_AC_LANG_PREFIX[]FLAGS=
  AC_ARG_ENABLE([openacc],
    [AS_HELP_STRING([--disable-openacc], [do not use OpenACC])])
  if test "$enable_openacc" != no; then
    AC_CACHE_CHECK([for $CC option to support OpenACC],
      [ac_cv_prog_[]_AC_LANG_ABBREV[]_openacc],
      [AC_LINK_IFELSE([_AC_LANG_OPENACC],
       [ac_cv_prog_[]_AC_LANG_ABBREV[]_openacc='none needed'],
        [ac_cv_prog_[]_AC_LANG_ABBREV[]_openacc='unsupported'
	  dnl Try these flags:
	  dnl   GCC >= 4.2           -fopenacc
	  dnl   SunPRO C             -xopenacc
	  dnl   Intel C              -openacc
	  dnl   SGI C, PGI C         -mp
	  dnl   Tru64 Compaq C       -omp
	  dnl   IBM C (AIX, Linux)   -qsmp=omp
	  dnl If in this loop a compiler is passed an option that it doesn't
	  dnl understand or that it misinterprets, the AC_LINK_IFELSE test
	  dnl will fail (since we know that it failed without the option),
	  dnl therefore the loop will continue searching for an option, and
	  dnl no output file called 'penmp' or 'mp' is created.
	  for ac_option in -acc; do
	    ac_save_[]_AC_LANG_PREFIX[]FLAGS=$[]_AC_LANG_PREFIX[]FLAGS
	    _AC_LANG_PREFIX[]FLAGS="$[]_AC_LANG_PREFIX[]FLAGS $ac_option"
	    AC_LINK_IFELSE([_AC_LANG_OPENACC],
	      [ac_cv_prog_[]_AC_LANG_ABBREV[]_openacc=$ac_option])
	    _AC_LANG_PREFIX[]FLAGS=$ac_save_[]_AC_LANG_PREFIX[]FLAGS
	    if test "$ac_cv_prog_[]_AC_LANG_ABBREV[]_openacc" != unsupported; then
	     break
	    fi
	  done])])
    case $ac_cv_prog_[]_AC_LANG_ABBREV[]_openacc in #(
      "none needed" | unsupported)
        ;; #(
      *)
        OPENACC_[]_AC_LANG_PREFIX[]FLAGS=$ac_cv_prog_[]_AC_LANG_ABBREV[]_openacc ;;
    esac
  fi
  AC_SUBST([OPENACC_]_AC_LANG_PREFIX[FLAGS])
  if test "x$ac_cv_prog_[]_AC_LANG_ABBREV[]_openacc" = "xunsupported"; then
    m4_default([$2],:)
  else
    m4_default([$1],:)
  fi
])