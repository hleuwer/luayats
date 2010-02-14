MODULE = polshap
PKG =
OBJS = leakyb.o  shap.o  shap2.o  shapctrl.o
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

