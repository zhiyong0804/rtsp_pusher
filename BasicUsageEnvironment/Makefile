###############################################################################
#
# Generic Makefile for C/C++ Program
#
# Author: DengYi
# Date:   2011/12/19

# Description:
# The makefile searches in <SRCDIRS> directories for the source files
# with extensions specified in <SOURCE_EXT>, then compiles the sources
# and finally produces the <PROGRAM>, the executable file, by linking
# the objectives.

# Usage:
#   $ make           compile and link the program.
#   $ make objs      compile only (no linking. Rarely used).
#   $ make clean     clean the objectives and dependencies.
#   $ make cleanall  clean the objectives, dependencies and executable.
#   $ make rebuild   rebuild the program. The same as make clean && make all.
#==============================================================================

## Customizing Section: adjust the following if necessary.
##=============================================================================

# The executable file name.
# It must be specified.
# PROGRAM   := a.out    # the executable name
PROGRAM   := libUsageEnvironment.so
  
# The directories in which source files reside.
# At least one path should be specified.
# SRCDIRS   := .        # current directory
SRCDIRS   := .

# The source file types (headers excluded).
# At least one type should be specified.
# The valid suffixes are among of .c, .C, .cc, .cpp, .CPP, .c++, .cp, or .cxx.
# SRCEXTS   := .c      # C program
# SRCEXTS   := .cpp    # C++ program
# SRCEXTS   := .c .cpp # C/C++ program
SRCEXTS   := .cpp

RELEASE :=
DEBUG := -g

# The flags used by the cpp (man cpp for more).
# CPPFLAGS  := -Wall -Werror # show all warnings and take them as errors
CPPFLAGS  := $(DEBUG) -I. -I./include

# The compiling flags used only for C.
# If it is a C++ program, no need to set these flags.
# If it is a C and C++ merging program, set these flags for the C parts.
CFLAGS    :=
CFLAGS    += -I. -I./include -fPIC 

# The compiling flags used only for C++.
# If it is a C program, no need to set these flags.
# If it is a C and C++ merging program, set these flags for the C++ parts.
CXXFLAGS  :=
CXXFLAGS  += -fPIC

# The library and the link options ( C and C++ common).
LDFLAGS   :=
LDFLAGS   += 

## Implict Section: change the following only when necessary.
##=============================================================================
# The C program compiler. Uncomment it to specify yours explicitly.
CC      = gcc

# The C++ program compiler. Uncomment it to specify yours explicitly.
CXX     = g++

# Uncomment the 2 lines to compile C programs as C++ ones.
#CC      = $(CXX)
#CFLAGS  = $(CXXFLAGS)

STRIP = strip

# The command used to delete file.
RM        = rm -f

## Stable Section: usually no need to be changed. But you can add more.
##=============================================================================
SHELL   = /bin/sh
SOURCES = $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))
OBJS    = $(foreach x,$(SRCEXTS), \
      $(patsubst %$(x),%.o,$(filter %$(x),$(SOURCES))))
DEPS    = $(patsubst %.o,%.d,$(OBJS))

.PHONY : all objs clean cleanall rebuild

all : $(PROGRAM)
#	$(STRIP) $(PROGRAM)

# Rules for creating the dependency files (.d).
#---------------------------------------------------
#%.d : %.c
#	@$(CC) -M -MD $(CFLAGS) $<

#%.d : %.C
#	@$(CC) -M -MD $(CXXFLAGS) $<

#%.d : %.cc
#	@$(CC) -M -MD $(CXXFLAGS) $<

#%.d : %.cpp
#	@$(CC) -M -MD $(CXXFLAGS) $<

#%.d : %.CPP
#	@$(CC) -M -MD $(CXXFLAGS) $<

#%.d : %.c++
#	@$(CC) -M -MD $(CXXFLAGS) $<

#%.d : %.cp
#	@$(CC) -M -MD $(CXXFLAGS) $<

#%.d : %.cxx
#	@$(CC) -M -MD $(CXXFLAGS) $<

# Rules for producing the objects.
#---------------------------------------------------
objs : $(OBJS)

%.o : %.c
	$(CC) -c  $(CPPFLAGS) $(CFLAGS) $< 

%.o : %.C
	$(CXX) -c  $(CPPFLAGS) $(CXXFLAGS) $<

%.o : %.cc
	$(CXX) -c  $(CPPFLAGS) $(CXXFLAGS) $<

%.o : %.cpp
	$(CXX) -c  $(CPPFLAGS) $(CXXFLAGS) $<

%.o : %.CPP
	$(CXX) -c  $(CPPFLAGS) $(CXXFLAGS) $<

%.o : %.c++
	$(CXX -c  $(CPPFLAGS) $(CXXFLAGS) $<

%.o : %.cp
	$(CXX) -c  $(CPPFLAGS) $(CXXFLAGS) $<

%.o : %.cxx
	$(CXX) -c  $(CPPFLAGS) $(CXXFLAGS) $<

# Rules for producing the executable.
#----------------------------------------------
$(PROGRAM) : $(OBJS)
ifeq ($(strip $(SRCEXTS)), .c)  # C file
	$(CC) -o $(PROGRAM) $(OBJS) $(LDFLAGS)
else                            # C++ file
ifeq ($(strip $(PROGRAM)), .a)  # a file
	$(AR) cru $(PROGRAM) *.o 
	$(RANLIB) $(PROGRAM)
else							# so file
	$(CXX) *.o --share -fPIC -o $(PROGRAM)
#	$(STRIP) $(PROGRAM)
endif
#	$(MV) $(PROGRAM) ./lib
endif

-include $(DEPS)

rebuild: clean all

clean :
	@$(RM) *.o *.d

cleanall: clean
	@$(RM) $(PROGRAM) 

### End of the Makefile ##  Suggestions are welcome  ## All rights reserved ###
###############################################################
