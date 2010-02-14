MODULE = muxevt
PKG =
OBJS = muxAsyncAF.o  muxAsyncDF.o  muxBase.o  muxEvtEPD.o  muxInpBuf.o\
 muxPrio.o muxSyncAF.o   muxSyncDF.o
VERSION = 0.1
topdir=../..

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

