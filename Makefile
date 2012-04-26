# Redis Makefile
# Copyright (C) 2009 Salvatore Sanfilippo <antirez at gmail dot com>
# This file is released under the BSD license, see the COPYING file

OPTIMIZATION?=
CFLAGS?=-std=c++0x $(OPTIMIZATION) -Wall $(PROF)
CCLINK?=
DEBUG?=-g -ggdb
CCOPT= $(CFLAGS) $(ARCH) $(PROF)

CCCOLOR="\033[34m"
LINKCOLOR="\033[34;1m"
SRCCOLOR="\033[33m"
BINCOLOR="\033[37;1m"
MAKECOLOR="\033[32;1m"
ENDCOLOR="\033[0m"

ifndef V
QUIET_CC = @printf '    %b %b\n' $(CCCOLOR)CXX$(ENDCOLOR) $(SRCCOLOR)$@$(ENDCOLOR);
QUIET_LINK = @printf '    %b %b\n' $(LINKCOLOR)LINK$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR);
endif

OBJ = bpt.o test.o
PRGNAME = bpt

all: bpt

run:
	./bpt

gprof:
	$(MAKE) PROF="-pg"

gcov:
	$(MAKE) PROF="-fprofile-arcs -ftest-coverage"

noopt:
	$(MAKE) OPTIMIZATION=""

clean:
	rm -rf $(PRGNAME) $(BENCHPRGNAME) $(CLIPRGNAME) $(CHECKDUMPPRGNAME) $(CHECKAOFPRGNAME) *.o *.gcda *.gcno *.gcov

distclean: clean
	$(MAKE) clean

dep:
	$(CC) -MM *.cc

bpt: $(OBJ)
	$(QUIET_LINK)$(CXX) -o $(PRGNAME) $(CCOPT) $(DEBUG) $(OBJ) $(CCLINK)

%.o: %.cc
	$(QUIET_CC)$(CXX) -c $(CFLAGS) $(DEBUG) $(COMPILE_TIME) $<

# Deps (use make dep to generate this)
bpt.o: bpt.cc bpt.h
test.o: test.cc bpt.h
