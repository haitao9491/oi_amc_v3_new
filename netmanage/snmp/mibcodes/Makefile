#
# (C) Copyright 2007-2010
# Hu Chunlin <chunlin.hu@gmail.com>
#
# Makefile - To make my life easier.
#

ROOTDIR = $(shell cd ../../.. ; pwd ; cd - > /dev/null)
export ROOTDIR

#-------------------------------------------------------------------------------
# Targets
#-------------------------------------------------------------------------------
library=snmpMibModule.so
libheaders=$(wildcard *.h)
libsources=$(wildcard *.c)
libobjs=${libsources:.c=.o}

#-------------------------------------------------------------------------------
# Customized flags
#-------------------------------------------------------------------------------
IPATH=-I. \
	  -I./include \
    -I$(ROOTDIR)/lch/include \
	-I$(ROOTDIR)/lch/adapter \
	-I$(ROOTDIR)/lch/util \
	-I$(ROOTDIR)/lch/aplog \
	-I$(ROOTDIR)/lch/apframe \
	-I$(ROOTDIR)/bdinfo \
	-I../../nmbase \
	-I../snmpv58/include

LIBDEPS=\
	-L$(ROOTDIR)/lch/adapter -llchadapter \
	-L$(ROOTDIR)/lch/util -llchutil \
	-L$(ROOTDIR)/lch/aplog -laplog \
	-L../../nmbase -lnmbase \
	-L../snmpv58/lib -lnetsnmpmibs

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

