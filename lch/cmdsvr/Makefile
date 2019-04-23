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
exe = testcmdsvr testcmdcli

library=libcmdsvr.so
libheaders=cmdsvr.h
libsources=cmdsvr.c
libobjs=${libsources:.c=.o}

#-------------------------------------------------------------------------------
# Customized flags
#-------------------------------------------------------------------------------
IPATH=-I. \
      	-I$(ROOTDIR)/lch/include \
	-I$(ROOTDIR)/lch/apframe \
	-I$(ROOTDIR)/lch/adapter \
	-I$(ROOTDIR)/lch/util \
	-I$(ROOTDIR)/lch/aplog

LIBDEPS=\
	-L$(ROOTDIR)/lch/apframe -lapfrm \
	-L$(ROOTDIR)/lch/adapter -llchadapter \
	-L$(ROOTDIR)/lch/util -llchutil \
	-L$(ROOTDIR)/lch/aplog -laplog

LIBS=\
	-L$(ROOTDIR)/lch/apframe -lapfrm \
	-L$(ROOTDIR)/lch/cmdsvr -lcmdsvr \
	-L$(ROOTDIR)/lch/adapter -llchadapter \
	-L$(ROOTDIR)/lch/util -llchutil \
	-L$(ROOTDIR)/lch/mm -llchmm \
	-L$(ROOTDIR)/lch/aplog -laplog

SLIBS=\
	$(ROOTDIR)/lch/apframe/libapfrm.a \
	$(ROOTDIR)/lch/cmdsvr/libcmdsvr.a \
	$(ROOTDIR)/lch/adapter/liblchadapter.a \
	$(ROOTDIR)/lch/util/liblchutil.a \
	$(ROOTDIR)/lch/mm/liblchmm.a \
	$(ROOTDIR)/lch/aplog/libaplog.a

ifeq ($(T),1)
ifeq ($(CROSS_COMPILE),tile-)
LIBS+=\
      -ltmc -lgxio

SLIBS+=\
	$(TILERA_ROOT)/tile/usr/lib/libtmc.a \
	$(TILERA_ROOT)/tile/usr/lib/libgxio.a

endif
endif

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

