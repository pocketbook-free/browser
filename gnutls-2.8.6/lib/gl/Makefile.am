## DO NOT EDIT! GENERATED AUTOMATICALLY!
## Process this file with automake to produce Makefile.in.
# Copyright (C) 2002-2009 Free Software Foundation, Inc.
#
# This file is free software, distributed under the terms of the GNU
# General Public License.  As a special exception to the GNU General
# Public License, this file may be distributed as part of a program
# that contains a configuration script generated by Autoconf, under
# the same distribution terms as the rest of that program.
#
# Generated by gnulib-tool.
# Reproduce by: gnulib-tool --import --dir=. --local-dir=gl/override --lib=liblgnu --source-base=gl --m4-base=gl/m4 --doc-base=doc --tests-base=gl/tests --aux-dir=build-aux --with-tests --avoid=alignof-tests --avoid=lseek-tests --lgpl=2 --libtool --macro-prefix=lgl --no-vc-files byteswap c-ctype fseeko func gettext lib-msvc-compat lib-symbol-versions memmem-simple memmove minmax netdb read-file snprintf sockets socklen stdint strcase strverscmp sys_socket sys_stat time_r unistd vasprintf vsnprintf

AUTOMAKE_OPTIONS = 1.5 gnits

SUBDIRS =
noinst_HEADERS =
noinst_LIBRARIES =
noinst_LTLIBRARIES =
EXTRA_DIST =
BUILT_SOURCES =
SUFFIXES =
MOSTLYCLEANFILES = core *.stackdump
MOSTLYCLEANDIRS =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =
SUBDIRS += tests
EXTRA_DIST += m4/gnulib-cache.m4

AM_CPPFLAGS =
AM_CFLAGS =

noinst_LTLIBRARIES += liblgnu.la

liblgnu_la_SOURCES =
liblgnu_la_LIBADD = $(lgl_LTLIBOBJS)
liblgnu_la_DEPENDENCIES = $(lgl_LTLIBOBJS)
EXTRA_liblgnu_la_SOURCES =
liblgnu_la_LDFLAGS = $(AM_LDFLAGS)

## begin gnulib module alignof


EXTRA_DIST += alignof.h

## end   gnulib module alignof

## begin gnulib module alloca-opt

BUILT_SOURCES += $(ALLOCA_H)

# We need the following in order to create <alloca.h> when the system
# doesn't have one that works with the given compiler.
alloca.h: alloca.in.h
	{ echo '/* DO NOT EDIT! GENERATED AUTOMATICALLY! */'; \
	  cat $(srcdir)/alloca.in.h; \
	} > $@-t
	mv -f $@-t $@
MOSTLYCLEANFILES += alloca.h alloca.h-t

EXTRA_DIST += alloca.in.h

## end   gnulib module alloca-opt

## begin gnulib module byteswap

BUILT_SOURCES += $(BYTESWAP_H)

# We need the following in order to create <byteswap.h> when the system
# doesn't have one.
byteswap.h: byteswap.in.h
	{ echo '/* DO NOT EDIT! GENERATED AUTOMATICALLY! */'; \
	  cat $(srcdir)/byteswap.in.h; \
	} > $@-t
	mv -f $@-t $@
MOSTLYCLEANFILES += byteswap.h byteswap.h-t

EXTRA_DIST += byteswap.in.h

## end   gnulib module byteswap

## begin gnulib module c-ctype

liblgnu_la_SOURCES += c-ctype.h c-ctype.c

## end   gnulib module c-ctype

## begin gnulib module close-hook

liblgnu_la_SOURCES += close-hook.c

EXTRA_DIST += close-hook.h

## end   gnulib module close-hook

## begin gnulib module errno

BUILT_SOURCES += $(ERRNO_H)

# We need the following in order to create <errno.h> when the system
# doesn't have one that is POSIX compliant.
errno.h: errno.in.h
	rm -f $@-t $@
	{ echo '/* DO NOT EDIT! GENERATED AUTOMATICALLY! */' && \
	  sed -e 's|@''INCLUDE_NEXT''@|$(INCLUDE_NEXT)|g' \
	      -e 's|@''PRAGMA_SYSTEM_HEADER''@|@PRAGMA_SYSTEM_HEADER@|g' \
	      -e 's|@''NEXT_ERRNO_H''@|$(NEXT_ERRNO_H)|g' \
	      -e 's|@''EMULTIHOP_HIDDEN''@|$(EMULTIHOP_HIDDEN)|g' \
	      -e 's|@''EMULTIHOP_VALUE''@|$(EMULTIHOP_VALUE)|g' \
	      -e 's|@''ENOLINK_HIDDEN''@|$(ENOLINK_HIDDEN)|g' \
	      -e 's|@''ENOLINK_VALUE''@|$(ENOLINK_VALUE)|g' \
	      -e 's|@''EOVERFLOW_HIDDEN''@|$(EOVERFLOW_HIDDEN)|g' \
	      -e 's|@''EOVERFLOW_VALUE''@|$(EOVERFLOW_VALUE)|g' \
	      < $(srcdir)/errno.in.h; \
	} > $@-t
	mv $@-t $@
MOSTLYCLEANFILES += errno.h errno.h-t

EXTRA_DIST += errno.in.h

## end   gnulib module errno

## begin gnulib module float

BUILT_SOURCES += $(FLOAT_H)

# We need the following in order to create <float.h> when the system
# doesn't have one that works with the given compiler.
float.h: float.in.h
	rm -f $@-t $@
	{ echo '/* DO NOT EDIT! GENERATED AUTOMATICALLY! */' && \
	  sed -e 's|@''INCLUDE_NEXT''@|$(INCLUDE_NEXT)|g' \
	      -e 's|@''PRAGMA_SYSTEM_HEADER''@|@PRAGMA_SYSTEM_HEADER@|g' \
	      -e 's|@''NEXT_FLOAT_H''@|$(NEXT_FLOAT_H)|g' \
	      < $(srcdir)/float.in.h; \
	} > $@-t
	mv $@-t $@
MOSTLYCLEANFILES += float.h float.h-t

EXTRA_DIST += float.in.h

## end   gnulib module float

## begin gnulib module fseeko


EXTRA_DIST += fseeko.c stdio-impl.h

EXTRA_liblgnu_la_SOURCES += fseeko.c

## end   gnulib module fseeko

## begin gnulib module gettext

# This is for those projects which use "gettextize --intl" to put a source-code
# copy of libintl into their package. In such projects, every Makefile.am needs
# -I$(top_builddir)/intl, so that <libintl.h> can be found in this directory.
# For the Makefile.ams in other directories it is the maintainer's
# responsibility; for the one from gnulib we do it here.
# This option has no effect when the user disables NLS (because then the intl
# directory contains no libintl.h file) or when the project does not use
# "gettextize --intl".
AM_CPPFLAGS += -I$(top_builddir)/intl

EXTRA_DIST += $(top_srcdir)/build-aux/config.rpath

## end   gnulib module gettext

## begin gnulib module gettext-h

liblgnu_la_SOURCES += gettext.h

## end   gnulib module gettext-h

## begin gnulib module havelib


EXTRA_DIST += $(top_srcdir)/build-aux/config.rpath

## end   gnulib module havelib

## begin gnulib module link-warning

LINK_WARNING_H=$(top_srcdir)/build-aux/link-warning.h

EXTRA_DIST += $(top_srcdir)/build-aux/link-warning.h

## end   gnulib module link-warning

## begin gnulib module lseek


EXTRA_DIST += lseek.c

EXTRA_liblgnu_la_SOURCES += lseek.c

## end   gnulib module lseek

## begin gnulib module memmem-simple


EXTRA_DIST += memmem.c str-two-way.h

EXTRA_liblgnu_la_SOURCES += memmem.c

## end   gnulib module memmem-simple

## begin gnulib module memmove


EXTRA_DIST += memmove.c

EXTRA_liblgnu_la_SOURCES += memmove.c

## end   gnulib module memmove

## begin gnulib module minmax

liblgnu_la_SOURCES += minmax.h

## end   gnulib module minmax

## begin gnulib module netdb

BUILT_SOURCES += $(NETDB_H)

# We need the following in order to create <netdb.h> when the system
# doesn't have one that works with the given compiler.
netdb.h: netdb.in.h
	rm -f $@-t $@
	{ echo '/* DO NOT EDIT! GENERATED AUTOMATICALLY! */'; \
	  sed -e 's|@''INCLUDE_NEXT''@|$(INCLUDE_NEXT)|g' \
	      -e 's|@''PRAGMA_SYSTEM_HEADER''@|@PRAGMA_SYSTEM_HEADER@|g' \
	      -e 's|@''NEXT_NETDB_H''@|$(NEXT_NETDB_H)|g' \
	      -e 's|@''HAVE_NETDB_H''@|$(HAVE_NETDB_H)|g' \
	      -e 's|@''GNULIB_GETADDRINFO''@|$(GNULIB_GETADDRINFO)|g' \
	      -e 's|@''HAVE_STRUCT_ADDRINFO''@|$(HAVE_STRUCT_ADDRINFO)|g' \
	      -e 's|@''HAVE_DECL_FREEADDRINFO''@|$(HAVE_DECL_FREEADDRINFO)|g' \
	      -e 's|@''HAVE_DECL_GAI_STRERROR''@|$(HAVE_DECL_GAI_STRERROR)|g' \
	      -e 's|@''HAVE_DECL_GETADDRINFO''@|$(HAVE_DECL_GETADDRINFO)|g' \
	      -e 's|@''HAVE_DECL_GETNAMEINFO''@|$(HAVE_DECL_GETNAMEINFO)|g' \
	      < $(srcdir)/netdb.in.h; \
	} > $@-t
	mv $@-t $@
MOSTLYCLEANFILES += netdb.h netdb.h-t

EXTRA_DIST += netdb.in.h

## end   gnulib module netdb

## begin gnulib module read-file


EXTRA_DIST += read-file.c read-file.h

EXTRA_liblgnu_la_SOURCES += read-file.c

## end   gnulib module read-file

## begin gnulib module realloc-posix


EXTRA_DIST += realloc.c

EXTRA_liblgnu_la_SOURCES += realloc.c

## end   gnulib module realloc-posix

## begin gnulib module size_max

liblgnu_la_SOURCES += size_max.h

## end   gnulib module size_max

## begin gnulib module snprintf


EXTRA_DIST += snprintf.c

EXTRA_liblgnu_la_SOURCES += snprintf.c

## end   gnulib module snprintf

## begin gnulib module sockets

liblgnu_la_SOURCES += sockets.h sockets.c

EXTRA_DIST += w32sock.h

## end   gnulib module sockets

## begin gnulib module stdbool

BUILT_SOURCES += $(STDBOOL_H)

# We need the following in order to create <stdbool.h> when the system
# doesn't have one that works.
stdbool.h: stdbool.in.h
	rm -f $@-t $@
	{ echo '/* DO NOT EDIT! GENERATED AUTOMATICALLY! */'; \
	  sed -e 's/@''HAVE__BOOL''@/$(HAVE__BOOL)/g' < $(srcdir)/stdbool.in.h; \
	} > $@-t
	mv $@-t $@
MOSTLYCLEANFILES += stdbool.h stdbool.h-t

EXTRA_DIST += stdbool.in.h

## end   gnulib module stdbool

## begin gnulib module stdint

BUILT_SOURCES += $(STDINT_H)

# We need the following in order to create <stdint.h> when the system
# doesn't have one that works with the given compiler.
stdint.h: stdint.in.h
	rm -f $@-t $@
	{ echo '/* DO NOT EDIT! GENERATED AUTOMATICALLY! */'; \
	  sed -e 's/@''HAVE_STDINT_H''@/$(HAVE_STDINT_H)/g' \
	      -e 's|@''INCLUDE_NEXT''@|$(INCLUDE_NEXT)|g' \
	      -e 's|@''PRAGMA_SYSTEM_HEADER''@|@PRAGMA_SYSTEM_HEADER@|g' \
	      -e 's|@''NEXT_STDINT_H''@|$(NEXT_STDINT_H)|g' \
	      -e 's/@''HAVE_SYS_TYPES_H''@/$(HAVE_SYS_TYPES_H)/g' \
	      -e 's/@''HAVE_INTTYPES_H''@/$(HAVE_INTTYPES_H)/g' \
	      -e 's/@''HAVE_SYS_INTTYPES_H''@/$(HAVE_SYS_INTTYPES_H)/g' \
	      -e 's/@''HAVE_SYS_BITYPES_H''@/$(HAVE_SYS_BITYPES_H)/g' \
	      -e 's/@''HAVE_LONG_LONG_INT''@/$(HAVE_LONG_LONG_INT)/g' \
	      -e 's/@''HAVE_UNSIGNED_LONG_LONG_INT''@/$(HAVE_UNSIGNED_LONG_LONG_INT)/g' \
	      -e 's/@''APPLE_UNIVERSAL_BUILD''@/$(APPLE_UNIVERSAL_BUILD)/g' \
	      -e 's/@''BITSIZEOF_PTRDIFF_T''@/$(BITSIZEOF_PTRDIFF_T)/g' \
	      -e 's/@''PTRDIFF_T_SUFFIX''@/$(PTRDIFF_T_SUFFIX)/g' \
	      -e 's/@''BITSIZEOF_SIG_ATOMIC_T''@/$(BITSIZEOF_SIG_ATOMIC_T)/g' \
	      -e 's/@''HAVE_SIGNED_SIG_ATOMIC_T''@/$(HAVE_SIGNED_SIG_ATOMIC_T)/g' \
	      -e 's/@''SIG_ATOMIC_T_SUFFIX''@/$(SIG_ATOMIC_T_SUFFIX)/g' \
	      -e 's/@''BITSIZEOF_SIZE_T''@/$(BITSIZEOF_SIZE_T)/g' \
	      -e 's/@''SIZE_T_SUFFIX''@/$(SIZE_T_SUFFIX)/g' \
	      -e 's/@''BITSIZEOF_WCHAR_T''@/$(BITSIZEOF_WCHAR_T)/g' \
	      -e 's/@''HAVE_SIGNED_WCHAR_T''@/$(HAVE_SIGNED_WCHAR_T)/g' \
	      -e 's/@''WCHAR_T_SUFFIX''@/$(WCHAR_T_SUFFIX)/g' \
	      -e 's/@''BITSIZEOF_WINT_T''@/$(BITSIZEOF_WINT_T)/g' \
	      -e 's/@''HAVE_SIGNED_WINT_T''@/$(HAVE_SIGNED_WINT_T)/g' \
	      -e 's/@''WINT_T_SUFFIX''@/$(WINT_T_SUFFIX)/g' \
	      < $(srcdir)/stdint.in.h; \
	} > $@-t
	mv $@-t $@
MOSTLYCLEANFILES += stdint.h stdint.h-t

EXTRA_DIST += stdint.in.h

## end   gnulib module stdint

## begin gnulib module stdio

BUILT_SOURCES += stdio.h

# We need the following in order to create <stdio.h> when the system
# doesn't have one that works with the given compiler.
stdio.h: stdio.in.h
	rm -f $@-t $@
	{ echo '/* DO NOT EDIT! GENERATED AUTOMATICALLY! */' && \
	  sed -e 's|@''INCLUDE_NEXT''@|$(INCLUDE_NEXT)|g' \
	      -e 's|@''PRAGMA_SYSTEM_HEADER''@|@PRAGMA_SYSTEM_HEADER@|g' \
	      -e 's|@''NEXT_STDIO_H''@|$(NEXT_STDIO_H)|g' \
	      -e 's|@''GNULIB_FPRINTF''@|$(GNULIB_FPRINTF)|g' \
	      -e 's|@''GNULIB_FPRINTF_POSIX''@|$(GNULIB_FPRINTF_POSIX)|g' \
	      -e 's|@''GNULIB_PRINTF''@|$(GNULIB_PRINTF)|g' \
	      -e 's|@''GNULIB_PRINTF_POSIX''@|$(GNULIB_PRINTF_POSIX)|g' \
	      -e 's|@''GNULIB_SNPRINTF''@|$(GNULIB_SNPRINTF)|g' \
	      -e 's|@''GNULIB_SPRINTF_POSIX''@|$(GNULIB_SPRINTF_POSIX)|g' \
	      -e 's|@''GNULIB_VFPRINTF''@|$(GNULIB_VFPRINTF)|g' \
	      -e 's|@''GNULIB_VFPRINTF_POSIX''@|$(GNULIB_VFPRINTF_POSIX)|g' \
	      -e 's|@''GNULIB_VPRINTF''@|$(GNULIB_VPRINTF)|g' \
	      -e 's|@''GNULIB_VPRINTF_POSIX''@|$(GNULIB_VPRINTF_POSIX)|g' \
	      -e 's|@''GNULIB_VSNPRINTF''@|$(GNULIB_VSNPRINTF)|g' \
	      -e 's|@''GNULIB_VSPRINTF_POSIX''@|$(GNULIB_VSPRINTF_POSIX)|g' \
	      -e 's|@''GNULIB_DPRINTF''@|$(GNULIB_DPRINTF)|g' \
	      -e 's|@''GNULIB_VDPRINTF''@|$(GNULIB_VDPRINTF)|g' \
	      -e 's|@''GNULIB_VASPRINTF''@|$(GNULIB_VASPRINTF)|g' \
	      -e 's|@''GNULIB_OBSTACK_PRINTF''@|$(GNULIB_OBSTACK_PRINTF)|g' \
	      -e 's|@''GNULIB_OBSTACK_PRINTF_POSIX''@|$(GNULIB_OBSTACK_PRINTF_POSIX)|g' \
	      -e 's|@''GNULIB_FOPEN''@|$(GNULIB_FOPEN)|g' \
	      -e 's|@''GNULIB_FREOPEN''@|$(GNULIB_FREOPEN)|g' \
	      -e 's|@''GNULIB_FSEEK''@|$(GNULIB_FSEEK)|g' \
	      -e 's|@''GNULIB_FSEEKO''@|$(GNULIB_FSEEKO)|g' \
	      -e 's|@''GNULIB_FTELL''@|$(GNULIB_FTELL)|g' \
	      -e 's|@''GNULIB_FTELLO''@|$(GNULIB_FTELLO)|g' \
	      -e 's|@''GNULIB_FFLUSH''@|$(GNULIB_FFLUSH)|g' \
	      -e 's|@''GNULIB_FCLOSE''@|$(GNULIB_FCLOSE)|g' \
	      -e 's|@''GNULIB_FPUTC''@|$(GNULIB_FPUTC)|g' \
	      -e 's|@''GNULIB_PUTC''@|$(GNULIB_PUTC)|g' \
	      -e 's|@''GNULIB_PUTCHAR''@|$(GNULIB_PUTCHAR)|g' \
	      -e 's|@''GNULIB_FPUTS''@|$(GNULIB_FPUTS)|g' \
	      -e 's|@''GNULIB_PUTS''@|$(GNULIB_PUTS)|g' \
	      -e 's|@''GNULIB_FWRITE''@|$(GNULIB_FWRITE)|g' \
	      -e 's|@''GNULIB_GETDELIM''@|$(GNULIB_GETDELIM)|g' \
	      -e 's|@''GNULIB_GETLINE''@|$(GNULIB_GETLINE)|g' \
	      -e 's|@''GNULIB_PERROR''@|$(GNULIB_PERROR)|g' \
	      -e 's|@''GNULIB_STDIO_H_SIGPIPE''@|$(GNULIB_STDIO_H_SIGPIPE)|g' \
	      -e 's|@''REPLACE_STDIO_WRITE_FUNCS''@|$(REPLACE_STDIO_WRITE_FUNCS)|g' \
	      -e 's|@''REPLACE_FPRINTF''@|$(REPLACE_FPRINTF)|g' \
	      -e 's|@''REPLACE_VFPRINTF''@|$(REPLACE_VFPRINTF)|g' \
	      -e 's|@''REPLACE_PRINTF''@|$(REPLACE_PRINTF)|g' \
	      -e 's|@''REPLACE_VPRINTF''@|$(REPLACE_VPRINTF)|g' \
	      -e 's|@''REPLACE_SNPRINTF''@|$(REPLACE_SNPRINTF)|g' \
	      -e 's|@''HAVE_DECL_SNPRINTF''@|$(HAVE_DECL_SNPRINTF)|g' \
	      -e 's|@''REPLACE_VSNPRINTF''@|$(REPLACE_VSNPRINTF)|g' \
	      -e 's|@''HAVE_DECL_VSNPRINTF''@|$(HAVE_DECL_VSNPRINTF)|g' \
	      -e 's|@''REPLACE_SPRINTF''@|$(REPLACE_SPRINTF)|g' \
	      -e 's|@''REPLACE_VSPRINTF''@|$(REPLACE_VSPRINTF)|g' \
	      -e 's|@''HAVE_DPRINTF''@|$(HAVE_DPRINTF)|g' \
	      -e 's|@''REPLACE_DPRINTF''@|$(REPLACE_DPRINTF)|g' \
	      -e 's|@''HAVE_VDPRINTF''@|$(HAVE_VDPRINTF)|g' \
	      -e 's|@''REPLACE_VDPRINTF''@|$(REPLACE_VDPRINTF)|g' \
	      -e 's|@''HAVE_VASPRINTF''@|$(HAVE_VASPRINTF)|g' \
	      -e 's|@''REPLACE_VASPRINTF''@|$(REPLACE_VASPRINTF)|g' \
	      -e 's|@''HAVE_DECL_OBSTACK_PRINTF''@|$(HAVE_DECL_OBSTACK_PRINTF)|g' \
	      -e 's|@''REPLACE_OBSTACK_PRINTF''@|$(REPLACE_OBSTACK_PRINTF)|g' \
	      -e 's|@''REPLACE_FOPEN''@|$(REPLACE_FOPEN)|g' \
	      -e 's|@''REPLACE_FREOPEN''@|$(REPLACE_FREOPEN)|g' \
	      -e 's|@''REPLACE_FSEEKO''@|$(REPLACE_FSEEKO)|g' \
	      -e 's|@''REPLACE_FSEEK''@|$(REPLACE_FSEEK)|g' \
	      -e 's|@''REPLACE_FTELLO''@|$(REPLACE_FTELLO)|g' \
	      -e 's|@''REPLACE_FTELL''@|$(REPLACE_FTELL)|g' \
	      -e 's|@''REPLACE_FFLUSH''@|$(REPLACE_FFLUSH)|g' \
	      -e 's|@''REPLACE_FCLOSE''@|$(REPLACE_FCLOSE)|g' \
	      -e 's|@''HAVE_DECL_GETDELIM''@|$(HAVE_DECL_GETDELIM)|g' \
	      -e 's|@''HAVE_DECL_GETLINE''@|$(HAVE_DECL_GETLINE)|g' \
	      -e 's|@''REPLACE_GETLINE''@|$(REPLACE_GETLINE)|g' \
	      -e 's|@''REPLACE_PERROR''@|$(REPLACE_PERROR)|g' \
	      -e '/definition of GL_LINK_WARNING/r $(LINK_WARNING_H)' \
	      < $(srcdir)/stdio.in.h; \
	} > $@-t
	mv $@-t $@
MOSTLYCLEANFILES += stdio.h stdio.h-t

EXTRA_DIST += stdio-write.c stdio.in.h

EXTRA_liblgnu_la_SOURCES += stdio-write.c

## end   gnulib module stdio

## begin gnulib module stdlib

BUILT_SOURCES += stdlib.h

# We need the following in order to create <stdlib.h> when the system
# doesn't have one that works with the given compiler.
stdlib.h: stdlib.in.h
	rm -f $@-t $@
	{ echo '/* DO NOT EDIT! GENERATED AUTOMATICALLY! */' && \
	  sed -e 's|@''INCLUDE_NEXT''@|$(INCLUDE_NEXT)|g' \
	      -e 's|@''PRAGMA_SYSTEM_HEADER''@|@PRAGMA_SYSTEM_HEADER@|g' \
	      -e 's|@''NEXT_STDLIB_H''@|$(NEXT_STDLIB_H)|g' \
	      -e 's|@''HAVE_RANDOM_H''@|$(HAVE_RANDOM_H)|g' \
	      -e 's|@''GNULIB_MALLOC_POSIX''@|$(GNULIB_MALLOC_POSIX)|g' \
	      -e 's|@''GNULIB_REALLOC_POSIX''@|$(GNULIB_REALLOC_POSIX)|g' \
	      -e 's|@''GNULIB_CALLOC_POSIX''@|$(GNULIB_CALLOC_POSIX)|g' \
	      -e 's|@''GNULIB_ATOLL''@|$(GNULIB_ATOLL)|g' \
	      -e 's|@''GNULIB_GETLOADAVG''@|$(GNULIB_GETLOADAVG)|g' \
	      -e 's|@''GNULIB_GETSUBOPT''@|$(GNULIB_GETSUBOPT)|g' \
	      -e 's|@''GNULIB_MKDTEMP''@|$(GNULIB_MKDTEMP)|g' \
	      -e 's|@''GNULIB_MKSTEMP''@|$(GNULIB_MKSTEMP)|g' \
	      -e 's|@''GNULIB_PUTENV''@|$(GNULIB_PUTENV)|g' \
	      -e 's|@''GNULIB_RANDOM_R''@|$(GNULIB_RANDOM_R)|g' \
	      -e 's|@''GNULIB_RPMATCH''@|$(GNULIB_RPMATCH)|g' \
	      -e 's|@''GNULIB_SETENV''@|$(GNULIB_SETENV)|g' \
	      -e 's|@''GNULIB_STRTOD''@|$(GNULIB_STRTOD)|g' \
	      -e 's|@''GNULIB_STRTOLL''@|$(GNULIB_STRTOLL)|g' \
	      -e 's|@''GNULIB_STRTOULL''@|$(GNULIB_STRTOULL)|g' \
	      -e 's|@''GNULIB_UNSETENV''@|$(GNULIB_UNSETENV)|g' \
	      -e 's|@''HAVE_ATOLL''@|$(HAVE_ATOLL)|g' \
	      -e 's|@''HAVE_CALLOC_POSIX''@|$(HAVE_CALLOC_POSIX)|g' \
	      -e 's|@''HAVE_GETSUBOPT''@|$(HAVE_GETSUBOPT)|g' \
	      -e 's|@''HAVE_MALLOC_POSIX''@|$(HAVE_MALLOC_POSIX)|g' \
	      -e 's|@''HAVE_MKDTEMP''@|$(HAVE_MKDTEMP)|g' \
	      -e 's|@''HAVE_REALLOC_POSIX''@|$(HAVE_REALLOC_POSIX)|g' \
	      -e 's|@''HAVE_RANDOM_R''@|$(HAVE_RANDOM_R)|g' \
	      -e 's|@''HAVE_RPMATCH''@|$(HAVE_RPMATCH)|g' \
	      -e 's|@''HAVE_SETENV''@|$(HAVE_SETENV)|g' \
	      -e 's|@''HAVE_STRTOD''@|$(HAVE_STRTOD)|g' \
	      -e 's|@''HAVE_STRTOLL''@|$(HAVE_STRTOLL)|g' \
	      -e 's|@''HAVE_STRTOULL''@|$(HAVE_STRTOULL)|g' \
	      -e 's|@''HAVE_STRUCT_RANDOM_DATA''@|$(HAVE_STRUCT_RANDOM_DATA)|g' \
	      -e 's|@''HAVE_SYS_LOADAVG_H''@|$(HAVE_SYS_LOADAVG_H)|g' \
	      -e 's|@''HAVE_UNSETENV''@|$(HAVE_UNSETENV)|g' \
	      -e 's|@''HAVE_DECL_GETLOADAVG''@|$(HAVE_DECL_GETLOADAVG)|g' \
	      -e 's|@''REPLACE_MKSTEMP''@|$(REPLACE_MKSTEMP)|g' \
	      -e 's|@''REPLACE_PUTENV''@|$(REPLACE_PUTENV)|g' \
	      -e 's|@''REPLACE_STRTOD''@|$(REPLACE_STRTOD)|g' \
	      -e 's|@''VOID_UNSETENV''@|$(VOID_UNSETENV)|g' \
	      -e '/definition of GL_LINK_WARNING/r $(LINK_WARNING_H)' \
	      < $(srcdir)/stdlib.in.h; \
	} > $@-t
	mv $@-t $@
MOSTLYCLEANFILES += stdlib.h stdlib.h-t

EXTRA_DIST += stdlib.in.h

## end   gnulib module stdlib

## begin gnulib module strcase


EXTRA_DIST += strcasecmp.c strncasecmp.c

EXTRA_liblgnu_la_SOURCES += strcasecmp.c strncasecmp.c

## end   gnulib module strcase

## begin gnulib module string

BUILT_SOURCES += string.h

# We need the following in order to create <string.h> when the system
# doesn't have one that works with the given compiler.
string.h: string.in.h
	rm -f $@-t $@
	{ echo '/* DO NOT EDIT! GENERATED AUTOMATICALLY! */' && \
	  sed -e 's|@''INCLUDE_NEXT''@|$(INCLUDE_NEXT)|g' \
	      -e 's|@''PRAGMA_SYSTEM_HEADER''@|@PRAGMA_SYSTEM_HEADER@|g' \
	      -e 's|@''NEXT_STRING_H''@|$(NEXT_STRING_H)|g' \
	      -e 's|@''GNULIB_MBSLEN''@|$(GNULIB_MBSLEN)|g' \
	      -e 's|@''GNULIB_MBSNLEN''@|$(GNULIB_MBSNLEN)|g' \
	      -e 's|@''GNULIB_MBSCHR''@|$(GNULIB_MBSCHR)|g' \
	      -e 's|@''GNULIB_MBSRCHR''@|$(GNULIB_MBSRCHR)|g' \
	      -e 's|@''GNULIB_MBSSTR''@|$(GNULIB_MBSSTR)|g' \
	      -e 's|@''GNULIB_MBSCASECMP''@|$(GNULIB_MBSCASECMP)|g' \
	      -e 's|@''GNULIB_MBSNCASECMP''@|$(GNULIB_MBSNCASECMP)|g' \
	      -e 's|@''GNULIB_MBSPCASECMP''@|$(GNULIB_MBSPCASECMP)|g' \
	      -e 's|@''GNULIB_MBSCASESTR''@|$(GNULIB_MBSCASESTR)|g' \
	      -e 's|@''GNULIB_MBSCSPN''@|$(GNULIB_MBSCSPN)|g' \
	      -e 's|@''GNULIB_MBSPBRK''@|$(GNULIB_MBSPBRK)|g' \
	      -e 's|@''GNULIB_MBSSPN''@|$(GNULIB_MBSSPN)|g' \
	      -e 's|@''GNULIB_MBSSEP''@|$(GNULIB_MBSSEP)|g' \
	      -e 's|@''GNULIB_MBSTOK_R''@|$(GNULIB_MBSTOK_R)|g' \
	      -e 's|@''GNULIB_MEMMEM''@|$(GNULIB_MEMMEM)|g' \
	      -e 's|@''GNULIB_MEMPCPY''@|$(GNULIB_MEMPCPY)|g' \
	      -e 's|@''GNULIB_MEMRCHR''@|$(GNULIB_MEMRCHR)|g' \
	      -e 's|@''GNULIB_RAWMEMCHR''@|$(GNULIB_RAWMEMCHR)|g' \
	      -e 's|@''GNULIB_STPCPY''@|$(GNULIB_STPCPY)|g' \
	      -e 's|@''GNULIB_STPNCPY''@|$(GNULIB_STPNCPY)|g' \
	      -e 's|@''GNULIB_STRCHRNUL''@|$(GNULIB_STRCHRNUL)|g' \
	      -e 's|@''GNULIB_STRDUP''@|$(GNULIB_STRDUP)|g' \
	      -e 's|@''GNULIB_STRNDUP''@|$(GNULIB_STRNDUP)|g' \
	      -e 's|@''GNULIB_STRNLEN''@|$(GNULIB_STRNLEN)|g' \
	      -e 's|@''GNULIB_STRPBRK''@|$(GNULIB_STRPBRK)|g' \
	      -e 's|@''GNULIB_STRSEP''@|$(GNULIB_STRSEP)|g' \
	      -e 's|@''GNULIB_STRSTR''@|$(GNULIB_STRSTR)|g' \
	      -e 's|@''GNULIB_STRCASESTR''@|$(GNULIB_STRCASESTR)|g' \
	      -e 's|@''GNULIB_STRTOK_R''@|$(GNULIB_STRTOK_R)|g' \
	      -e 's|@''GNULIB_STRERROR''@|$(GNULIB_STRERROR)|g' \
	      -e 's|@''GNULIB_STRSIGNAL''@|$(GNULIB_STRSIGNAL)|g' \
	      -e 's|@''GNULIB_STRVERSCMP''@|$(GNULIB_STRVERSCMP)|g' \
	      -e 's|@''HAVE_DECL_MEMMEM''@|$(HAVE_DECL_MEMMEM)|g' \
	      -e 's|@''HAVE_MEMPCPY''@|$(HAVE_MEMPCPY)|g' \
	      -e 's|@''HAVE_DECL_MEMRCHR''@|$(HAVE_DECL_MEMRCHR)|g' \
	      -e 's|@''HAVE_RAWMEMCHR''@|$(HAVE_RAWMEMCHR)|g' \
	      -e 's|@''HAVE_STPCPY''@|$(HAVE_STPCPY)|g' \
	      -e 's|@''HAVE_STPNCPY''@|$(HAVE_STPNCPY)|g' \
	      -e 's|@''HAVE_STRCHRNUL''@|$(HAVE_STRCHRNUL)|g' \
	      -e 's|@''HAVE_DECL_STRDUP''@|$(HAVE_DECL_STRDUP)|g' \
	      -e 's|@''HAVE_STRNDUP''@|$(HAVE_STRNDUP)|g' \
	      -e 's|@''HAVE_DECL_STRNDUP''@|$(HAVE_DECL_STRNDUP)|g' \
	      -e 's|@''HAVE_DECL_STRNLEN''@|$(HAVE_DECL_STRNLEN)|g' \
	      -e 's|@''HAVE_STRPBRK''@|$(HAVE_STRPBRK)|g' \
	      -e 's|@''HAVE_STRSEP''@|$(HAVE_STRSEP)|g' \
	      -e 's|@''HAVE_STRCASESTR''@|$(HAVE_STRCASESTR)|g' \
	      -e 's|@''HAVE_DECL_STRTOK_R''@|$(HAVE_DECL_STRTOK_R)|g' \
	      -e 's|@''HAVE_DECL_STRERROR''@|$(HAVE_DECL_STRERROR)|g' \
	      -e 's|@''HAVE_DECL_STRSIGNAL''@|$(HAVE_DECL_STRSIGNAL)|g' \
	      -e 's|@''HAVE_STRVERSCMP''@|$(HAVE_STRVERSCMP)|g' \
	      -e 's|@''REPLACE_MEMMEM''@|$(REPLACE_MEMMEM)|g' \
	      -e 's|@''REPLACE_STRCASESTR''@|$(REPLACE_STRCASESTR)|g' \
	      -e 's|@''REPLACE_STRDUP''@|$(REPLACE_STRDUP)|g' \
	      -e 's|@''REPLACE_STRSTR''@|$(REPLACE_STRSTR)|g' \
	      -e 's|@''REPLACE_STRERROR''@|$(REPLACE_STRERROR)|g' \
	      -e 's|@''REPLACE_STRSIGNAL''@|$(REPLACE_STRSIGNAL)|g' \
	      -e '/definition of GL_LINK_WARNING/r $(LINK_WARNING_H)' \
	      < $(srcdir)/string.in.h; \
	} > $@-t
	mv $@-t $@
MOSTLYCLEANFILES += string.h string.h-t

EXTRA_DIST += string.in.h

## end   gnulib module string

## begin gnulib module strings

BUILT_SOURCES += strings.h

# We need the following in order to create <strings.h> when the system
# doesn't have one that works with the given compiler.
strings.h: strings.in.h
	rm -f $@-t $@
	{ echo '/* DO NOT EDIT! GENERATED AUTOMATICALLY! */' && \
	  sed -e 's|@''INCLUDE_NEXT''@|$(INCLUDE_NEXT)|g' \
	      -e 's|@''PRAGMA_SYSTEM_HEADER''@|@PRAGMA_SYSTEM_HEADER@|g' \
	      -e 's|@''NEXT_STRINGS_H''@|$(NEXT_STRINGS_H)|g' \
	      -e 's|@''HAVE_STRCASECMP''@|$(HAVE_STRCASECMP)|g' \
	      -e 's|@''HAVE_DECL_STRNCASECMP''@|$(HAVE_DECL_STRNCASECMP)|g' \
	      -e '/definition of GL_LINK_WARNING/r $(LINK_WARNING_H)' \
	      < $(srcdir)/strings.in.h; \
	} > $@-t
	mv $@-t $@
MOSTLYCLEANFILES += strings.h strings.h-t

EXTRA_DIST += strings.in.h

## end   gnulib module strings

## begin gnulib module strverscmp


EXTRA_DIST += strverscmp.c

EXTRA_liblgnu_la_SOURCES += strverscmp.c

## end   gnulib module strverscmp

## begin gnulib module sys_socket

BUILT_SOURCES += $(SYS_SOCKET_H)

# We need the following in order to create <sys/socket.h> when the system
# doesn't have one that works with the given compiler.
sys/socket.h: sys_socket.in.h
	@MKDIR_P@ sys
	{ echo '/* DO NOT EDIT! GENERATED AUTOMATICALLY! */'; \
	  sed -e 's|@''INCLUDE_NEXT''@|$(INCLUDE_NEXT)|g' \
	      -e 's|@''PRAGMA_SYSTEM_HEADER''@|@PRAGMA_SYSTEM_HEADER@|g' \
	      -e 's|@''NEXT_SYS_SOCKET_H''@|$(NEXT_SYS_SOCKET_H)|g' \
	      -e 's|@''HAVE_SYS_SOCKET_H''@|$(HAVE_SYS_SOCKET_H)|g' \
	      -e 's|@''GNULIB_CLOSE''@|$(GNULIB_CLOSE)|g' \
	      -e 's|@''GNULIB_SOCKET''@|$(GNULIB_SOCKET)|g' \
	      -e 's|@''GNULIB_CONNECT''@|$(GNULIB_CONNECT)|g' \
	      -e 's|@''GNULIB_ACCEPT''@|$(GNULIB_ACCEPT)|g' \
	      -e 's|@''GNULIB_BIND''@|$(GNULIB_BIND)|g' \
	      -e 's|@''GNULIB_GETPEERNAME''@|$(GNULIB_GETPEERNAME)|g' \
	      -e 's|@''GNULIB_GETSOCKNAME''@|$(GNULIB_GETSOCKNAME)|g' \
	      -e 's|@''GNULIB_GETSOCKOPT''@|$(GNULIB_GETSOCKOPT)|g' \
	      -e 's|@''GNULIB_LISTEN''@|$(GNULIB_LISTEN)|g' \
	      -e 's|@''GNULIB_RECV''@|$(GNULIB_RECV)|g' \
	      -e 's|@''GNULIB_SEND''@|$(GNULIB_SEND)|g' \
	      -e 's|@''GNULIB_RECVFROM''@|$(GNULIB_RECVFROM)|g' \
	      -e 's|@''GNULIB_SENDTO''@|$(GNULIB_SENDTO)|g' \
	      -e 's|@''GNULIB_SETSOCKOPT''@|$(GNULIB_SETSOCKOPT)|g' \
	      -e 's|@''GNULIB_SHUTDOWN''@|$(GNULIB_SHUTDOWN)|g' \
	      -e 's|@''HAVE_WINSOCK2_H''@|$(HAVE_WINSOCK2_H)|g' \
	      -e 's|@''HAVE_WS2TCPIP_H''@|$(HAVE_WS2TCPIP_H)|g' \
	      -e 's|@''HAVE_STRUCT_SOCKADDR_STORAGE''@|$(HAVE_STRUCT_SOCKADDR_STORAGE)|g' \
	      -e 's|@''HAVE_SA_FAMILY_T''@|$(HAVE_SA_FAMILY_T)|g' \
	      -e '/definition of GL_LINK_WARNING/r $(LINK_WARNING_H)' \
	      < $(srcdir)/sys_socket.in.h; \
	} > $@-t
	mv -f $@-t $@
MOSTLYCLEANFILES += sys/socket.h sys/socket.h-t
MOSTLYCLEANDIRS += sys

EXTRA_DIST += sys_socket.in.h

## end   gnulib module sys_socket

## begin gnulib module sys_stat

BUILT_SOURCES += $(SYS_STAT_H)

# We need the following in order to create <sys/stat.h> when the system
# has one that is incomplete.
sys/stat.h: sys_stat.in.h
	@MKDIR_P@ sys
	rm -f $@-t $@
	{ echo '/* DO NOT EDIT! GENERATED AUTOMATICALLY! */'; \
	  sed -e 's|@''INCLUDE_NEXT''@|$(INCLUDE_NEXT)|g' \
	      -e 's|@''PRAGMA_SYSTEM_HEADER''@|@PRAGMA_SYSTEM_HEADER@|g' \
	      -e 's|@''NEXT_SYS_STAT_H''@|$(NEXT_SYS_STAT_H)|g' \
	      -e 's|@''GNULIB_LCHMOD''@|$(GNULIB_LCHMOD)|g' \
	      -e 's|@''GNULIB_LSTAT''@|$(GNULIB_LSTAT)|g' \
	      -e 's|@''HAVE_LCHMOD''@|$(HAVE_LCHMOD)|g' \
	      -e 's|@''HAVE_LSTAT''@|$(HAVE_LSTAT)|g' \
	      -e 's|@''REPLACE_LSTAT''@|$(REPLACE_LSTAT)|g' \
	      -e 's|@''REPLACE_MKDIR''@|$(REPLACE_MKDIR)|g' \
	      -e '/definition of GL_LINK_WARNING/r $(LINK_WARNING_H)' \
	      < $(srcdir)/sys_stat.in.h; \
	} > $@-t
	mv $@-t $@
MOSTLYCLEANFILES += sys/stat.h sys/stat.h-t
MOSTLYCLEANDIRS += sys

EXTRA_DIST += sys_stat.in.h

## end   gnulib module sys_stat

## begin gnulib module time

BUILT_SOURCES += time.h

# We need the following in order to create <time.h> when the system
# doesn't have one that works with the given compiler.
time.h: time.in.h
	rm -f $@-t $@
	{ echo '/* DO NOT EDIT! GENERATED AUTOMATICALLY! */' && \
	  sed -e 's|@''INCLUDE_NEXT''@|$(INCLUDE_NEXT)|g' \
	      -e 's|@''PRAGMA_SYSTEM_HEADER''@|@PRAGMA_SYSTEM_HEADER@|g' \
	      -e 's|@NEXT_TIME_H''@|$(NEXT_TIME_H)|g' \
	      -e 's|@REPLACE_LOCALTIME_R''@|$(REPLACE_LOCALTIME_R)|g' \
	      -e 's|@REPLACE_NANOSLEEP''@|$(REPLACE_NANOSLEEP)|g' \
	      -e 's|@REPLACE_STRPTIME''@|$(REPLACE_STRPTIME)|g' \
	      -e 's|@REPLACE_TIMEGM''@|$(REPLACE_TIMEGM)|g' \
	      -e 's|@SYS_TIME_H_DEFINES_STRUCT_TIMESPEC''@|$(SYS_TIME_H_DEFINES_STRUCT_TIMESPEC)|g' \
	      -e 's|@TIME_H_DEFINES_STRUCT_TIMESPEC''@|$(TIME_H_DEFINES_STRUCT_TIMESPEC)|g' \
	      < $(srcdir)/time.in.h; \
	} > $@-t
	mv $@-t $@
MOSTLYCLEANFILES += time.h time.h-t

EXTRA_DIST += time.in.h

## end   gnulib module time

## begin gnulib module time_r


EXTRA_DIST += time_r.c

EXTRA_liblgnu_la_SOURCES += time_r.c

## end   gnulib module time_r

## begin gnulib module unistd

BUILT_SOURCES += unistd.h

# We need the following in order to create an empty placeholder for
# <unistd.h> when the system doesn't have one.
unistd.h: unistd.in.h
	rm -f $@-t $@
	{ echo '/* DO NOT EDIT! GENERATED AUTOMATICALLY! */'; \
	  sed -e 's|@''HAVE_UNISTD_H''@|$(HAVE_UNISTD_H)|g' \
	      -e 's|@''INCLUDE_NEXT''@|$(INCLUDE_NEXT)|g' \
	      -e 's|@''PRAGMA_SYSTEM_HEADER''@|@PRAGMA_SYSTEM_HEADER@|g' \
	      -e 's|@''NEXT_UNISTD_H''@|$(NEXT_UNISTD_H)|g' \
	      -e 's|@''GNULIB_CHOWN''@|$(GNULIB_CHOWN)|g' \
	      -e 's|@''GNULIB_CLOSE''@|$(GNULIB_CLOSE)|g' \
	      -e 's|@''GNULIB_DUP2''@|$(GNULIB_DUP2)|g' \
	      -e 's|@''GNULIB_ENVIRON''@|$(GNULIB_ENVIRON)|g' \
	      -e 's|@''GNULIB_EUIDACCESS''@|$(GNULIB_EUIDACCESS)|g' \
	      -e 's|@''GNULIB_FCHDIR''@|$(GNULIB_FCHDIR)|g' \
	      -e 's|@''GNULIB_FSYNC''@|$(GNULIB_FSYNC)|g' \
	      -e 's|@''GNULIB_FTRUNCATE''@|$(GNULIB_FTRUNCATE)|g' \
	      -e 's|@''GNULIB_GETCWD''@|$(GNULIB_GETCWD)|g' \
	      -e 's|@''GNULIB_GETDOMAINNAME''@|$(GNULIB_GETDOMAINNAME)|g' \
	      -e 's|@''GNULIB_GETDTABLESIZE''@|$(GNULIB_GETDTABLESIZE)|g' \
	      -e 's|@''GNULIB_GETHOSTNAME''@|$(GNULIB_GETHOSTNAME)|g' \
	      -e 's|@''GNULIB_GETLOGIN_R''@|$(GNULIB_GETLOGIN_R)|g' \
	      -e 's|@''GNULIB_GETPAGESIZE''@|$(GNULIB_GETPAGESIZE)|g' \
	      -e 's|@''GNULIB_GETUSERSHELL''@|$(GNULIB_GETUSERSHELL)|g' \
	      -e 's|@''GNULIB_LCHOWN''@|$(GNULIB_LCHOWN)|g' \
	      -e 's|@''GNULIB_LINK''@|$(GNULIB_LINK)|g' \
	      -e 's|@''GNULIB_LSEEK''@|$(GNULIB_LSEEK)|g' \
	      -e 's|@''GNULIB_READLINK''@|$(GNULIB_READLINK)|g' \
	      -e 's|@''GNULIB_SLEEP''@|$(GNULIB_SLEEP)|g' \
	      -e 's|@''GNULIB_UNISTD_H_SIGPIPE''@|$(GNULIB_UNISTD_H_SIGPIPE)|g' \
	      -e 's|@''GNULIB_WRITE''@|$(GNULIB_WRITE)|g' \
	      -e 's|@''HAVE_DUP2''@|$(HAVE_DUP2)|g' \
	      -e 's|@''HAVE_EUIDACCESS''@|$(HAVE_EUIDACCESS)|g' \
	      -e 's|@''HAVE_FSYNC''@|$(HAVE_FSYNC)|g' \
	      -e 's|@''HAVE_FTRUNCATE''@|$(HAVE_FTRUNCATE)|g' \
	      -e 's|@''HAVE_GETDOMAINNAME''@|$(HAVE_GETDOMAINNAME)|g' \
	      -e 's|@''HAVE_GETDTABLESIZE''@|$(HAVE_GETDTABLESIZE)|g' \
	      -e 's|@''HAVE_GETHOSTNAME''@|$(HAVE_GETHOSTNAME)|g' \
	      -e 's|@''HAVE_GETPAGESIZE''@|$(HAVE_GETPAGESIZE)|g' \
	      -e 's|@''HAVE_GETUSERSHELL''@|$(HAVE_GETUSERSHELL)|g' \
	      -e 's|@''HAVE_LINK''@|$(HAVE_LINK)|g' \
	      -e 's|@''HAVE_READLINK''@|$(HAVE_READLINK)|g' \
	      -e 's|@''HAVE_SLEEP''@|$(HAVE_SLEEP)|g' \
	      -e 's|@''HAVE_DECL_ENVIRON''@|$(HAVE_DECL_ENVIRON)|g' \
	      -e 's|@''HAVE_DECL_GETLOGIN_R''@|$(HAVE_DECL_GETLOGIN_R)|g' \
	      -e 's|@''HAVE_OS_H''@|$(HAVE_OS_H)|g' \
	      -e 's|@''HAVE_SYS_PARAM_H''@|$(HAVE_SYS_PARAM_H)|g' \
	      -e 's|@''REPLACE_CHOWN''@|$(REPLACE_CHOWN)|g' \
	      -e 's|@''REPLACE_CLOSE''@|$(REPLACE_CLOSE)|g' \
	      -e 's|@''REPLACE_FCHDIR''@|$(REPLACE_FCHDIR)|g' \
	      -e 's|@''REPLACE_GETCWD''@|$(REPLACE_GETCWD)|g' \
	      -e 's|@''REPLACE_GETPAGESIZE''@|$(REPLACE_GETPAGESIZE)|g' \
	      -e 's|@''REPLACE_LCHOWN''@|$(REPLACE_LCHOWN)|g' \
	      -e 's|@''REPLACE_LSEEK''@|$(REPLACE_LSEEK)|g' \
	      -e 's|@''REPLACE_WRITE''@|$(REPLACE_WRITE)|g' \
	      -e 's|@''UNISTD_H_HAVE_WINSOCK2_H''@|$(UNISTD_H_HAVE_WINSOCK2_H)|g' \
	      -e '/definition of GL_LINK_WARNING/r $(LINK_WARNING_H)' \
	      < $(srcdir)/unistd.in.h; \
	} > $@-t
	mv $@-t $@
MOSTLYCLEANFILES += unistd.h unistd.h-t

EXTRA_DIST += unistd.in.h

## end   gnulib module unistd

## begin gnulib module vasnprintf


EXTRA_DIST += asnprintf.c float+.h printf-args.c printf-args.h printf-parse.c printf-parse.h vasnprintf.c vasnprintf.h

EXTRA_liblgnu_la_SOURCES += asnprintf.c printf-args.c printf-parse.c vasnprintf.c

## end   gnulib module vasnprintf

## begin gnulib module vasprintf


EXTRA_DIST += asprintf.c vasprintf.c

EXTRA_liblgnu_la_SOURCES += asprintf.c vasprintf.c

## end   gnulib module vasprintf

## begin gnulib module vsnprintf


EXTRA_DIST += vsnprintf.c

EXTRA_liblgnu_la_SOURCES += vsnprintf.c

## end   gnulib module vsnprintf

## begin gnulib module wchar

BUILT_SOURCES += $(WCHAR_H)

# We need the following in order to create <wchar.h> when the system
# version does not work standalone.
wchar.h: wchar.in.h
	rm -f $@-t $@
	{ echo '/* DO NOT EDIT! GENERATED AUTOMATICALLY! */'; \
	  sed -e 's|@''INCLUDE_NEXT''@|$(INCLUDE_NEXT)|g' \
	      -e 's|@''PRAGMA_SYSTEM_HEADER''@|@PRAGMA_SYSTEM_HEADER@|g' \
	      -e 's|@''NEXT_WCHAR_H''@|$(NEXT_WCHAR_H)|g' \
	      -e 's|@''HAVE_WCHAR_H''@|$(HAVE_WCHAR_H)|g' \
	      -e 's|@''GNULIB_BTOWC''@|$(GNULIB_BTOWC)|g' \
	      -e 's|@''GNULIB_WCTOB''@|$(GNULIB_WCTOB)|g' \
	      -e 's|@''GNULIB_MBSINIT''@|$(GNULIB_MBSINIT)|g' \
	      -e 's|@''GNULIB_MBRTOWC''@|$(GNULIB_MBRTOWC)|g' \
	      -e 's|@''GNULIB_MBRLEN''@|$(GNULIB_MBRLEN)|g' \
	      -e 's|@''GNULIB_MBSRTOWCS''@|$(GNULIB_MBSRTOWCS)|g' \
	      -e 's|@''GNULIB_MBSNRTOWCS''@|$(GNULIB_MBSNRTOWCS)|g' \
	      -e 's|@''GNULIB_WCRTOMB''@|$(GNULIB_WCRTOMB)|g' \
	      -e 's|@''GNULIB_WCSRTOMBS''@|$(GNULIB_WCSRTOMBS)|g' \
	      -e 's|@''GNULIB_WCSNRTOMBS''@|$(GNULIB_WCSNRTOMBS)|g' \
	      -e 's|@''GNULIB_WCWIDTH''@|$(GNULIB_WCWIDTH)|g' \
	      -e 's|@''HAVE_WINT_T''@|$(HAVE_WINT_T)|g' \
	      -e 's|@''HAVE_BTOWC''@|$(HAVE_BTOWC)|g' \
	      -e 's|@''HAVE_MBSINIT''@|$(HAVE_MBSINIT)|g' \
	      -e 's|@''HAVE_MBRTOWC''@|$(HAVE_MBRTOWC)|g' \
	      -e 's|@''HAVE_MBRLEN''@|$(HAVE_MBRLEN)|g' \
	      -e 's|@''HAVE_MBSRTOWCS''@|$(HAVE_MBSRTOWCS)|g' \
	      -e 's|@''HAVE_MBSNRTOWCS''@|$(HAVE_MBSNRTOWCS)|g' \
	      -e 's|@''HAVE_WCRTOMB''@|$(HAVE_WCRTOMB)|g' \
	      -e 's|@''HAVE_WCSRTOMBS''@|$(HAVE_WCSRTOMBS)|g' \
	      -e 's|@''HAVE_WCSNRTOMBS''@|$(HAVE_WCSNRTOMBS)|g' \
	      -e 's|@''HAVE_DECL_WCTOB''@|$(HAVE_DECL_WCTOB)|g' \
	      -e 's|@''HAVE_DECL_WCWIDTH''@|$(HAVE_DECL_WCWIDTH)|g' \
	      -e 's|@''REPLACE_MBSTATE_T''@|$(REPLACE_MBSTATE_T)|g' \
	      -e 's|@''REPLACE_BTOWC''@|$(REPLACE_BTOWC)|g' \
	      -e 's|@''REPLACE_WCTOB''@|$(REPLACE_WCTOB)|g' \
	      -e 's|@''REPLACE_MBSINIT''@|$(REPLACE_MBSINIT)|g' \
	      -e 's|@''REPLACE_MBRTOWC''@|$(REPLACE_MBRTOWC)|g' \
	      -e 's|@''REPLACE_MBRLEN''@|$(REPLACE_MBRLEN)|g' \
	      -e 's|@''REPLACE_MBSRTOWCS''@|$(REPLACE_MBSRTOWCS)|g' \
	      -e 's|@''REPLACE_MBSNRTOWCS''@|$(REPLACE_MBSNRTOWCS)|g' \
	      -e 's|@''REPLACE_WCRTOMB''@|$(REPLACE_WCRTOMB)|g' \
	      -e 's|@''REPLACE_WCSRTOMBS''@|$(REPLACE_WCSRTOMBS)|g' \
	      -e 's|@''REPLACE_WCSNRTOMBS''@|$(REPLACE_WCSNRTOMBS)|g' \
	      -e 's|@''REPLACE_WCWIDTH''@|$(REPLACE_WCWIDTH)|g' \
	      -e '/definition of GL_LINK_WARNING/r $(LINK_WARNING_H)' \
	    < $(srcdir)/wchar.in.h; \
	} > $@-t
	mv $@-t $@
MOSTLYCLEANFILES += wchar.h wchar.h-t

EXTRA_DIST += wchar.in.h

## end   gnulib module wchar

## begin gnulib module xsize

liblgnu_la_SOURCES += xsize.h

## end   gnulib module xsize


mostlyclean-local: mostlyclean-generic
	@for dir in '' $(MOSTLYCLEANDIRS); do \
	  if test -n "$$dir" && test -d $$dir; then \
	    echo "rmdir $$dir"; rmdir $$dir; \
	  fi; \
	done; \
	:
