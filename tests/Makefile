# Generated automatically from Makefile.in by configure.
# Dirt - Mud Client for Unix (mcl fork)
# Makefile.in - this file is processed by configure to create the Makefile
# created by Rodrigo Parra Novo <rodarvus@conectiva.com.br> - 19990907
# first based on the Makefile of mcl version 0.5something

# Common autoconf flags
CC=gcc
CXX=c++
CFLAGS=-g
CPPFLAGS=
CXXFLAGS=-g
DEFS= 
EXE=OptionTester
INSTALL=/usr/bin/install -c
LDFLAGS=-g
LIBS=
prefix=/usr/local
INSTALL_ROOT=
COMPILED_BY=mcelrath@draal.physics.wisc.edu
MODULES=perl 

# Common programs
LN_S=ln -s
RM=/bin/rm
FIND=/usr/bin/find
XARGS=/usr/bin/xargs
CTAGS=/bin/ctags
ECHO=/bin/echo

# Compile with perl libraries?
# Shut up the compiler about something caused by perl 5.6
PERL=/bin/perl
PERL_CPPFLAGS= -I/usr/lib/perl5/5.00503/alpha-linux/CORE  -Wno-unused
PERL_LDFLAGS=-rdynamic  -L/usr/local/lib /usr/lib/perl5/5.00503/alpha-linux/auto/DynaLoader/DynaLoader.a -L/usr/lib/perl5/5.00503/alpha-linux/CORE -lperl -lnsl -ldl -lm -lc -lcrypt

# Compile with python libraries?
PYTHON=no
PYTHON_CPPFLAGS=
PYTHON_LDFLAGS=

# Extra libraries
LIBDL=-ldl
LIBEFENCE=
LIBM=-lm
LIBNCURSES=-lncurses
LIBPTHREAD=-lpthread
LIBZ=-lz
LIBSTDCPP=-lstdc++

# Extra compiler flags
HAVE_FLAG_NO_RTTI=-fno-rtti
HAVE_FLAG_NO_EXCEPTIONS=-fno-exceptions
LOCAL_CFLAGS=-Wall -pipe
LOCAL_CPPFLAGS=-I../h
LOCAL_CXXFLAGS=-W -Wall -pipe -I../h
LOCAL_LDFLAGS=-rdynamic
LOCAL_LIBS=

# Common flags
DIRT_CFLAGS=$(CFLAGS) $(CPPFLAGS) $(DEFS) $(LOCAL_CFLAGS) $(LOCAL_CPPFLAGS)
DIRT_CXXFLAGS=$(CXXFLAGS) $(CPPFLAGS) $(DEFS) $(HAVE_FLAG_NO_RTTI) \
	$(HAVE_FLAG_NO_EXCEPTIONS) $(LOCAL_CXXFLAGS) $(LOCAL_CPPFLAGS)
DIRT_LDFLAGS=$(LDFLAGS) $(LOCAL_LDFLAGS)
DIRT_LIBS=$(LIBS) $(LOCAL_LIBS) $(LIBDL) $(LIBEFENCE) $(LIBM) $(LIBNCURSES) $(LIBZ) $(LIBSTDCPP)

# Source files
SRC_FILES := $(wildcard *.cc)

# Object files
OBJ_DIR=../o
OBJ_FILES := $(patsubst %.cc,$(OBJ_DIR)/%.o,$(SRC_FILES))
#DEPS := $(patsubst %, $(OBJ_DIR)/plugins/%.so, $(MODULES))

# Include files
INC_FILES := $(wildcard h/*.h)

.PHONY: all distclean clean tidy install

all: $(EXE) #$(DEPS)

distclean: clean
	$(RM) -f config.* Makefile VERSION dirt.spec h/config.h

clean: tidy 
	$(RM) -f $(OBJ_FILES) $(EXE) o/plugins/*.so o/plugins/*.o .depend

tidy:
	$(RM) -f *~ *orig *bak *rej doc/*~ samples/*~
	$(FIND) samples -name "*~" -print | $(XARGS) $(RM) -f

.depend:
	$(CXX) -MM -I../h $(SRC_FILES) > .depend
	$(PERL) -i -p -e 's/([^.]+)\.o/o\/\1.o/g' .depend

tags:	$(SRC_FILES) $(INC_FILES)
	$(CTAGS) $(SRC_FILES) $(INC_FILES)

OptionTester: OptionTester.o ../o/Option.o ../h/Option.h
	$(CC) $(DIRT_CXXFLAGS) $(DIRT_LDFLAGS) -o OptionTester ../o/Option.o OptionTester.o $(DIRT_LIBS)

# Link with $(CC) to avoid the c++ libs to be linked
#$(EXE): $(OBJ_FILES)
#	$(CC) $(DIRT_LDFLAGS) -o $(EXE) $(OBJ_FILES) $(DIRT_LIBS)

$(OBJ_DIR)/%.o: ../%.cc
	$(CXX) -c $(DIRT_CXXFLAGS) $< -o $@

%.o: %.cc
	$(CXX) -c $(DIRT_CXXFLAGS) $< -o $@

include .depend
