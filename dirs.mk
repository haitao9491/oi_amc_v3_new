#
# (C) Copyright 2010
# Hu Chunlin <chunlin.hu@gmail.com>
#
# dirs.mk - To make my life easier.
#

MAKEFLAGS += --no-print-directory -r

REPDIR=$(subst $(ROOTDIR),,$(CURDIR)/)
ifeq ($(GITBAREMODE),1)
	REPNAME=$(shell echo $(REPDIR) | sed s/^\\///g | sed s/\\/$$//g | \
				sed s/\\//-/g | sed s/.$$/\&-/g )
else
	REPNAME=$(shell echo $(REPDIR) | sed s/^\\///g )
endif

.PHONY : $(SUBDIRS)

all:
	@for dir in $(SUBDIRS) ; do \
		if [ -f $$dir/Makefile ]; then \
			$(MAKE) -C $$dir ; \
		fi \
	done

localinstall: all
	@for dir in $(SUBDIRS) ; do \
		if [ -f $$dir/Makefile ]; then \
			$(MAKE) -C $$dir localinstall ; \
		fi \
	done

populate_repository:
	@for dir in $(SUBDIRS) ; do \
		if [ -f $$dir/Makefile ]; then \
			$(MAKE) -C $$dir populate_repository ; \
		else \
			echo "Cloning from $(GITURL)$(REPNAME)$$dir$(GITURLSUFFIX)" | \
				awk '{ printf("%-72s", $$0); }' ; \
			if git clone $(GITURL)$(REPNAME)$$dir$(GITURLSUFFIX) $$dir >/dev/null 2>&1; then \
				echo "[OK]" ; \
				$(MAKE) -C $$dir populate_repository ; \
			else \
				echo "[FAILED]" ; \
			fi; \
		fi \
	done

compile_environment: all
	@if [ "x$(ROOTDIR)" != "x" ]; then \
		install -d $(ROOTDIR)/bldenv$(REPDIR) ; \
		if [ "$(REPDIR)" = "/" ]; then \
			rm -f $(ROOTDIR)/bldenv/version; \
		fi; \
		echo "`git rev-parse --verify HEAD` $(REPDIR)" >> $(ROOTDIR)/bldenv/version ; \
		for dir in $(SUBDIRS) ; do \
			if [ -f $$dir/Makefile ]; then \
				$(MAKE) -C $$dir compile_environment ; \
			fi \
		done; \
	fi

update_environment:
	@for dir in $(SUBDIRS) ; do \
		if [ -f $$dir/Makefile ]; then \
			$(MAKE) -C $$dir update_environment ; \
		else \
			if [ ! -d $$dir ]; then mkdir $$dir; fi; \
			cd $$dir; \
			tar -x -f $(BLDENVTAR) -C $(ROOTDIR) .$(REPDIR)$$dir ; \
			echo "Repository $(REPDIR)$$dir: updated from $(BLDENVTAR)"; \
			cd ..; \
		fi \
	done

runtime_environment: all
	@if [ "x$(ROOTDIR)" != "x" ]; then \
		for dir in $(SUBDIRS) ; do \
			if [ -f $$dir/Makefile ]; then \
				$(MAKE) -C $$dir runtime_environment ; \
			fi \
		done; \
	fi

list:
	@for dir in $(SUBDIRS) ; do \
		if [ -f $$dir/Makefile ]; then \
			$(MAKE) -C $$dir list ; \
		fi \
	done

clean:
	@for dir in $(SUBDIRS) ; do \
		if [ -f $$dir/Makefile ]; then \
			$(MAKE) -C $$dir clean ; \
		fi \
	done

distclean:
	@for dir in $(SUBDIRS) ; do \
		if [ -f $$dir/Makefile ]; then \
			$(MAKE) -C $$dir distclean ; \
		fi \
	done
	@rm -f tags

tag:
	@ctags -R . >/dev/null 2>&1

