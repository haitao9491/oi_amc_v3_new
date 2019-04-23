#
# (C) Copyright 2007-2010
# Hu Chunlin <chunlin.hu@gmail.com>
#
# Makefile - To make my life easier.
#

ROOTDIR = $(shell cd ../ ; pwd ; cd - > /dev/null)
export ROOTDIR

#-------------------------------------------------------------------------------
# Targets
#-------------------------------------------------------------------------------
library=libpsagentfrm.so
libheaders=$(wildcard *.h)
libsources=$(wildcard *.c)
libobjs=${libsources:.c=.o}

#-------------------------------------------------------------------------------
# Customized flags
#-------------------------------------------------------------------------------
IPATH=-I. \
	-I$(ROOTDIR)/lch/include \
	-I$(ROOTDIR)/lch/util \
	-I$(ROOTDIR)/lch/aplog \
	-I$(ROOTDIR)/lch/apframe \
	-I$(ROOTDIR)/lch/adapter \
	-I$(ROOTDIR)/lch/psp_lkmp

LIBS=\
	-L$(ROOTDIR)/lch/util -llchutil \
	-L$(ROOTDIR)/lch/aplog -laplog \
	-L$(ROOTDIR)/lch/apframe -lapfrm \
	-L$(ROOTDIR)/lch/adapter -llchadapter \
	-L$(ROOTDIR)/lch/psp_lkmp -lpsp 

SLIBS=\
	-Wl,--start-group \
	$(ROOTDIR)/lch/util/liblchutil.a \
	$(ROOTDIR)/lch/aplog/libaplog.a \
	$(ROOTDIR)/lch/apframe/libapfrm.a \
	$(ROOTDIR)/lch/adapter/liblchadapter.a \
	$(ROOTDIR)/lch/psp_lkmp/libpsp.a \
	-Wl,--end-group

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
-include $(exe:=.o.dep)

