#******************************************************************************
#
#   Copyright (C) 1999-2007, International Business Machines
#   Corporation and others.  All Rights Reserved.
#
#******************************************************************************
## Makefile.in for ICU stubdata
## Stephen F. Booth

## Source directory information
srcdir = ../../source/stubdata
top_srcdir = ../../source

top_builddir = ..

## All the flags and other definitions are included here.
include $(top_builddir)/icudefs.mk

## Build directory information
subdir = stubdata

## Extra files to remove for 'make clean'
CLEANFILES = *~ $(DEPS) $(IMPORT_LIB) $(MIDDLE_IMPORT_LIB) $(FINAL_IMPORT_LIB)

## Target information

TARGET_STUBNAME=$(DATA_STUBNAME)

ifneq ($(ENABLE_STATIC),)
TARGET = $(STUBDATA_LIBDIR)$(LIBSICU)$(TARGET_STUBNAME)$(ICULIBSUFFIX).$(A)
endif


ifneq ($(ENABLE_SHARED),)
SO_TARGET = $(STUBDATA_LIBDIR)$(LIBICU)$(TARGET_STUBNAME)$(ICULIBSUFFIX)$(STUB_SUFFIX).$(SO)
ALL_SO_TARGETS = $(FINAL_SO_TARGET) $(MIDDLE_SO_TARGET) $(SO_TARGET) $(BATCH_STUB_TARGET) $(SHARED_OBJECT)
endif


ALL_TARGETS = $(TARGET) $(ALL_SO_TARGETS)

DYNAMICCPPFLAGS = $(SHAREDLIBCPPFLAGS)
DYNAMICCFLAGS = $(SHAREDLIBCFLAGS)
DYNAMICCXXFLAGS = $(SHAREDLIBCXXFLAGS)

ifneq ($(top_builddir),$(top_srcdir))
CPPFLAGS += -I$(top_builddir)/common
endif
CPPFLAGS += -I$(top_srcdir)/common $(LIBCPPFLAGS)
LDFLAGS += $(LDFLAGSICUDT)

OBJECTS = stubdata.o

## Header files to install
HEADERS = 

STATIC_OBJECTS = $(OBJECTS:.o=.$(STATIC_O))

DEPS = $(OBJECTS:.o=.d)

-include Makefile.local

## List of phony targets
.PHONY : all all-local install install-local clean clean-local	\
distclean distclean-local install-library dist	\
dist-local check check-local

## Clear suffix list
.SUFFIXES :

## List of standard targets
all: all-local
install: install-local
clean: clean-local
distclean : distclean-local
dist: dist-local
check: all check-local

all-local: $(ALL_TARGETS)

install-local: install-library

install-library: all-local
	$(MKINSTALLDIRS) $(DESTDIR)$(libdir)
ifneq ($(ENABLE_STATIC),)
	$(INSTALL-L) $(TARGET) $(DESTDIR)$(libdir)
endif
ifneq ($(ENABLE_SHARED),)
	$(INSTALL-L) $(FINAL_SO_TARGET) $(DESTDIR)$(libdir)
ifneq ($(FINAL_SO_TARGET),$(SO_TARGET))
	cd $(DESTDIR)$(libdir) && $(RM) $(notdir $(SO_TARGET)) && ln -s $(notdir $(FINAL_SO_TARGET)) $(notdir $(SO_TARGET))
ifneq ($(FINAL_SO_TARGET),$(MIDDLE_SO_TARGET))
	cd $(DESTDIR)$(libdir) && $(RM) $(notdir $(MIDDLE_SO_TARGET)) && ln -s $(notdir $(FINAL_SO_TARGET)) $(notdir $(MIDDLE_SO_TARGET))
endif
endif
ifneq ($(IMPORT_LIB_EXT),)
	$(INSTALL-L) $(FINAL_IMPORT_LIB) $(DESTDIR)$(libdir)
ifneq ($(IMPORT_LIB),$(FINAL_IMPORT_LIB))
	cd $(DESTDIR)$(libdir) && $(RM) $(notdir $(IMPORT_LIB)) && ln -s $(notdir $(FINAL_IMPORT_LIB)) $(notdir $(IMPORT_LIB))
endif
ifneq ($(MIDDLE_IMPORT_LIB),$(FINAL_IMPORT_LIB))
	cd $(DESTDIR)$(libdir) && $(RM) $(notdir $(MIDDLE_IMPORT_LIB)) && ln -s $(notdir $(FINAL_IMPORT_LIB)) $(notdir $(MIDDLE_IMPORT_LIB))
endif
endif
endif

dist-local:

clean-local:
	test -z "$(CLEANFILES)" || $(RMV) $(CLEANFILES)
	$(RMV) $(OBJECTS) $(STATIC_OBJECTS) $(ALL_TARGETS)

distclean-local: clean-local
	$(RMV) Makefile

check-local:

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status
	cd $(top_builddir) \
	&& CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status

../common/unicode/platform.h: $(srcdir)/../common/unicode/platform.h.in $(top_builddir)/config.status
	cd $(top_builddir) \
	 && CONFIG_FILES=common/unicode/platform.h CONFIG_HEADERS= $(SHELL) ./config.status

ifneq ($(ENABLE_STATIC),)
$(TARGET): $(STATIC_OBJECTS)
	$(AR) $(ARFLAGS) $(AR_OUTOPT)$@ $^
	$(RANLIB) $@
endif

ifneq ($(ENABLE_SHARED),)
$(SHARED_OBJECT): $(OBJECTS)
	$(SHLIB.c) $(LD_SONAME) $(OUTOPT)$@ $^ $(LIBS)

ifeq ($(OS390BATCH),1)
$(BATCH_STUB_TARGET): $(OBJECTS)
	$(SHLIB.c) $(LD_SONAME) $(OUTOPT)$@ $^ $(LIBS)
endif   # OS390BATCH
endif   # ENABLE_SHARED

ifeq (,$(MAKECMDGOALS))
-include $(DEPS)
else
ifneq ($(patsubst %clean,,$(MAKECMDGOALS)),)
-include $(DEPS)
endif
endif

