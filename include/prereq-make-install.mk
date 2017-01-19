TOPDIR:=${CURDIR}
LC_ALL:=C
LANG:=C
export TOPDIR LC_ALL LANG

all: $(TOPDIR)/staging_dir/host/stamp/.make
	@true

ifndef __mxl_config_mi
# based on work by John Dill

# Inclusion guard for this file.
__mxl_config_mi:=$(true)

# This variable represents the true construct.
true:=true

# This tests the existance of $(lastword) (GNU Make 3.81).
__mxl_have_lastword:=$(lastword $(true))

# This defines the make compatibility for GNU Make 3.81.
__mxl_have_make_3.81:=$(__mxl_have_lastword)

endif # __mxl_config_mi

ifndef __mxl_have_make_3.81

$(TOPDIR)/staging_dir/host/bin/make:
	@if [ ! -f $(TOPDIR)/dl/make-3.81.tar.bz2 ]; then \
		echo "Downloading make 3.81"; \
		mkdir -p $(TOPDIR)/dl; \
		$(TOPDIR)/scripts/download.pl $(TOPDIR)/dl/ make-3.81.tar.bz2 x 2>/dev/null; \
		if [ ! -f $(TOPDIR)/dl/make-3.81.tar.bz2 ]; then \
		(	cd $(TOPDIR)/dl; \
			wget http://mirror.switch.ch/ftp/mirror/gnu/make/make-3.81.tar.bz2; \
		); \
		fi; \
	fi
	@if [ -f $(TOPDIR)/dl/make-3.81.tar.bz2 ]; then \
		echo "Building make 3.81"; \
		rm -Rf $(TOPDIR)/build_dir/make-3.81 >/dev/null; \
		mkdir -p $(TOPDIR)/build_dir >/dev/null; \
		mkdir -p $(TOPDIR)/tmp >/dev/null; \
		mkdir -p $(TOPDIR)/staging_dir/host/stamp >/dev/null; \
		bzcat $(TOPDIR)/dl/make-3.81.tar.bz2 | tar -x -C $(TOPDIR)/build_dir; \
		(	cd $(TOPDIR)/build_dir/make-3.81; \
			./configure --prefix=$(TOPDIR)/staging_dir/host >/dev/null; \
		); \
		$(MAKE) -s -C $(TOPDIR)/build_dir/make-3.81 >/dev/null; \
		$(MAKE) -s -C $(TOPDIR)/build_dir/make-3.81 install >/dev/null; \
	else \
		echo "ERROR: Please copy make-3.81.tar.bz2 to $(TOPDIR)/dl"; \
		false; \
	fi;

$(TOPDIR)/staging_dir/host/stamp/.make: $(TOPDIR)/staging_dir/host/bin/make
	@touch $@

else

$(TOPDIR)/staging_dir/host/stamp/.make:
	@echo make version correct
	@mkdir -p $(TOPDIR)/staging_dir/host/stamp
	@touch $@

endif # 3.81 compatibility check
