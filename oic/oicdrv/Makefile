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


drv = oicdrv oic1drv oic2drv oic3drv oic4drv
oicdrv-sources = $(wildcard *.c)
oicdrv-objs = $(oicdrv-sources:.c=.o)

oic1drv-sources = $(wildcard *.c)
oic1drv-objs = $(oic1drv-sources:.c=.o)

oic2drv-sources = $(wildcard *.c)
oic2drv-objs = $(oic2drv-sources:.c=.o)

oic3drv-sources = $(wildcard *.c)
oic3drv-objs = $(oic3drv-sources:.c=.o)

oic4drv-sources = $(wildcard *.c)
oic4drv-objs = $(oic4drv-sources:.c=.o)

#-------------------------------------------------------------------------------
# Customized flags
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# The real stuff to do the tricks.
#-------------------------------------------------------------------------------
ifneq ($(DAP_CONFIG_MK_INCLUDED),1)
-include $(ROOTDIR)/config.mk
endif

include $(ROOTDIR)/drivers.mk

#-------------------------------------------------------------------------------
# Dependency, customized
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Dependency, automatically generated.
#-------------------------------------------------------------------------------
-include $(exe:=.o.dep)

