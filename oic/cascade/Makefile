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
exe = cascadeapp

#-------------------------------------------------------------------------------
# Customized flags
#-------------------------------------------------------------------------------
IPATH=-I. \
	-I../oicdrv \
	-I../oiclib \
	-I../../lch/include \
	-I../../lch/util \
	-I../../lch/aplog \
	-I../../lch/apframe

LIBS=\
	-L../oiclib -loiclib \
	-L../../lch/util -llchutil \
	-L../../lch/aplog -laplog \
	-L../../lch/apframe -lapfrm

SLIBS=\
	../oiclib/liboiclib.a  \
	../../lch/util/liblchutil.a \
	../../lch/aplog/libaplog.a \
	../../lch/apframe/libapfrm.a

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

