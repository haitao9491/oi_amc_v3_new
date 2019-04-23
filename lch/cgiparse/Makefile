#
# (C) Copyright 2007-2010
# Hu Chunlin <chunlin.hu@gmail.com>
#
# Makefile - To make my life easier.
#

ROOTDIR = $(shell cd ../.. ; pwd ; cd - > /dev/null)
export ROOTDIR

#-------------------------------------------------------------------------------
# Targets
#-------------------------------------------------------------------------------
exe = cgiparse

library=libcgiparse.so
libheaders=libcgiparse.h
libsources=libcgiparse.c
libobjs=${libsources:.c=.o}

#-------------------------------------------------------------------------------
# Customized flags
#-------------------------------------------------------------------------------
IPATH=-I.

LIBS=\
	-L$(ROOTDIR)/lch/cgiparse -lcgiparse

SLIBS=\
	$(ROOTDIR)/lch/cgiparse/libcgiparse.a

#-------------------------------------------------------------------------------
# The real stuff to do the tricks.
#-------------------------------------------------------------------------------
ifneq ($(DAP_CONFIG_MK_INCLUDED),1)
-include $(ROOTDIR)/config.mk
endif

include $(ROOTDIR)/files.mk

#-------------------------------------------------------------------------------
# Dependency, customized
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Dependency, automatically generated.
#-------------------------------------------------------------------------------
-include $(libobjs:.o=.o.dep)
-include $(exe:=.o.dep)

