#
# (C) Copyright 2010
# Hu Chunlin <chunlin.hu@gmail.com>
#
# files.mk - To make my life easier.
#

LOCALINSTALLPATH=$(HOME)/local

MAKEFLAGS += --no-print-directory -r

ifeq ($(D),)
	D = 0
	export D
endif

ifeq ($(T),1)
CROSSPREFIX=$(CROSS_COMPILE)
endif

ifneq ($(BOARDTYPE),)
EXTRA_CFLAGS += -D$(BOARDTYPE)
endif
ifneq ($(NOREGCHK), 1)
EXTRA_CFLAGS += -DHAVE_REGCHK
endif

EXTRA_CFLAGS += -I$(ROOTDIR)/bldinfo

ifeq ($(V),1)
Q=
VFLAG += V=1
else
Q=@
endif

CC=$(CROSSPREFIX)gcc
CPP=$(CROSSPREFIX)g++
AR=$(CROSSPREFIX)ar
ECHO=/bin/echo -e

REPDIR=$(subst $(ROOTDIR),,$(CURDIR)/)

BLDINFOPATH=bldinfo
MODBLDINFONAME=modbldinfo
MODBLDINFOTEMPLATE=$(BLDINFOPATH)/$(MODBLDINFONAME).c

#-------------------------------------------------------------------------------
# That's our default target when none is given on the command line
#-------------------------------------------------------------------------------
.PHONY: all
all: $(drv)
	@echo > /dev/null

localinstall: $(drv)
	@install -d $(LOCALINSTALLPATH)/include
	@if [ "x$(drvheaders)" != "x" ]; then \
		$(ECHO) -n "Installing"; \
		for file in $(drvheaders); do \
			$(ECHO) -n " $$file"; \
			install $$file $(LOCALINSTALLPATH)/include; \
		done; \
		$(ECHO); \
	fi
	@install -d $(LOCALINSTALLPATH)/obj
	@if [ "x$(drv)" != "x" ]; then \
		$(ECHO) -n "Installing"; \
		for file in $(drv); do \
			$(ECHO) -n " $$file"; \
			install $$file $(LOCALINSTALLPATH)/obj; \
		done; \
		$(ECHO); \
	fi

populate_repository:
	@echo > /dev/null

compile_environment: $(drv)
	@if [ "x$(ROOTDIR)" != "x" ]; then \
		install -d $(ROOTDIR)/bldenv$(REPDIR) ; \
		echo "`git rev-parse --verify HEAD` $(REPDIR)" >> $(ROOTDIR)/bldenv/version ; \
		if [ "x$(drvheaders)" != "x" ]; then \
			for file in $(drvheaders); do \
				install $$file $(ROOTDIR)/bldenv/$(REPDIR); \
			done; \
		fi; \
	fi

update_environment:
	@echo "Repository $(REPDIR): managed by git, skipped."

runtime_environment: $(drv)
	@if [ "x$(ROOTDIR)" != "x" ]; then \
		install -d $(ROOTDIR)/runtime/obj ; \
		if [ "x$(drv)" != "x" ]; then \
			for file in $(drv); do \
				install $$file $(ROOTDIR)/runtime/obj; \
			done; \
		fi; \
	fi

list:
	@if [ "x$(drv)" != "x" ]; then \
		for file in $(drv); do \
			echo "executable $$file" >> $(ROOTDIR)/binaries.dat ; \
		done; \
	fi

#-------------------------------------------------------------------------------
# Miscellaneous
#-------------------------------------------------------------------------------
tag:
	@ctags -R . >/dev/null 2>&1

clean:
	@rm -f $(drv) $(tempfiles)
	@rm -f $(ROOTDIR)/binaries.dat
	@rm -rf .tmp_versions
	@find . -type f -a \( -name "*.o" -o -name "*.dep" -o -name "*.log" \
		-o -name "*.bak" -o -name "core*" -o -name "*.a" -o -name "*.static" \
		-o -name "*.cmd" -o -name "Module.symvers" -o -name "modules.order" \
		-o -name "*.mod.c" -o -name "*.ko" -o -name $(MODBLDINFONAME).c \) -delete

distclean: clean
	@rm -f tags

#-------------------------------------------------------------------------------
# Rules
#-------------------------------------------------------------------------------
$(drv): %: $(CURDIR)/$(MODBLDINFONAME).c
	@if [ -z $(KBUILD) ]; then \
		$(ECHO) "\tKBUILD must be defined!"; \
	else \
		$(MAKE) drv_target=$@ drv_objects="$($@-objs) $(MODBLDINFONAME).o" drv_rule; \
	fi

drv_rule:
	@if [ ! -z $(Q) ]; then \
		$(ECHO) "\tGenerating driver: $(drv_target)"; \
	fi;
	@$(MAKE) $(VFLAG) -C $(KBUILD) SUBDIRS=$(CURDIR) modules obj-m=$(drv_target).o $(drv_target)-objs="$(drv_objects)" ROOTDIR=$(ROOTDIR)

#-------------------------------------------------------------------------------
# Rules for integrating building info into executables and libraries
#-------------------------------------------------------------------------------
$(CURDIR)/$(MODBLDINFONAME).c: genbldinfo
	@cat $(ROOTDIR)/$(MODBLDINFOTEMPLATE) > $@

.PHONY: genbldinfo
genbldinfo:
	@$(ECHO) "\tBuild: $(REPDIR) `git rev-parse --verify HEAD`"
	@$(ECHO) "#define BLD_PATH \"$(REPDIR)\"" > $(ROOTDIR)/$(BLDINFOPATH)/bldpath.h
	@$(ECHO) "#define BLD_VERSION \"`git rev-parse --verify HEAD`\"" > $(ROOTDIR)/$(BLDINFOPATH)/bldver.h
	@$(ECHO) "#define BLD_CONFIG \"Unknown\"" > $(ROOTDIR)/$(BLDINFOPATH)/bldsinfo.h
	@$(ECHO) "#define BLD_DATETIME \"`LANG=C date`\"" >> $(ROOTDIR)/$(BLDINFOPATH)/bldsinfo.h
	@$(ECHO) "#define BLD_PLATFORM \"`lsb_release -si` `lsb_release -sr` `uname -a`\"" >> $(ROOTDIR)/$(BLDINFOPATH)/bldsinfo.h
	@$(ECHO) "#define BLD_USER \"`id -un`\"" >> $(ROOTDIR)/$(BLDINFOPATH)/bldsinfo.h

