dnl Process this file with autoconf to produce a configure script.
# Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009
#    Free Software Foundation, Inc.
#
# Author: Nikos Mavrogiannopoulos, Simon Josefsson
#
# This file is part of GNUTLS-EXTRA.
#
# GNUTLS-EXTRA is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 3 of the
# License, or (at your option) any later version.
#
# GNUTLS-EXTRA is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNUTLS-EXTRA; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.

AC_PREREQ(2.61)
AC_INIT([libgnutls-extra], [2.8.6], [bug-gnutls@gnu.org])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.10 -Wall -Werror -Wno-override])
AM_CONFIG_HEADER(config.h)

AC_PROG_CC
xgl_EARLY

AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

LIBGNUTLS_HOOKS
LIBGNUTLS_EXTRA_HOOKS

xgl_INIT

LIBGNUTLS_EXTRA_LIBS="-L${libdir} -lgnutls-extra $LZO_LIBS $LIBGNUTLS_LIBS"
LIBGNUTLS_EXTRA_CFLAGS="-I${includedir}"
AC_SUBST(LIBGNUTLS_EXTRA_LIBS)
AC_SUBST(LIBGNUTLS_EXTRA_CFLAGS)

# Finish things from ../configure.ac.
AC_SUBST([WARN_CFLAGS])
AC_SUBST([WERROR_CFLAGS])

AC_CONFIG_FILES([
  Makefile
  gnutls-extra.pc
  gl/Makefile
  includes/Makefile
])
AC_OUTPUT
