MODULE = kernel
PKG =
#OBJS = all.o deriv.o id.o inxout.o main.o scan.o valArray.o\
#class.o expr.o in1out.o macro.o	parse.o sim.o\
#data.o geo1.o ino.o macshell.o root.o symb.o
OBJS = all.o deriv.o inxout.o \
       class.o in1out.o sim.o main.o \
       data.o geo1.o ino.o root.o
topdir = ../..

VERSION = 0.1

# Adjust libraries to your needs

include $(topdir)/rules.mk

# Uncomment to override default settings, e.g. LD=c++ to link c++ objects.
MODCFLAGS=
MODLDFLAGS=

# Uncomment one of the building rules below (either C or C++).
# yats_bind.cpp yats_bind.hpp: yats.pkg 
#	$(TOLUA)/bin/tolua++ -1 -o yats_bind.cpp -H yats_bind.hpp $(PKG)

# Uncomment for having BINDH in dependency list
# depend Makefile.deps: $(BINDH)

-include Makefile.deps

