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
exe = svcapp 

#svcapp-sources = $(wildcard *.c)
#svcapp-objs = $(svcapp-sources:.c=.o)

#-------------------------------------------------------------------------------
# Customized flags
#-------------------------------------------------------------------------------
IPATH=-I. \
	-I$(ROOTDIR)/oiamc/app/lkmp_lib 

LIBS=\
	-L$(ROOTDIR)/oiamc/app/lkmp_lib/ -llkmpmif 

SLIBS=\
	  $(ROOTDIR)/oiamc/app/lkmp_lib/liblkmpmif.a 

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
#-include $($(svcapp-objs):=.o.dep)
-include $(exe:=.o.dep)

