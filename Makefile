#
# Makefile for Yats
#
include config

topdir=.
TARGETLUA=bin/luayats
TARGET=bin/luayats-bin$(EXE)
YLIBS = $(addsuffix .a, $(addprefix lib/lib, $(MODULES)))
DOCDIR = $(topdir)/doc
LUADOCDIR = $(DOCDIR)/lua
LUASRCS = $(addprefix yats/, $(addsuffix .lua, $(LUAMODULES)))
LUADOCS = $(addprefix $(LUADOCDIR), $(LUASRCS:.lua=.html)) $(LUADOCDIR)/index.html \
	  $(LUADOCDIR)/luadoc.css
CPPDOCDIR = $(DOCDIR)/cpp
SHELL = /bin/sh
TAGNAME=LUAYATS-$(MAJOR)_$(MINOR)
EXPORTDIR=$(HOME)/exports
DISTNAME=luayats-$(MAJOR).$(MINOR)
DISTARCH=$(DISTNAME).tar.gz
DISTLATESTNAME=luayats-latest-$(MAJOR).$(MINOR)
DISTLATESTARCH=$(DISTLATESTNAME).tar.gz
SVNMODULE=luayats-$(MAJOR).0
INSTALL_ROOT=/usr/local/share/luayats
ifeq ($(SYSTEM), Cygwin)
  RCOBJS=$(topdir)/src/lua/yats.ro
else
  RCOBJS=
endif
.PHONY: all libs clean uclean doc-clean depend dist-svn dist-git dist tag doc doxy doxy-clean
.PHONY: install-code install-doc install-examples install
.PHONY: uninstall-code uninstall-doc uninstall-examples uninstall

all: libs $(TARGETLUA)

include rules.mk

libs:
	mkdir -p ./lib
	for i in $(MODULES); do $(MAKE) -C src/$$i lib; done

$(TARGET): $(YLIBS) $(RCOBJS)
	mkdir -p bin
	$(CC) $(LDFLAGS) $(RCOBJS) $(YLIBS) $(YLIBS) $(LIBDIR) $(LIBS) $(LUASLIBS) -o $@

$(TARGETLUA): $(TARGET)
	cd bin;ln -f -s ../yats/luayats.lua luayats;chmod +x ../yats/luayats.lua

clean:
	for i in $(MODULES); do $(MAKE) -C src/$$i lib-clean; done
	rm -f $(TARGET) *.stackdump *~
	rm -f $(TARGETLUA)
	rm -f lua/*~
	rm -f gmon.out
	rm -f $(RCOBJS)

uclean:  
	for i in $(MODULES); do $(MAKE) -C src/$$i lib-uclean; done
	rm -f $(TARGET) *.stackdump *~ examples/*~
	rm -f $(TARGETLUA)
	rm -f src/lua/yats_bind.[ch]
	rm -f `find . -name "semantic.cache"`
	rm -rf lib bin
	rm -rf `find . -name "*~"`

doc-clean::
	rm -rf $(LUADOCDIR)

depend:
	for i in $(MODULES); do $(MAKE) -C src/$$i lib-depend; done

luadoc::
	mkdir -p $(LUADOCDIR)
	luadoc --nomodules -d $(LUADOCDIR) $(LUASRCS)

doc::
	$(MAKE) luadoc
	$(MAKE) doxy
	$(MAKE) luadocindex
	$(MAKE) refindex

cdoc doxy::	
	mkdir -p $(CPPDOCDIR)
	doxygen doc/doxygen.conf

cdoc-clean doxy-clean::
	rm -rf $(CPPDOCDIR)

refindex:: $(CPPDOCDIR)/luayats.xml
	luayats-bin -l iuplua yats/luayats.lua -R doc/ldocindex.lua -d $(CPPDOCDIR)/luayats.xml -o doc/refindex.html
#	luayats -R doc/ldocindex.lua -d $(CPPDOCDIR)/luayats.xml -o doc/refindex.html

luadocindex:
	luayats -l doc/lua/files/yats -o doc/ldocindex.lua

info::
	luayats -i a > INFO

tag::
	echo "Using GIT - no tags yet"

taglatest::
	echo "Using GIT - no tags yet"

distrelease-svn:: 
	svn export $(REPOSITORY)/$(SVNMODULE)/tags/release-$(MAJOR).$(MINOR) $(EXPORTDIR)/$(DISTNAME)
	cd $(EXPORTDIR); tar -cvzf $(DISTARCH) $(DISTNAME)/*
	rm -rf $(EXPORTDIR)/$(DISTNAME)

dist-svn: 
	svn export $(REPOSITORY)/$(SVNMODULE)/trunk $(EXPORTDIR)/$(DISTNAME)
	cd $(EXPORTDIR); tar -cvzf $(DISTARCH) $(DISTNAME)/*
	rm -rf $(EXPORTDIR)/$(DISTNAME)

dist-git:
	mkdir -p $(EXPORTDIR)
	git archive --format=tar --prefix=$(DISTNAME)/ HEAD | gzip >$(EXPORTDIR)/$(DISTARCH)

dist: dist-git
testresult::
	LUAYATSTESTMODE=w luayats -n examples/test-all.lua

test::
	LUAYATSTESTMODE=t luayats -n examples/test-all.lua

install-code:
	mkdir -p /usr/local/bin
	cp -f $(TARGET) /usr/local/bin
	cp -f yats/luayats.lua /usr/local/bin/luayats
	chmod +x /usr/local/bin/luayats
	mkdir -p $(INSTALL_ROOT)
	cp -rf yats $(INSTALL_ROOT)

install-doc:
	mkdir -p $(INSTALL_ROOT)/doc
	cd doc; cp -rf * $(INSTALL_ROOT)/doc
	mkdir -p /usr/local/share/doc
	cd /usr/local/share/doc; ln -f -s $(INSTALL_ROOT)/doc luayats

install-examples:
	mkdir -p $(INSTALL_ROOT)/examples
	cd examples; cp -rf * $(INSTALL_ROOT)/examples

install: install-code install-doc

uninstall-code:
	rm -rf /usr/local/bin/$(TARGET)
	rm -rf /usr/local/bin/luayats
	rm -rf $(INSTALL_ROOT)/yats

uninstall-doc:
	rm -rf /$(INSTALL_ROOT)/doc
	rm -f /usr/local/share/doc/luayats

uninstall-examples:
	rm -rf $(INSTALL_ROOT)/examples

uninstall: uninstall-code uninstall-doc uninstall-examples
	rm -rf $(INSTALL_ROOT)