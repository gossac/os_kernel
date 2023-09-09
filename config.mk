###########################################################################
# This is the include file for the make file.
# You should have to edit only this file to get things to build.
###########################################################################

###########################################################################
# P4
###########################################################################
# This is a list of guests you want to put into test. Remember to update
# the corresponding hvstubs.S.
STUDENTGUESTS = hello magic dumper console station tick_tock flayrod \
			    cliff
				
410GUESTBINS = gomoku

###########################################################################
# Tab stops
###########################################################################
# If you use tabstops set to something other than the international
# standard of eight characters, this is your opportunity to inform
# our print scripts.
TABSTOP = 4

###########################################################################
# Compiler
###########################################################################
# Selections (see handout for details):
#
# gcc - default (what we will grade with)
# clang - Clang/LLVM
# clangalyzer - Clang/LLVM plus static analyzer
#
# "gcc" may have a better Simics debugging experience
#
# "clang" may provide helpful warnings, assembly
# code may be more readable
#
# "clangalyzer" will likely complain more than "clang"
#
# Use "make veryclean" if you adjust CC.
CC = gcc

###########################################################################
# DEBUG
###########################################################################
# You can set CONFIG_DEBUG to any mixture of the words
# "kernel" and "user" to #define the DEBUG flag when
# compiling the respective type(s) of code.  The ordering
# of the words doesn't matter, and repeating a word has
# no additional effect.
#
# Use "make veryclean" if you adjust CONFIG_DEBUG.
#
CONFIG_DEBUG = user kernel

###########################################################################
# NDEBUG
###########################################################################
# You can set CONFIG_NDEBUG to any mixture of the words
# "kernel" and "user" to #define the NDEBUG flag when
# compiling the respective type(s) of code.  The ordering
# of the words doesn't matter, and repeating a word has
# no additional effect.  Defining NDEBUG will cause the
# checks using assert() to be *removed*.
#
# Use "make veryclean" if you adjust CONFIG_NDEBUG.
#
CONFIG_NDEBUG =

###########################################################################
# The method for acquiring project updates.
###########################################################################
# This should be "afs" for any Andrew machine, "web" for non-andrew machines
# and "offline" for machines with no network access.
#
# "offline" is strongly not recommended as you may miss important project
# updates.
#
UPDATE_METHOD = afs

###########################################################################
# WARNING: When we test your code, the two TESTS variables below will be
# blanked.  Your kernel MUST BOOT AND RUN if 410TESTS and STUDENTTESTS
# are blank.  It would be wise for you to test that this works.
###########################################################################

###########################################################################
# Test programs provided by course staff you wish to run
###########################################################################
# A list of the test programs you want compiled in from the 410user/progs
# directory.
#
410TESTS = # ck1 fork_test1 getpid_test1 exec_basic exec_basic_helper \
		   # exec_nonexist halt_test score loader_test1 loader_test2 \
		   # print_basic yield_desc_mkrun deschedule_hang wait_getpid \
		   # fork_wait readline_basic new_pages remove_pages_test1 \
		   # remove_pages_test2 mem_permissions fork_exit_bomb \
		   # actual_wait fork_wait_bomb stack_test1 mem_eat_test swexn_regs \
		   # swexn_basic_test swexn_cookie_monster swexn_dispatch \
		   # swexn_stands_for_swextensible swexn_uninstall_test sleep_test1 \
		   # make_crash make_crash_helper cho cho2 cho_variant \
		   # wild_test1 register_test minclone_mem \
		   # cat fork_bomb knife work bg slaughter coolness peon merchant \
		   # chow ack fib bistromath
		   

###########################################################################
# Test programs you have written which you wish to run
###########################################################################
# A list of the test programs you want compiled in from the user/progs
# directory.
#
STUDENTTESTS =

###########################################################################
# Data files provided by course staff to build into the RAM disk
###########################################################################
# A list of the data files you want built in from the 410user/files
# directory.
#
410FILES =

###########################################################################
# Data files you have created which you wish to build into the RAM disk
###########################################################################
# A list of the data files you want built in from the user/files
# directory.
#
STUDENTFILES =

###########################################################################
# Object files for your thread library
###########################################################################
THREAD_OBJS = malloc.o panic.o mutex.o linked_list.o thread.o \
			  xchange_stub.o cond.o sem.o rwlock.o

# Thread Group Library Support.
#
# Since libthrgrp.a depends on your thread library, the "buildable blank
# P3" we give you can't build libthrgrp.a.  Once you install your thread
# library and fix THREAD_OBJS above, uncomment this line to enable building
# libthrgrp.a:
410USER_LIBS_EARLY += libthrgrp.a

###########################################################################
# Object files for your syscall wrappers
###########################################################################
SYSCALL_OBJS = fork_stub.o exec_stub.o set_status_stub.o sleep_stub.o\
			   vanish_stub.o wait_stub.o task_vanish_stub.o gettid_stub.o \
			   yield_stub.o deschedule_stub.o make_runnable_stub.o \
			   get_ticks_stub.o new_pages_stub.o remove_pages_stub.o \
			   getchar_stub.o readline_stub.o print_stub.o \
			   set_term_color_stub.o set_cursor_pos_stub.o \
			   get_cursor_pos_stub.o halt_stub.o readfile_stub.o \
			   misbehave_stub.o swexn_stub.o thread_fork_stub.o

###########################################################################
# Object files for your automatic stack handling
###########################################################################
AUTOSTACK_OBJS = autostack.o

###########################################################################
# Parts of your kernel
###########################################################################
#
# Kernel object files you want included from 410kern/
#
410KERNEL_OBJS = load_helper.o
#
# Kernel object files you provide in from kern/
#
KERNEL_OBJS = console.o kernel.o loader.o malloc_wrappers.o interrupt.o \
			  handler_wrapper.o vm.o ctrl_blk.o execution_state.o mutex.o \
			  xchange_stub.o timer.o system_call.o fault_handler.o \
			  keyboard.o context.o context_switcher.o scheduler.o \
			  stop_stub.o mem_allocation.o segmentation.o \
			  virtual_interrupt.o

###########################################################################
# WARNING: Do not put **test** programs into the REQPROGS variables.  Your
#          kernel will probably not build in the test harness and you will
#          lose points.
###########################################################################

###########################################################################
# Mandatory programs whose source is provided by course staff
###########################################################################
# A list of the programs in 410user/progs which are provided in source
# form and NECESSARY FOR THE KERNEL TO RUN.
#
# The shell is a really good thing to keep here.  Don't delete idle
# or init unless you are writing your own, and don't do that unless
# you have a really good reason to do so.
#
410REQPROGS = idle init shell

###########################################################################
# Mandatory programs whose source is provided by you
###########################################################################
# A list of the programs in user/progs which are provided in source
# form and NECESSARY FOR THE KERNEL TO RUN.
#
# Leave this blank unless you are writing custom init/idle/shell programs
# (not generally recommended).  If you use STUDENTREQPROGS so you can
# temporarily run a special debugging version of init/idle/shell, you
# need to be very sure you blank STUDENTREQPROGS before turning your
# kernel in, or else your tweaked version will run and the test harness
# won't.
#
STUDENTREQPROGS =
