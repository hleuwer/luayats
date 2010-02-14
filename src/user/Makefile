MODULE = user
PKG =
OBJS = $(shell cat OBJLIST)
VERSION = 0.1
topdir = ../..
# Adjust libraries to your needs

include $(topdir)/rules.mk

# Uncomment to override default settings, e.g. LD=c++ to link c++ objects.
MODCFLAGS=-DINFINITY=1e99 -I../muxdmx -I../misc -I../tcpip
# MODCFLAGS= -I../muxdmx -I../misc -I../tcpip
MODLDFLAGS=

# Uncomment one of the building rules below (either C or C++).
# yats_bind.cpp yats_bind.hpp: yats.pkg 
#	$(TOLUA)/bin/tolua++ -1 -o yats_bind.cpp -H yats_bind.hpp $(PKG)

# Uncomment for having BINDH in dependency list
# depend Makefile.deps: $(BINDH)

-include Makefile.deps

