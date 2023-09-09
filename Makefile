
##################################################################
#                                                                #
#          Do not edit this file.  Edit config.mk instead.       #
#         Do not edit this file.  Edit config.mk instead.        #
#        Do not edit this file.  Edit config.mk instead.         #
#       Do not edit this file.  Edit config.mk instead.          #
#      Do not edit this file.  Edit config.mk instead.           #
#                                                                #
#    #####        #######       #######       ######       ###   #
#   #     #          #          #     #       #     #      ###   #
#   #                #          #     #       #     #      ###   #
#    #####           #          #     #       ######        #    #
#         #          #          #     #       #                  #
#   #     #          #          #     #       #            ###   #
#    #####           #          #######       #            ###   #
#                                                                #
#                                                                #
#      Do not edit this file.  Edit config.mk instead.           #
#       Do not edit this file.  Edit config.mk instead.          #
#        Do not edit this file.  Edit config.mk instead.         #
#         Do not edit this file.  Edit config.mk instead.        #
#          Do not edit this file.  Edit config.mk instead.       #
#                                                                #
##################################################################

#################### RULE HEADER ####################

.PHONY: update query_update print html_doc
all: query_update bootfd.img

############## VARIABLE DEFINITIONS #################

DOC = doxygen
MAKE = gmake

# If you are on an Andrew machine, these are in 410/bin,
# which should be on your PATH.
# If not, you'll need to arrange for programs with these
# names which work the same way to be on your PATH.  Note
# that different versions of gcc generally do NOT work the
# same way.
LD = 410-ld
AR = 410-ar
OBJCOPY = 410-objcopy

PROJROOT = $(PWD)
# All paths relative to PROJROOT unless otherwise noted
410KDIR = 410kern
410SDIR = spec
410UDIR = 410user
BOOTDIR = boot
410GDIR = 410guests
STUGDIR = guests
STUKDIR = kern
STUUDIR = user
EXTRADIR ?= 410extra
BUILDDIR = temp

# Relative to {410,STU}UDIR
UPROGDIR = progs
UFILEDIR = files

# By default, we are offline
UPDATE_METHOD = offline

# Suffix for dependency files
DEP_SUFFIX = dep

###########################################################################
# Libraries!
# These may be overwridden by {,$(STUKDIR)/,$(STUUDIR)/}config.mk
###########################################################################
410KERNEL_LIBS = \
				 libboot.a \
				 libelf.a \
				 liblmm.a \
				 libmalloc.a \
				 libmisc.a \
				 libRNG.a \
				 libsimics.a \
				 libstdio.a \
				 libstdlib.a \
				 libstring.a \
				 libx86.a \
				 libsmp.a \

## Can't build thrgrp by default because it depends on student-written code.
410USER_LIBS_EARLY = # libthrgrp.a
410USER_LIBS_LATE = libRNG.a libx86.a libsimics.a libstdio.a libstdlib.a \
                    libstring.a libmalloc.a libtest.a
STUDENT_LIBS_EARLY = libthread.a libautostack.a
STUDENT_LIBS_LATE = libsyscall.a

###########################################################################
# Details of library structure (you do NOT have to read this part!!)
###########################################################################
#  _EARLY means "specified early in the compiler command line" and so
#               can be taken to mean "can depend on all _LATE files"
#  _LATE means "specified late in the compiler command line" and so
#              might be taken as "there be daemons here"; in particular
#              these libraries may not depend on _EARLY libraries
#              or any library in the list before them.
#
# The full command line order is
#   410USER_LIBS_EARLY -- can depend on everything
#   STUDENT_LIBS_EARLY -- can depend only on _LATE libs
#   410USER_LIBS_LATE -- must not depend on _EARLY libs
#   STUDENT_LIBS_LATE -- must not depend on any other lib group

################# DETERMINATION OF INCLUSION BEHAVIORS #####################
ifeq (0,$(words $(filter %clean,$(MAKECMDGOALS))))
ifeq (0,$(words $(filter print,$(MAKECMDGOALS))))
ifeq (0,$(words $(filter sample,$(MAKECMDGOALS))))
ifeq (0,$(words $(filter %update,$(MAKECMDGOALS))))
DO_INCLUDE_DEPS=1
endif
endif
endif
endif


################ CONFIGURATION INCLUSION #################

-include config.mk

# Infrastructure configuration files
# These may look like fun, but they are NOT FOR STUDENT USE without
# instructor permisison.  Almost anything you want to do can be done
# via the top-level config.mk.
-include $(STUKDIR)/config.mk
-include $(STUUDIR)/config.mk
-include $(EXTRADIR)/extra-early.mk

############### TARGET VARIABLES ###############

# FIXME 410ULIBS_LATE is a temporary gross hack; fix later

PROGS = $(410REQPROGS) $(410REQBINPROGS) \
        $(STUDENTREQPROGS) $(410TESTS) $(STUDENTTESTS) \
        $(EXTRA_TESTS) $(EXTRA_410_TESTS)

FILES = $(410FILES) $(STUDENTFILES)

410ULIBS_EARLY = $(patsubst %,$(410UDIR)/%,$(410USER_LIBS_EARLY))
410ULIBS_LATE = $(patsubst %,$(410UDIR)/%,$(410USER_LIBS_LATE))
STUULIBS_EARLY = $(patsubst %,$(STUUDIR)/%,$(STUDENT_LIBS_EARLY))
STUULIBS_LATE = $(patsubst %,$(STUUDIR)/%,$(STUDENT_LIBS_LATE))

################# MISCELLANEOUS VARAIBLES ##############

CRT0 = $(410UDIR)/crt0.o

LDFLAGSENTRY = --entry=_main

########### MAKEFILE FILE INCLUSION ##############

# These are optional so that we can use the same Makefile for P1,2,3
# Again, these files are NOT FOR STUDENT USE, and unlike the above,
# we probably won't give you permission to overwrite these. :)

# Boot loader P4
-include $(BOOTDIR)/boot.mk

# Guest P4
-include $(410GDIR)/guests.mk

# Include 410kern control, responsible for 410kern libraries, objects, interim
-include $(410KDIR)/kernel.mk
# Include 410user control, responsible for 410user libraries and programs
-include $(410UDIR)/user.mk
# Infrastructure
-include $(EXTRADIR)/extra-late.mk

# Top level targets
-include $(410KDIR)/toplevel.mk


############# BUILD PARAMETER VARIABLES ################
# Note that this section DEPENDS UPON THE INCLUDES ABOVE

# When generating includes, we do so in the order we do so that it is always
# possible to match <lib/header.h> from 410kern/lib exactly, or from kern/lib
# if so desired, but that files without directory prefixes will match
# 410kern/inc, kern/, kern/inc, then the 410kern/lib/* entries according to
# 410KERNEL_LIBS, which is under external control.

CFLAGS_COMMON = -nostdinc \
	-fno-strict-aliasing -fno-builtin -fno-stack-protector -fno-omit-frame-pointer \
	-fno-delete-null-pointer-checks -fwrapv \
	-fno-pic \
	-fcf-protection=none \
	--std=gnu99 \
	-D__STDC_NO_ATOMICS__ \
	-Wall -Werror

CFLAGS_GCC = $(CFLAGS_COMMON) \
	-fno-aggressive-loop-optimizations \
	-gstabs+ -gstrict-dwarf -O0 -m32

CFLAGS_CLANG = $(CFLAGS_COMMON) \
	-gdwarf-3 -gstrict-dwarf -Og -m32 -fno-addrsig -march=i386 \
	--target=i386-unknown-none-unknown \
	-fno-blocks

CFLAGS_CLANGALYZER = $(CFLAGS_CLANG)
CFLAGS_STUDENT_CLANGALYZER = -pedantic-errors -Weverything \
	-Wno-reserved-id-macro -Wno-unused-macros \
	-Wno-documentation -Wno-documentation-unknown-command \
	-Wno-cast-align -Wno-padded \
	-Wno-unused-parameter \
	-Wno-pointer-arith \
	-Wno-shift-sign-overflow \
	-Wno-vla \
	-Wno-missing-noreturn

ANALYZER_CFLAGS = --analyze -Xanalyzer -analyzer-output=text

ifneq (, $(findstring clangalyzer, $(CC)))
	CC = 410-clang
	KCFLAGS = $(CFLAGS_CLANGALYZER)
	UCFLAGS = $(CFLAGS_CLANG) -mstack-alignment=2
	ANALYZE_ENABLED = YES
	CFLAGS_STUDENT = $(CFLAGS_STUDENT_CLANGALYZER)
else ifneq (,$(findstring clang, $(CC)))
	CC = 410-clang
	KCFLAGS = $(CFLAGS_CLANG)
	UCFLAGS = $(CFLAGS_CLANG) -mstack-alignment=2
else ifneq (, $(findstring gcc, $(CC)))
	CC = 410-gcc
	KCFLAGS = $(CFLAGS_GCC)
	UCFLAGS = $(CFLAGS_GCC)
	# This prevents the gcc4 "hurr durr I'm aligning main()'s stack"
	UCFLAGS += -mpreferred-stack-boundary=2
else
$(error "Unknown CC = $(CC)")
endif

ifneq (,$(findstring kernel,$(CONFIG_DEBUG)))
	KCFLAGS += -DDEBUG
endif
ifneq (,$(findstring kernel,$(CONFIG_NDEBUG)))
	KCFLAGS += -DNDEBUG
endif
KLDFLAGS = -static -Ttext 100000 --fatal-warnings -melf_i386
# As of Ubuntu 20.04, w/o --nmagic, ld generates a program header
# which loads the ELF header below 1M.
# GRUB refuses to attempt that, thus ending booting.  In theory
# using an add-on linker script with a PHDRS section should make
# this not happen, but we have not yet achieved that.  Note that
# an alternate "solution" (again as of Ubuntu 20.04) is to re-enable
# our ancient custom kernel.lds -- apparently that makes things work
# w/o nmagic.
KLDFLAGS += --nmagic
#
KINCLUDES = -I$(410KDIR) -I$(410KDIR)/inc \
			-I$(410SDIR) \
			-I$(STUKDIR) -I$(STUKDIR)/inc \
			$(patsubst lib%.a,-I$(410KDIR)/%,$(410KERNEL_LIBS))

ifneq (,$(findstring user,$(CONFIG_DEBUG)))
	UCFLAGS += -DDEBUG
endif
ifneq (,$(findstring user,$(CONFIG_NDEBUG)))
	UCFLAGS += -DNDEBUG
endif
ULDFLAGS = -static -Ttext 1000000 --fatal-warnings -melf_i386 $(LDFLAGSENTRY)
UINCLUDES = -I$(410SDIR) -I$(410UDIR) -I$(410UDIR)/inc \
						-I$(STUUDIR)/inc \
			$(patsubst %.a,-I$(410UDIR)/%,$(410USER_LIBS_EARLY) $(410USER_LIBS_LATE)) \
			$(patsubst %.a,-I$(STUUDIR)/%,$(STUDENT_LIBS_EARLY) $(STUDENT_LIBS_LATE))

TMP_410KOBJS_NOT_DOTO = $(filter-out %.o,$(ALL_410KOBJS))
ifneq (,$(TMP_410KOBJS_NOT_DOTO))
$(error "Feeling nervous about '$(TMP_410KOBJS_NOT_DOTO)'")
endif
ALL_410KOBJS_DEPS = $(ALL_410KOBJS:%.o=%.$(DEP_SUFFIX))

TMP_410UOBJS_NOT_DOTO = $(filter-out %.o,$(ALL_410UOBJS))
ifneq (,$(TMP_410UOBJS_NOT_DOTO))
$(error "Feeling nervous about '$(TMP_410UOBJS_NOT_DOTO)'")
endif
ALL_410UOBJS_DEPS = $(ALL_410UOBJS:%.o=%.$(DEP_SUFFIX))

TMP_STUKOBJS_NOT_DOTO = $(filter-out %.o,$(ALL_STUKOBJS))
ifneq (,$(TMP_STUKOBJS_NOT_DOTO))
$(error "Feeling nervous about '$(TMP_STUKOBJS_NOT_DOTO)'")
endif
ALL_STUKOBJS_DEPS = $(ALL_STUKOBJS:%.o=%.$(DEP_SUFFIX))

TMP_STUUOBJS_NOT_DOTO = $(filter-out %.o,$(ALL_STUUOBJS))
ifneq (,$(TMP_STUUOBJS_NOT_DOTO))
$(error "Feeling nervous about '$(TMP_STUUOBJS_NOT_DOTO)'")
endif
ALL_STUUOBJS_DEPS = $(ALL_STUUOBJS:%.o=%.$(DEP_SUFFIX))

STUUPROGS_DEPS = $(STUUPROGS:%=%.dep)
410UPROGS_DEPS = $(410UPROGS:%=%.dep)

############## UPDATE RULES #####################

update:
	./update.sh $(UPDATE_METHOD)
	$(GUEST_UPDATE)

query_update:
	./update.sh $(UPDATE_METHOD) query

################### GENERIC RULES ######################
%.o: %.S
	$(CC) $(CFLAGS) -DASSEMBLER $(INCLUDES) -c -MD -MP -MF $(@:.o=.$(DEP_SUFFIX)) -MT $@ -o $@ $<
	$(OBJCOPY) -R .comment -R .note -R .note.gnu.property $@ $@

%.o: %.s
	@echo "You should use the .S file extension rather than .s"
	@echo ".s does not support precompiler directives (like #include)"
	@false

# There is a bug in clang that such that the .o file will not be generated if
# the static analyzer is executed.
%.o: %.c
	@expand -t $(TABSTOP) $< | awk 'length > 80 \
	  { err++; \
	    errs[err] = "Warning: line " NR " in $< is longer than 80 characters"; \
	  } END { \
	    if (err > 2 && err < 1000) for (i = 1; i <= err; i++) print errs[i];\
	    exit(err > 10 && err < 1000) }'
ifeq ($(ANALYZE),YES)
	$(CC) $(CFLAGS) $(ANALYZER_CFLAGS) $(INCLUDES) -c -o $@ $<
endif
	$(CC) $(CFLAGS) $(INCLUDES) -c -MD -MP -MF $(@:.o=.$(DEP_SUFFIX)) -MT $@ -o $@ $<
	$(OBJCOPY) -R .comment -R .note -R .note.gnu.property $@ $@

%.a:
	rm -f $@
	$(AR) rc $@ $^

################ PATTERNED VARIABLE ASSIGNMENTS ##################
# Some of the oddness is that the 410 base code won't currently
# stand up to extreme scrutiny.
$(410KDIR)/%: CFLAGS=$(KCFLAGS)
$(410KDIR)/%: ANALYZE=NO
$(410KDIR)/%: INCLUDES=$(KINCLUDES)
$(410KDIR)/%: LDFLAGS=$(KLDFLAGS)
$(STUKDIR)/%: CFLAGS=$(KCFLAGS) $(CFLAGS_STUDENT)
$(STUKDIR)/%: ANALYZE=$(ANALYZE_ENABLED)
$(STUKDIR)/%: INCLUDES=$(KINCLUDES)
$(STUKDIR)/%: LDFLAGS=$(KLDFLAGS)
$(410UDIR)/%: CFLAGS=$(UCFLAGS)
$(410UDIR)/%: ANALYZE=NO
$(410UDIR)/%: INCLUDES=$(UINCLUDES)
$(410UDIR)/%: LDFLAGS=$(ULDFLAGS)
$(STUUDIR)/%: CFLAGS=$(UCFLAGS) $(CFLAGS_STUDENT)
$(STUUDIR)/%: ANALYZE=$(ANALYZE_ENABLED)
$(STUUDIR)/%: INCLUDES=$(UINCLUDES)
$(STUUDIR)/%: LDFLAGS=$(ULDFLAGS)

################# USERLAND PROGRAM UNIFICATION RULES #################

$(410REQPROGS:%=$(BUILDDIR)/%): \
$(BUILDDIR)/%: $(410UDIR)/$(UPROGDIR)/%
	mkdir -p $(BUILDDIR)
	cp $< $@

$(410REQBINPROGS:%=$(BUILDDIR)/%): \
$(BUILDDIR)/%: $(410UDIR)/$(UPROGDIR)/%
	mkdir -p $(BUILDDIR)
	cp $< $@

$(410TESTS:%=$(BUILDDIR)/%) : \
$(BUILDDIR)/% : $(410UDIR)/$(UPROGDIR)/%
	mkdir -p $(BUILDDIR)
	cp $< $@

$(EXTRA_410_TESTS:%=$(BUILDDIR)/%) : \
$(BUILDDIR)/% : $(410UDIR)/$(UPROGDIR)/%
	mkdir -p $(BUILDDIR)
	cp $< $@

$(STUDENTREQPROGS:%=$(BUILDDIR)/%) : \
$(BUILDDIR)/% : $(STUUDIR)/$(UPROGDIR)/%
	mkdir -p $(BUILDDIR)
	cp $< $@

$(STUDENTTESTS:%=$(BUILDDIR)/%) : \
$(BUILDDIR)/% : $(STUUDIR)/$(UPROGDIR)/%
	mkdir -p $(BUILDDIR)
	cp $< $@

# 410user/files/* and user/files/* are dependencies, but
# they are not targets to be built.  In particular, if
# both 410user/files/x and 410user/files/x.S exist, don't
# try to build one from the other.
.PHONY: $(410FILES:%=$(410UDIR)/$(UFILEDIR)/%)
.PHONY: $(STUDENTFILES:%=$(STUUDIR)/$(UFILEDIR)/%)

$(410FILES:%=$(BUILDDIR)/%) : \
$(BUILDDIR)/% : $(410UDIR)/$(UFILEDIR)/%
	mkdir -p $(BUILDDIR)
	cp $< $@

$(STUDENTFILES:%=$(BUILDDIR)/%) : \
$(BUILDDIR)/% : $(STUUDIR)/$(UFILEDIR)/%
	mkdir -p $(BUILDDIR)
	cp $< $@

############## MISCELLANEOUS TARGETS #################

html_doc:
	$(DOC) doxygen.conf

PRINTOUT=kernel.pdf
PRINTOUTPS=$(PRINTOUT:%.pdf=%.ps)

print:
	enscript -2rG -fCourier7 -FCourier-Bold10 -p $(PRINTOUTPS) \
		$(if $(strip $(TABSTOP)),-T $(TABSTOP),) \
			README.dox \
			`find ./kern/ -type f -regex  '.*\.[chS]' | sort`
	ps2pdf $(PRINTOUTPS) $(PRINTOUT)

################# CLEANING RULES #####################
.PHONY: clean veryclean

clean:
	rm -f $(CRT0)
	rm -f $(FINALCLEANS)
	rm -f $(ALL_410KOBJS)
	rm -f $(ALL_410KOBJS_DEPS)
	rm -f $(410KCLEANS)
	rm -f $(ALL_410UOBJS)
	rm -f $(ALL_410UOBJS_DEPS)
	rm -f $(410UCLEANS)
	rm -f $(410UPROGS:%=%.o)
	rm -f $(410UPROGS_DEPS)
	rm -f $(ALL_STUKOBJS)
	rm -f $(ALL_STUKOBJS_DEPS)
	rm -f $(STUKCLEANS)
	rm -f $(ALL_STUUOBJS)
	rm -f $(ALL_STUUOBJS_DEPS)
	rm -f $(STUUCLEANS)
	rm -f $(STUUPROGS:%=%.o)
	rm -f $(STUUPROGS_DEPS)
	rm -f $(BOOT_CLEANS)
	$(GUEST_CLEAN)

veryclean: clean
	rm -rf doc $(PRINTOUT) $(PRINTOUTPS) bootfd.img kernel kernel.log $(BUILDDIR)
	rm -f $(FINALVERYCLEANS)
	$(GUEST_VERYCLEAN)

%clean:
	$(error "Unknown cleaning target")

########### DEPENDENCY FILE INCLUSION ############
ifeq (1,$(DO_INCLUDE_DEPS))
ifneq (,$(ALL_410KOBJS))
-include $(ALL_410KOBJS_DEPS)
endif
ifneq (,$(ALL_STUKOBJS))
-include $(ALL_STUKOBJS_DEPS)
endif
ifneq (,$(ALL_410UOBJS))
-include $(ALL_410UOBJS_DEPS)
endif
ifneq (,$(ALL_STUUOBJS))
-include $(ALL_STUUOBJS_DEPS)
endif
ifneq (,$(410UPROGS_DEPS))
-include $(410UPROGS_DEPS)
endif
ifneq (,$(STUUPROGS_DEPS))
-include $(STUUPROGS_DEPS)
endif
endif

########### MANDATE CLEANING AS ONLY TARGETS ############
ifneq (0,$(words $(filter %clean,$(MAKECMDGOALS))))
  # The target includes a make target
  ifneq (0, $(words $(filter-out %clean,$(MAKECMDGOALS))))
    # There is another target on the list
    $(error "Clean targets must run by themselves")
  endif
endif
