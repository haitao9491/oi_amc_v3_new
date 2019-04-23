#
# (C) Copyright 2007-2010
# Hu Chunlin <chunlin.hu@gmail.com>
#
# Makefile - To make my life easier.
#

ROOTDIR = $(CURDIR)
export ROOTDIR

-include $(ROOTDIR)/config.mk

ifeq ($(D),)
	D = 0
	export D
endif

BOARDTYPE=OI_AMC_V3
export BOARDTYPE

ifeq ($(D),0)
BLDENVTAR=$(shell echo "$(ROOTDIR)/$(BOARDTYPE)-BLDENV-`./scripts/hostenv.sh`.tar")
else
BLDENVTAR=$(shell echo "$(ROOTDIR)/$(BOARDTYPE)-BLDENV-`./scripts/hostenv.sh`_DEBUG.tar")
endif
export BLDENVTAR

GITURL0=$(shell grep -Ee "^[[:space:]]{1}url = .*$$" .git/config | \
	awk '{print $$3;}' )
GITREPNAME=$(shell echo $(GITURL0) | \
	awk -F '/' '{print $$NF;}' )
GITBAREMODE=$(shell if echo $(GITURL0) | \
	grep -qEe "\.git$$"; then echo 1; else echo 0; fi; )
ifeq ($(GITBAREMODE),1)
	GITURL=$(subst /$(GITREPNAME),,$(GITURL0))/
	GITURLSUFFIX=".git"
else
	GITURL=$(shell echo $(GITURL0) | sed s/\\/$$//g)/
	GITURLSUFFIX=
endif
export GITURL GITBAREMODE GITURLSUFFIX

SUBDIRS = lch psagentfrm psagentfrm_lkmp bdinfo bldinfo oic oiamc app netmanage

include $(CURDIR)/dirs.mk

populate: populate_repository
	@echo > /dev/null

env: compile_environment
	@cd bldenv; \
	if [ -f $(BLDENVTAR) ]; then rm -f $(BLDENVTAR); fi; \
	tar czf $(BLDENVTAR) ./*; \
	cd ..; \
	rm -rf bldenv

installenv: env
	@scp $(BLDENVTAR) 172.168.10.62:/data1/BuildENV/`basename $(BLDENVTAR)`.`date '+%Y%m%d%H%M%S'`
	@cp $(BLDENVTAR) `basename $(BLDENVTAR)`.`date '+%Y%m%d%H%M%S'`

updenv: update_environment
	@echo > /dev/null

runtime: runtime_environment
	@find runtime -type f | xargs md5sum > MD5SUM
	@if [ -f runtime.tgz ]; then rm -f runtime.tgz; fi;
	@tar czf runtime.tgz ./runtime/* ./MD5SUM;
	@rm -rf runtime MD5SUM
