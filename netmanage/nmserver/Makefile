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
ifeq ($(BOARDTYPE),MPCB_V3)
mexe = nmserver 
nmserver-sources = $(wildcard *.c)
nmserver-objs = $(nmserver-sources:.c=.o)
endif

#-------------------------------------------------------------------------------
# Customized flags
#-------------------------------------------------------------------------------
IPATH=-I. \
	-I$(ROOTDIR)/lch/include \
	-I$(ROOTDIR)/lch/apframe \
	-I$(ROOTDIR)/lch/aplog \
	-I$(ROOTDIR)/lch/util \
	-I$(ROOTDIR)/lch/adapter \
	-I$(ROOTDIR)/bdinfo \
	-I$(ROOTDIR)/netmanage/nmbase \

LIBS=\
     	-L$(ROOTDIR)/lch/adapter -llchadapter \
     	-L$(ROOTDIR)/lch/util -llchutil \
     	-L$(ROOTDIR)/lch/aplog -laplog \
     	-L$(ROOTDIR)/lch/apframe -lapfrm \
	-L$(ROOTDIR)/bdinfo -lbdinfo \
	-L$(ROOTDIR)/netmanage/nmbase -lnmbase \

#	-L$(ROOTDIR)/ethswitch/swlib/ -lswlib

SLIBS=\
      	$(ROOTDIR)/lch/adapter/liblchadapter.a \
      	$(ROOTDIR)/lch/util/liblchutil.a \
      	$(ROOTDIR)/lch/apframe/libapfrm.a \
      	$(ROOTDIR)/lch/aplog/libaplog.a \
	$(ROOTDIR)/bdinfo/libbdinfo.a \
	$(ROOTDIR)/netmanage/nmbase/libnmbase.a \

#	$(ROOTDIR)/ethswitch/swlib/libswlib.a 

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
-include $($(nmserver-objs):=.o.dep)

