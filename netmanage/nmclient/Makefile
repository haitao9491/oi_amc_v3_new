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
exe = nmclient

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
	-I$(ROOTDIR)/oic/oiclib \
	-I$(ROOTDIR)/oic/oicdrv

LIBS=\
     	-L$(ROOTDIR)/lch/adapter -llchadapter \
     	-L$(ROOTDIR)/lch/util -llchutil \
     	-L$(ROOTDIR)/lch/aplog -laplog \
     	-L$(ROOTDIR)/lch/apframe -lapfrm \
	-L$(ROOTDIR)/bdinfo -lbdinfo \
	-L$(ROOTDIR)/netmanage/nmbase -lnmbase 

SLIBS=\
      	$(ROOTDIR)/lch/adapter/liblchadapter.a \
      	$(ROOTDIR)/lch/util/liblchutil.a \
      	$(ROOTDIR)/lch/apframe/libapfrm.a \
      	$(ROOTDIR)/lch/aplog/libaplog.a \
	$(ROOTDIR)/bdinfo/libbdinfo.a \
	$(ROOTDIR)/netmanage/nmbase/libnmbase.a 

ifeq ($(BOARDTYPE),OI_AMC_V3)
LIBS+=\
	-L$(ROOTDIR)/oiamc/app/nmboard_lib/ -lnmboard \
	-L$(ROOTDIR)/oic/oiclib -loiclib
SLIBS+=\
	$(ROOTDIR)/oiamc/app/nmboard_lib/libnmboard.a \
	$(ROOTDIR)/oic/oiclib/liboiclib.a
endif

ifeq ($(BOARDTYPE),OI_AMC_V4)
LIBS+=\
	-L$(ROOTDIR)/oiamc/app/nmboard_lib/ -lnmboard \
	-L$(ROOTDIR)/oiamc/app/fpgamif_lib/ -lfpgamif
SLIBS+=\
	$(ROOTDIR)/oiamc/app/nmboard_lib/libnmboard.a \
	$(ROOTDIR)/oiamc/app/fpgamif_lib/libfpgamif.a 
endif

ifeq ($(BOARDTYPE),EI_AMC_V1)
LIBS+=\
	-L$(ROOTDIR)/eiamc/app/nmboard_lib/ -lnmboard \
	-L$(ROOTDIR)/eiamc/app/fpgamif_lib/ -lfpgamif
SLIBS+=\
	$(ROOTDIR)/eiamc/app/nmboard_lib/libnmboard.a \
	$(ROOTDIR)/eiamc/app/fpgamif_lib/libfpgamif.a
endif

ifeq ($(BOARDTYPE),EIPB_V2)
LIBS+=\
	-L$(ROOTDIR)/eipb/app/nmboard_lib/ -lnmboard \
	-L$(ROOTDIR)/eipb/base/bcm5396lib/ -lswlib \
	-L$(ROOTDIR)/eic/eiclib/ -leiclib \
	-L$(ROOTDIR)/netmanage/nmbase -lnmbase
SLIBS+=\
	$(ROOTDIR)/eipb/app/nmboard_lib/libnmboard.a \
	$(ROOTDIR)/eipb/base/bcm5396lib/libswlib.a \
	$(ROOTDIR)/eic/eiclib/libeiclib.a \
	$(ROOTDIR)/netmanage/nmbase/libnmbase.a
endif

ifeq ($(BOARDTYPE),MACB_V2)
LIBS+=\
	-L$(ROOTDIR)/macb/app/nmboard_lib/ -lnmboard \
	-L$(ROOTDIR)/ethswitch/swlib/ -lswlib
SLIBS+=\
	$(ROOTDIR)/macb/app/nmboard_lib/libnmboard.a \
	$(ROOTDIR)/ethswitch/swlib/libswlib.a 
endif

ifeq ($(BOARDTYPE),MACB_V3)
LIBS+=\
	-L$(ROOTDIR)/macb_v3/app/nmboard_lib/ -lnmboard \
	-L$(ROOTDIR)/ethswitch/swlib/ -lswlib
SLIBS+=\
	$(ROOTDIR)/macb_v3/app/nmboard_lib/libnmboard.a \
	$(ROOTDIR)/ethswitch/swlib/libswlib.a 
endif

ifeq ($(BOARDTYPE),MPCB_V3)
LIBS+=\
	-L$(ROOTDIR)/mpcb/app/nmboard_lib/ -lnmboard \
	-L$(ROOTDIR)/ethswitch/swlib/ -lswlib
SLIBS+=\
	$(ROOTDIR)/mpcb/app/nmboard_lib/libnmboard.a \
	$(ROOTDIR)/ethswitch/swlib/libswlib.a 
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

