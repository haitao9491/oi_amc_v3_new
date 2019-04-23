#
# (C) Copyright 2007-2010
# Hu Chunlin <chunlin.hu@gmail.com>
#
# Makefile - To make my life easier.
#

ROOTDIR = $(shell cd .. ; pwd ; cd - > /dev/null)
export ROOTDIR

#-------------------------------------------------------------------------------
# Targets
#-------------------------------------------------------------------------------
ifneq ($(BOARDTYPE),)
exe = getbldinfo
endif

#-------------------------------------------------------------------------------
# Customized flags
#-------------------------------------------------------------------------------
IPATH=-I. \
      	-I$(ROOTDIR)/lch/include \
	-I$(ROOTDIR)/lch/apframe

LIBS=\
	-L$(ROOTDIR)/lch/apframe -lapfrm \
	-L$(ROOTDIR)/lch/util -llchutil \
	-L$(ROOTDIR)/lch/aplog -laplog

SLIBS=\
	-rdynamic \
	-Wl,--allow-multiple-definition \
	-Wl,--whole-archive \
	$(ROOTDIR)/lch/apframe/libapfrm.a \
	$(ROOTDIR)/lch/util/liblchutil.a \
	$(ROOTDIR)/lch/aplog/libaplog.a \
	-Wl,--no-whole-archive

ifeq ($(T),1)
ifeq ($(CROSS_COMPILE),tile-)
LIBS+=\
      -ltmc

SLIBS+=\
	$(TILERA_ROOT)/tile/usr/lib/libtmc.a

endif
endif

#-------------------------------------------------------------------------------
# The real stuff to do the tricks.
#-------------------------------------------------------------------------------
include $(ROOTDIR)/files.mk

#-------------------------------------------------------------------------------
# Dependency, customized
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Dependency, automatically generated.
#-------------------------------------------------------------------------------
-include $(exe:=.o.dep)

