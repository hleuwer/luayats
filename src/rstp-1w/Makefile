MODULE = rstp
PKG = bridge.pkg
OBJS =  rstp_bridge.o \
	stpm.o \
	edge.o \
	migrate.o \
	p2p.o \
	pcost.o \
	port.o \
	portinfo.o \
	rolesel.o \
	roletrns.o \
	statmch.o \
	stp_in.o \
	sttrans.o \
	times.o \
	topoch.o \
	transmit.o \
	vector.o \
        stpmgmt.o 

topdir=../..

VERSION = 0.1

include $(topdir)/rules.mk

# Uncomment to override default settings, e.g. LD=c++ to link c++ objects.
#MODCFLAGS=-D__LINUX__ -DSTP_DBG=1 -DSTRONGLY_SPEC_802_1W=1
MODCFLAGS=-D__LINUX__ -DSTP_DBG=1
MODLDFLAGS=

# Uncomment one of the building rules below (either C or C++).
# bridge_bind.c bridge_bind.h: bridge.pkg stp_in.h bitmap.h uid_stp.h
#	$(TOLUA) -1 -o bridge_bind.c -H bridge_bind.h $(PKG)

# Uncomment for having BINDH in dependency list
depend Makefile.deps: 

-include Makefile.deps

