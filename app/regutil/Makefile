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
exe = reg

ifeq ($(BOARDTYPE),MACB_V2)
exe =
endif
ifeq ($(BOARDTYPE),MPCB_V3)
exe =
endif
ifeq ($(BOARDTYPE),XIPB_V1)
exe =
endif

#-------------------------------------------------------------------------------
# Customized flags
#-------------------------------------------------------------------------------
ifeq ($(BOARDTYPE),OI_AMC_V3)
IPATH=-I. \
	-I$(ROOTDIR)/oiamc/base/clovesdrv
endif
ifeq ($(BOARDTYPE),EI_AMC_V1)
IPATH=-I. \
	-I$(ROOTDIR)/eiamc/base/clovesdrv
endif
ifeq ($(BOARDTYPE),EIPB_V2)
IPATH=-I. \
	-I$(ROOTDIR)/eipb/base/clovesdrv
endif
ifeq ($(BOARDTYPE),OIPC_V1)
IPATH=-I. \
	-I$(ROOTDIR)/oipc/base/clovesdrv
endif
ifeq ($(BOARDTYPE),EIPC_V1)
IPATH=-I. \
	-I$(ROOTDIR)/eipc/base/clovesdrv
endif
ifeq ($(BOARDTYPE),EIPC_V2)
IPATH=-I. \
	-I$(ROOTDIR)/eipc/base/clovesdrv
endif
ifeq ($(BOARDTYPE),MUCB_V1)
IPATH=-I. \
	-I$(ROOTDIR)/mucb_v1/base/clovesdrv
endif
ifeq ($(BOARDTYPE),LBB_V1)
IPATH=-I. \
	-I$(ROOTDIR)/lbb_v1/base/clovesdrv
endif
ifeq ($(BOARDTYPE),OI_AMC_V4)
IPATH=-I. \
	-I$(ROOTDIR)/oiamc/base/clovesdrv
endif
ifeq ($(BOARDTYPE),MACB_V3)
IPATH=-I. \
	-I$(ROOTDIR)/macb_v3/base/clovesdrv
endif
ifeq ($(BOARDTYPE),XSCB_V2)
IPATH=-I. \
	-I$(ROOTDIR)/xscb/base/clovesdrv
endif
ifeq ($(BOARDTYPE),OTAP_V1)
IPATH=-I. \
	-I$(ROOTDIR)/bypass/base/clovesdrv
endif
ifeq ($(BOARDTYPE),ETAP_V1)
IPATH=-I. \
	-I$(ROOTDIR)/bypass/base/clovesdrv
endif
ifeq ($(BOARDTYPE),XPPP_V2)
IPATH=-I. \
	  -I$(ROOTDIR)/xppp_v2/base/clovesdrv
endif
ifeq ($(BOARDTYPE),LCEA_V1)
IPATH=-I. \
	-I$(ROOTDIR)/lcea/base/clovesdrv
endif

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

