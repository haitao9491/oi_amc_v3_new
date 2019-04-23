#
# (C) Copyright 2007-2010
#
# Makefile - To make my life easier.
#

ROOTDIR = $(shell cd ../../.. ; pwd ; cd - > /dev/null)
export ROOTDIR

#-------------------------------------------------------------------------------
# Targets
#-------------------------------------------------------------------------------
library=libnmboard.so
libsources=$(wildcard *.c)
libobjs=${libsources:.c=.o}

#-------------------------------------------------------------------------------
# Customized flags
#-------------------------------------------------------------------------------
IPATH=\
	-I../fpgamif_lib	\
	-I../fpgadrv		\
	-I$(ROOTDIR)/netmanage/nmbase			\
	-I$(ROOTDIR)/lch/include				\
	-I$(ROOTDIR)/lch/util					\
	-I$(ROOTDIR)/lch/aplog					\
	-I$(ROOTDIR)/oic/oiclib					\
	-I$(ROOTDIR)/oic/oicdrv

LIBS=\
	-L../fpgamif_lib -lfpgamif	\
	-L$(ROOTDIR)/lch/aplog -laplog \
	-L$(ROOTDIR)/oic/oiclib -loiclib

SLIBS=\
	-rdynamic \
	-Wl,--allow-multiple-definition \
	-Wl,--whole-archive \
	-Wl,--no-whole-archive \
	$(ROOTDIR)/lch/aplog/libaplog.a \
	../fpgamif_lib/libfpgamif.a \
	$(ROOTDIR)/oic/oiclib/liboiclib.a
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

