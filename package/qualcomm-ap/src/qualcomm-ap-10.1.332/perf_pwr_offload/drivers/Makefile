#############################################################
# Required variables for each makefile
# Discard this section from all parent makefiles
# Expected variables (with automatic defaults):
#   CSRCS (all "C" files in the dir)
#   SUBDIRS (all subdirs with a Makefile)
#   GEN_LIBS - list of libs to be generated ()
#   GEN_IMAGES - list of images to be generated ()
#   COMPONENTS_xxx - a list of libs/objs in the form
#     subdir/lib to be extracted and rolled up into
#     a generated lib/image xxx.a ()
#   DEPENDS_xxx - a list of explicit dependencies
#
ifndef PDIR

SUBDIRS = target

endif

# Default to verbose to match existing behavior
V ?= 1

ifeq ($(V), 1)
Q=
E=true
MAKE_PRINT_DIR=--print-directory
else
Q=@
MAKE_PRINT_DIR=--no-print-directory
E=echo
endif

#############################################################
# Configuration i.e. compile options etc.
# Target specific stuff (defines etc.) goes in here!
# Generally values applying to a tree are captured in the
#   makefile at its root level - these are then overridden
#   for a subtree within the makefile rooted therein
#

#############################################################
# Computed variables
# Should be done in top-level makefile only
#

CSRCS ?= $(wildcard *.c)
CSRCs ?= $(wildcard *.cpp)
ASRCs ?= $(wildcard *.s)
ASRCS ?= $(wildcard *.S)
SUBDIRS ?= $(patsubst %/,%,$(dir $(wildcard */Makefile)))

ODIR ?= .output
OBJODIR ?= $(ODIR)/obj
LIBODIR ?= $(ODIR)/lib
IMAGEODIR ?= $(ODIR)/image
BINODIR ?= $(ODIR)/bin
EXTRACT_DIR ?= $(ODIR)/.extract
UP_EXTRACT_DIR ?= ../../../..

AUXCNAMES := $(notdir $(AUXCSRCS))

OBJS := $(CSRCS:%.c=$(OBJODIR)/%.o) \
	$(CSRCs:%.cpp=$(OBJODIR)/%.o) \
	$(ASRCs:%.s=$(OBJODIR)/%.o) \
	$(ASRCS:%.S=$(OBJODIR)/%.o) \
        $(AUXCNAMES:%.c=$(OBJODIR)/%.o)

DEPS := $(CSRCS:%.c=$(OBJODIR)/%.d) \
	$(CSRCs:%.cpp=$(OBJODIR)/%.d) \
	$(ASRCs:%.s=$(OBJODIR)/%.d) \
	$(ASRCS:%.S=$(OBJODIR)/%.d) \
        $(AUXCNAMES:%.c=$(OBJODIR)/%.d)

OLIBS := $(GEN_LIBS:%=$(LIBODIR)/%) $(E_LIBS:%=$(LIBODIR)/%)
OIMAGES := $(GEN_IMAGES:%=$(IMAGEODIR)/%)
OBINS := $(GEN_BINS:%=$(BINODIR)/%)

CFLAGS = $(COPTS) $(DEFINES) $(INCLUDES)
DFLAGS = $(DOPTS) $(DDEFINES) $(INCLUDES)

#############################################################
# Functions
#

define ShortcutRule
$(1): $(2)/$(1)
endef

define MakeLibrary
DEP_LIBS_$(1) = $$(foreach lib,$$(filter %.a,$$(COMPONENTS_$(1))),$$(dir $$(lib))$$(LIBODIR)/$$(notdir $$(lib)))
DEP_OBJS_$(1) = $$(foreach obj,$$(filter %.o,$$(COMPONENTS_$(1))),$$(dir $$(obj))$$(OBJODIR)/$$(notdir $$(obj)))
$$(LIBODIR)/$(1).a: $$(OBJS) $$(DEP_OBJS_$(1)) $$(DEP_LIBS_$(1)) $$(DEPENDS_$(1))
	$(Q)@mkdir -p $$(LIBODIR)
	$(Q)$$(if $$(filter %.a,$$?),mkdir -p $$(EXTRACT_DIR)_$(1))
	$(Q)$$(if $$(filter %.a,$$?),cd $$(EXTRACT_DIR)_$(1); $$(foreach lib,$$(filter %.a,$$?),$$(AR) xo $$(UP_EXTRACT_DIR)/$$(lib);))
	$(Q)($$(AR) ru $$@ $$(filter %.o,$$?) $$(if $$(filter %.a,$$?),$$(EXTRACT_DIR)_$(1)/*.o) 2>&1) >/dev/null
	@$(E) "  AR " $(subst $(WORKAREA)/,,$(CURDIR))/$1.a
	$(Q)$$(if $$(filter %.a,$$?),$$(RM) -r $$(EXTRACT_DIR)_$(1))
endef

define MakeELibrary
DEP_LIBS_$(1) = $$(foreach lib,$$(filter %.a,$$(COMPONENTS_$(1))),$$(dir $$(lib))$$(LIBODIR)/$$(notdir $$(lib)))
DEP_OBJS_$(1) = $$(foreach obj,$$(filter %.o,$$(COMPONENTS_$(1))),$$(dir $$(obj))$$(OBJODIR)/$$(notdir $$(obj)))
$$(LIBODIR)/$(1).a: $$(OBJS) $$(DEP_OBJS_$(1)) $$(DEP_LIBS_$(1)) $$(DEPENDS_$(1))
	$(Q)@mkdir -p $$(LIBODIR)
	$(Q)$$(if $$(filter %.a,$$?),mkdir -p $$(EXTRACT_DIR)_$(1))
	$(Q)$$(if $$(filter %.a,$$?),cd $$(EXTRACT_DIR)_$(1); $$(foreach lib,$$(filter %.a,$$?),$$(AR) xo $$(UP_EXTRACT_DIR)/$$(lib);))
	$(Q)$($$(AR) ru $$@ $$(filter %.o,$$?) $$(if $$(filter %.a,$$?),$$(EXTRACT_DIR)_$(1)/*.o) 2>&1) >/dev/null
	@$(E) "  AR " $1.a
	$(Q)$$(if $$(filter %.a,$$?),$$(RM) -r $$(EXTRACT_DIR)_$(1))
	$$(OBJCOPY) \
		--rename-section .data=.Edata \
		--rename-section .bss=.Ebss \
		--rename-section .rodata=.Erodata \
		--rename-section .rodata.str1.4=.Erodata.str1.4 \
		$$@ $$@
endef

define MakeImage
DEP_LIBS_$(1) = $$(foreach lib,$$(filter %.a,$$(COMPONENTS_$(1))),$$(dir $$(lib))$$(LIBODIR)/$$(notdir $$(lib)))
DEP_OBJS_$(1) = $$(foreach obj,$$(filter %.o,$$(COMPONENTS_$(1))),$$(dir $$(obj))$$(OBJODIR)/$$(notdir $$(obj)))
ifneq ($(A_SIMOS_DEVHOST),y)
LD=$$(CC)
endif
$$(IMAGEODIR)/$(1).out: $$(OBJS) $$(DEP_OBJS_$(1)) $$(DEP_LIBS_$(1)) $$(DEPENDS_$(1))
	@mkdir -p $$(IMAGEODIR)
	$(Q)$$(LD) $$(LDFLAGS) $$(if $$(LINKFLAGS_$(1)),$$(LINKFLAGS_$(1)),$$(LINKFLAGS_DEFAULT) $$(OBJS) $$(DEP_OBJS_$(1)) $$(DEP_LIBS_$(1))) -o $$@ 
	@$(E) "  LD " target/$$@
endef

define AuxRule
AUXOBJNAME_$(2) = $(2:%.c=$(OBJODIR)/%.o)
AUXDEPNAME_$(2) = $(2:%.c=$(OBJODIR)/%.d)
ALL_AUXINCLUDES_$(2) = $(foreach dir,$(AUXINCLUDES), -I$(dir))

$$(AUXOBJNAME_$(2)): $(1)
	@mkdir -p $$(IMAGEODIR);
	$(Q)$(CC) $(CFLAGS) $(COPTS_$(*F)) $$(ALL_AUXINCLUDES_$(2)) -o $$@ -c $$<
	@$(E) "  CC " $(subst $(WORKAREA)/,,$(CURDIR))$<

$$(AUXDEPNAME_$(2)): $(1)
	@mkdir -p $(OBJODIR);
	@set -e; rm -f $$@; \
	$(CC) -M $(CFLAGS) $$(ALL_AUXINCLUDES_$(2)) $$< > $$@.$$$$$$$$; \
	sed 's,\(.$$\*\.o\)[ :]*,$$(OBJODIR)/\1 $$@ : ,g' < $$@.$$$$$$$$ > $$@; \
	rm -f $$@.$$$$$$$$
endef


#############################################################
# Rules base
# Should be done in top-level makefile only
#

default_all:	.subdirs $(OBJS) $(OLIBS) $(OIMAGES) $(OBINS) $(SPECIAL_MKTARGS)

.subdirs:
	@set -e; $(foreach d, $(SUBDIRS), $(MAKE) $(MAKE_PRINT_DIR) -C $(d);)

clean:
	$(Q)$(foreach d, $(SUBDIRS), $(MAKE) $(MAKE_PRINT_DIR) -C $(d) clean;)
	$(Q)$(RM) -r $(ODIR)

clobber: $(SPECIAL_CLOBBER)
	$(Q)$(foreach d, $(SUBDIRS), $(MAKE) $(MAKE_PRINT_DIR) -C $(d) clobber;)
	$(Q)$(RM) -r $(OUTPUTDIR)

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),clobber)
ifdef DEPS
sinclude $(DEPS)
endif
endif
endif

$(OBJODIR)/%.o: %.c
	@mkdir -p $(OBJODIR);
	$(Q)$(CC) $(if $(findstring $<,$(DSRCS)),$(DFLAGS),$(CFLAGS)) $(COPTS_$(*F)) -o $@ -c $<
	@$(E) "  CC " $(subst $(WORKAREA)/,,$(CURDIR))/$<

$(OBJODIR)/%.d: %.c
	@mkdir -p $(OBJODIR);
	@set -e; rm -f $@; \
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\.o\)[ :]*,$(OBJODIR)/\1 $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(OBJODIR)/%.o: %.cpp
	@mkdir -p $(OBJODIR);
	$(Q)$(CC) $(if $(findstring $<,$(DSRCS)),$(DFLAGS),$(CFLAGS)) $(COPTS_$(*F)) -o $@ -c $<
	@$(E) "  CC " $(subst $(WORKAREA)/,,$(CURDIR))/$<

$(OBJODIR)/%.d: %.cpp
	@mkdir -p $(OBJODIR);
	$(Q)set -e; rm -f $@;
	$(Q)$(CC) -M $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\.o\)[ :]*,$(OBJODIR)/\1 $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(OBJODIR)/%.o: %.s
	@mkdir -p $(OBJODIR);
	$(Q)$(CC) $(CFLAGS) -o $@ -c $<
	@$(E) " ASM " $(subst $(WORKAREA)/,,$(CURDIR))/$<

$(OBJODIR)/%.d: %.s
	@mkdir -p $(OBJODIR); \
	$(Q)set -e; rm -f $@; \
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\.o\)[ :]*,$(OBJODIR)/\1 $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(OBJODIR)/%.o: %.S
	@mkdir -p $(OBJODIR);
	$(Q)$(CC) $(CFLAGS) -D__ASSEMBLER__ -o $@ -c $<
	@$(E) " ASM " $(subst $(WORKAREA)/,,$(CURDIR))/$<

$(OBJODIR)/%.d: %.S
	@mkdir -p $(OBJODIR); \
	$(Q)set -e; rm -f $@; \
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\.o\)[ :]*,$(OBJODIR)/\1 $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$


$(foreach lib,$(GEN_LIBS),$(eval $(call ShortcutRule,$(lib),$(LIBODIR))))

$(foreach image,$(GEN_IMAGES),$(eval $(call ShortcutRule,$(image),$(IMAGEODIR))))

$(foreach bin,$(GEN_BINS),$(eval $(call ShortcutRule,$(bin),$(BINODIR))))

$(foreach lib,$(GEN_LIBS),$(eval $(call MakeLibrary,$(basename $(lib)))))

$(foreach lib,$(E_LIBS),$(eval $(call MakeELibrary,$(basename $(lib)))))

$(foreach image,$(GEN_IMAGES),$(eval $(call MakeImage,$(basename $(image)))))

$(foreach name,$(AUXCSRCS),$(eval $(call AuxRule,$(name),$(notdir $(name)))))
DEP_OBJS_$(1) = $$(foreach
