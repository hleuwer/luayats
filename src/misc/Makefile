MODULE = misc
PKG =
OBJS = signal.o line.o sink.o sinktrac.o xx2sink.o meas.o meas2.o meas3.o distrib.o ip2atm.o timestp.o typchk.o dummy.o
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

