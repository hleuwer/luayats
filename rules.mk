# user customisation
include $(topdir)/config

# Lua

INSTALL_DIR=/usr/local


# include paths
YINCS = $(addprefix -I$(topdir)/src/, $(MODULES) rstp)
INCS =  -I. $(YINCS) -I../kernel -I/usr/local/include -I$(TOLUAINC) -I$(LUAINC) $(USERINCDIR) -I$(IUPINC) -I$(CDINC)

# compiler flags
CC = c++
LD = c++
#WARN = -Wall -Wno-deprecated
WARN = -Wall -Wno-write-strings
# CFLAGS = -DUSELUA -D__LINUX__ -Dexport=export_ -fwritable-strings -fno-operator-names $(INCS) $(WARN) $(MODCFLAGS) $(USERCFLAGS) 
CFLAGS = -DUSELUA -D__LINUX__ -Dexport=export_ -DCD_NO_OLD_INTERFACE -fno-operator-names $(WARN) $(INCS) $(MODCFLAGS) $(USERCFLAGS) 
LDFLAGS =  $(MODLDFLAGS) $(USERLDFLAGS)
LIBDIR = -L/usr/local/lib $(USERLIBDIR)
LIBS = -lm $(LUALIBS) $(IUPLIBS) $(CDLIBS)


# system capture
SYSTEM=$(shell uname -o)

ifeq ($(SYSTEM), Cygwin)
  DLIB = ../../lib/lib$(MODULE).dll
else
  DLIB = ../../lib/lib$(MODULE).so
endif
SLIB = ../../lib/lib$(MODULE).a

ifeq ($(SYSTEM), Cygwin)
# Cygwin build
lib:	$(SLIB)
$(DLIB): $(OBJS) Makefile.deps Makefile
	$(LD) -o $@ -shared  -Wl,--export-all-symbols,--output-def,$(MODULE).def,--out-implib,$(DLIB).a $(LDFLAGS) $(OBJS) $(LIBDIR) $(LIBS)

else

# Linux build - we build only static libraries
lib:	$(SLIB)

$(DLIB): $(OBJS) Makefile.deps Makefile
	$(LD) -o $@ -shared $(LDFLAGS) $(OBJS) $(LIBDIR) $(LIBS)
endif

$(SLIB): $(OBJS) Makefile Makefile.deps
	ar rcu $(SLIB) $(OBJS)
	ranlib $(SLIB)

.SUFFIXES: .o .s .c .cpp .cxx .lch .lc .lua .ro .rc

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

.cpp.o:
	$(CC) -c $(CFLAGS) -o $@ $<

.cpp.s:
	$(CC) -c -Wa,-alh,-L -dA $(CFLAGS) -o $@ $< > $*.L

.lua.lc:
	luac -o $@ $<

.lc.lch:
	bin2c $< > $@

.rc.ro:
	windres $(RCFLAGS) -O coff -o $@ $<

lib-clean: 
	rm -f $(OBJS) $(SLIB) $(DLIB) $(DLIB).a $(MODULE).def core core.* a.out *~ 	rm -f $(SLIB)
	rm -f *.da *.bbg *.c.gcov *.h.gcov

lib-uclean: lib-clean
	rm -f Makefile.deps

lib-depend Makefile.deps: 
	rm -f Makefile.deps
	test -z "$(OBJS)" || $(CC) -MM $(CFLAGS) $(OBJS:.o=.c) >> Makefile.deps
