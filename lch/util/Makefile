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
library=liblchutil.so
libheaders=$(wildcard *.h)
libsources=$(wildcard *.c)
libobjs=${libsources:.c=.o}

#-------------------------------------------------------------------------------
# Customized flags
#-------------------------------------------------------------------------------
EXTRACFLAGS += -DSSOCK_SYNCED
EXTRACFLAGS += -DCSOCK_SYNCED
EXTRACFLAGS += -DCSSOCK_TX_PFIFO

IPATH=-I. \
      	-I$(ROOTDIR)/lch/include \
	-I$(ROOTDIR)/lch/aplog

LIBDEPS=\
	-L$(ROOTDIR)/lch/aplog -laplog

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

