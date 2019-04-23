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
library=liboiclib.so

ifeq ($(BOARDTYPE), XSCB_V2)
libheaders=$(wildcard *.h)
libsources=$(wildcard *.c)
else
libheaders=oiclib.h sgfplib.h grpsm.h
libsources=oiclib.c sgfplib.c grpsm.c
endif

libobjs=${libsources:.c=.o}
#-------------------------------------------------------------------------------
# Customized flags
#-------------------------------------------------------------------------------
IPATH=-I. \
	-I$(ROOTDIR)/xscb/base/clovesdrv \
	-I$(ROOTDIR)/oic/oicdrv \
	-I$(ROOTDIR)/lch/include \
	-I$(ROOTDIR)/lch/util \
	-I$(ROOTDIR)/lch/aplog

ifeq ($(BOARDTYPE), XSCB_V2)
IPATH=-I. \
	-I$(ROOTDIR)/xscb/base/clovesdrv \
	-I$(ROOTDIR)/oic/oicdrv \
	-I$(ROOTDIR)/lch/include \
	-I$(ROOTDIR)/lch/util \
	-I$(ROOTDIR)/lch/aplog \
	-I$(ROOTDIR)/gfpnetmanager/common \
	-I$(ROOTDIR)/sdhnetmanager/common \
	-I$(ROOTDIR)/ethswitch/bcm56840drvitf

else
IPATH=-I. \
	-I$(ROOTDIR)/xscb/base/clovesdrv \
	-I$(ROOTDIR)/oic/oicdrv \
	-I$(ROOTDIR)/lch/include \
	-I$(ROOTDIR)/lch/util \
	-I$(ROOTDIR)/lch/aplog

endif

LIBS=\
	-L$(ROOTDIR)/lch/util -llchutil \
	-L$(ROOTDIR)/lch/aplog -laplog

SLIBS=\
	$(ROOTDIR)/lch/util/liblchutil.a \
	$(ROOTDIR)/lch/aplog/libaplog.a

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

