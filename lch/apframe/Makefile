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
library=libapfrm.so
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
	-I$(ROOTDIR)/bldinfo

LIBDEPS=\
	-L$(ROOTDIR)/lch/util -llchutil \
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

