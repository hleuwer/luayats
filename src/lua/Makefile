MODULE = lua
PKG = yats.pkg
IPKG = cd.pkg
OBJS = yats.o yats_bind.o lua.o
VERSION = 0.1
topdir = ../..

BINDHEADERS = \
        ../kernel/data.h \
	../kernel/ino.h \
	../kernel/in1out.h \
	../kernel/inxout.h \
        ../kernel/queue.h \
	../kernel/oqueue.h \
	../kernel/special.h \
	../lua/yats.h \
        ../misc/dummy.h \
	../misc/line.h \
	../misc/sink.h \
	../misc/meas.h \
	../misc/meas2.h \
	../misc/meas3.h \
	../misc/distrib.h \
	../src/cbr.h \
	../src/bssrc.h \
	../src/geosrc.h \
        ../src/listsrc.h \
	../src/distsrc.h \
	../src/mmbp.h \
	../src/gmdp.h \
	../src/gmdpstop.h \
	../tcpip/dat2fram.h \
	../tcpip/cbrframe.h \
	../tcpip/tcpiprec.h \
	../tcpip/tcpipsend.h \
	../tcpip/termstrtstp.h \
	../win/winobj.h \
	../win/histo.h \
	../win/meter.h \
	../polshap/leakyb.h \
	../polshap/shap2.h \
	../user/agere_tm.h \
        ../user/data2frs.h \
	../user/marker.h \
	../user/framemarker.h \
	../user/measframe.h \
	../user/muxpacket.h \
	../user/leakybucket.h \
	../user/agere_tm.h \
	../user/setTrace.h \
	../user/fork.h \
	../user/tickctrl.h \
	../statist/confid.h \
	../muxdmx/mux.h \
	../muxdmx/muxdf.h \
	../muxdmx/muxaf.h \
	../muxdmx/muxdist.h \
	../muxdmx/wfqmux.h \
	../muxdmx/demux.h \
	../muxevt/muxBase.h \
	../muxevt/muxPrio.h \
	../muxevt/muxFrmPrio.h \
	../user/ethbridge.h \
	../rstp/rstp_bridge.h

include $(topdir)/rules.mk

TOLUACUST=./toluacust.lua

# Uncomment to override default settings, e.g. LD=c++ to link c++ objects.
MODCFLAGS=$(VERSION_INFO) -I../muxdmx -I../win -I../misc -I../tcpip -I../src -I../user -I../polshap -I../win -I../muxevt -DINFINITY=1e99 -DUSELUA=1 -DLUA_USERCONFIG=\"./saconfig.c\" -DUSE_READLINE
MODLDFLAGS=

# Uncomment one of the building rules below (either C or C++).
yats_bind.c yats_bind.h: $(PKG) $(BINDHEADERS) $(IPKG) $(TOLUACUST)
	$(TOLUA) -1 -L toluacust.lua -o yats_bind.c -H yats_bind.h $(PKG)

# Uncomment for having BINDH in dependency list
depend Makefile.deps: yats_bind.h

bind-clean: 
	$(MAKE) lib-uclean
	rm yats_bind.c yats_bind.h

binding::
	$(TOLUA) -1 -L toluacust.lua -o yats_bind.c -H yats_bind.h $(PKG)
ups::
	echo $(TOLUA)
	echo $(BINDHEADERS)
	echo $(IPKG)
	echo $(TOLUACUST)
	ls $(TOLUA)
	ls $(BINDHEADERS)
	ls $(IPKG)
	ls $(TOLUACUST)
-include Makefile.deps

