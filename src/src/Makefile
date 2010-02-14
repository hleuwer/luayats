MODULE = src
PKG =
OBJS = bssrc.o cbr.o distsrc.o filsrc.o geosrc.o gmdp.o listsrc.o\
       mmbp.o modbp.o xx2.o gmdpstop.o
topdir=../..
VERSION = 0.1

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

