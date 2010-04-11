# Makefile to build D compiler runtime library for Linux
# Designed to work with GNU make
# Targets:
#	make
#		Same as make all
#	make lib
#		Build library
#   make doc
#       Generate documentation
#	make clean
#		Delete unneeded files created by build process

CP=cp -f
RM=rm -f
MD=mkdir -p

CFLAGS=-O -m32
#CFLAGS=-g -m32

DFLAGS=-release -O -inline -version=Posix -w
#DFLAGS=-release -O -inline -version=Posix -I.. -w
#DFLAGS=-g -release -version=Posix -I.. -w

TFLAGS=-O -inline -version=Posix -w
#TFLAGS=-O -inline -version=Posix -I.. -w
#TFLAGS=-g -version=Posix -I. -w

DOCFLAGS=-version=DDoc -version=Posix
#DOCFLAGS=-version=DDoc -version=Posix -I..

CC=gcc
LC=$(AR) -qsv
DC=dmd

LIB_DEST=..

.SUFFIXES: .s .S .c .cpp .d .html .o

.s.o:
	$(CC) -c $(CFLAGS) $< -o$@

.S.o:
	$(CC) -c $(CFLAGS) $< -o$@

.c.o:
	$(CC) -c $(CFLAGS) $< -o$@

.cpp.o:
	g++ -c $(CFLAGS) $< -o$@

.d.o:
	$(DC) -c $(DFLAGS) $< -of$@

.d.html:
	$(DC) -c -o- $(DOCFLAGS) -Df$*.html dmd.ddoc $<

targets : lib doc
all     : lib doc
lib     : dmd.lib
doc     : dmd.doc

######################################################

OBJ_BASE= \
    aaA.o \
    aApply.o \
    aApplyR.o \
    adi.o \
    alloca.o \
    arraycast.o \
    arraycat.o \
    cast.o \
    cmath2.o \
    complex.o \
    cover.o \
    critical.o \
    deh2.o \
    dmain2.o \
    genobj.o \
    invariant.o \
    lifetime.o \
    llmath.o \
    memory.o \
    memset.o \
    monitor.o \
    obj.o \
    qsort.o \
    switch.o \
    moduleinit.o \
    trace.o
# NOTE: trace.obj and cover.obj are not necessary for a successful build
#       as both are used for debugging features (profiling and coverage)
# NOTE: a pre-compiled minit.obj has been provided in dmd for Win32 and
#       minit.asm is not used by dmd for linux
# NOTE: deh.o is only needed for Win32, linux uses deh2.o

OBJ_UTIL= \
    util/console.o \
    util/ctype.o \
    util/string.o \
    util/utf.o

OBJ_TI= \
    typeinfo/ti_AC.o \
    typeinfo/ti_Acdouble.o \
    typeinfo/ti_Acfloat.o \
    typeinfo/ti_Acreal.o \
    typeinfo/ti_Adouble.o \
    typeinfo/ti_Afloat.o \
    typeinfo/ti_Ag.o \
    typeinfo/ti_Aint.o \
    typeinfo/ti_Along.o \
    typeinfo/ti_Areal.o \
    typeinfo/ti_Ashort.o \
    typeinfo/ti_byte.o \
    typeinfo/ti_C.o \
    typeinfo/ti_cdouble.o \
    typeinfo/ti_cfloat.o \
    typeinfo/ti_char.o \
    typeinfo/ti_creal.o \
    typeinfo/ti_dchar.o \
    typeinfo/ti_Delegate.o \
    typeinfo/ti_double.o \
    typeinfo/ti_float.o \
    typeinfo/ti_idouble.o \
    typeinfo/ti_ifloat.o \
    typeinfo/ti_int.o \
    typeinfo/ti_ireal.o \
    typeinfo/ti_long.o \
    typeinfo/ti_ptr.o \
    typeinfo/ti_real.o \
    typeinfo/ti_short.o \
    typeinfo/ti_ubyte.o \
    typeinfo/ti_uint.o \
    typeinfo/ti_ulong.o \
    typeinfo/ti_ushort.o \
    typeinfo/ti_void.o \
    typeinfo/ti_wchar.o

ALL_OBJS= \
    $(OBJ_BASE) \
    $(OBJ_UTIL) \
    $(OBJ_TI)

######################################################

ALL_DOCS=

######################################################

dmd.lib : libdmd.a

libdmd.a : $(ALL_OBJS)
	$(RM) $@
	$(LC) $@ $(ALL_OBJS)

dmd.doc : $(ALL_DOCS)
	echo No documentation available.

######################################################

clean :
	find . -name "*.di" | xargs $(RM)
	$(RM) $(ALL_OBJS)
	$(RM) $(ALL_DOCS)
	$(RM) libdmd*.a

install :
	$(MD) $(LIB_DEST)
	$(CP) libdmd*.a $(LIB_DEST)/.
