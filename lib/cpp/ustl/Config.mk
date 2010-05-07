################ Build options #######################################

NAME		:= ustl
MAJOR		:= 1
MINOR		:= 4

DEBUG		:= 1
BUILD_SHARED	:= 1
#BUILD_STATIC	:= 1
NOLIBSTDCPP	:= 1

################ Programs ############################################

CXX		:= g++
LD		:= g++
AR		:= ar
RANLIB		:= ranlib
DOXYGEN		:= doxygen
INSTALL		:= install

INSTALLDATA	:= ${INSTALL} -D -p -m 644
INSTALLLIB	:= ${INSTALLDATA}

################ Destination #########################################

prefix		:= /usr/local
exec_prefix	:= /usr/local
BINDIR		:= /usr/local/bin
INCDIR		:= /usr/local/include
LIBDIR		:= /usr/local/lib

################ Compiler options ####################################

WARNOPTS	:= -Wall -Wextra -Wpointer-arith -Wcast-qual -Wsynth \
		-Wwrite-strings -Wno-cast-align -Wno-strict-aliasing \
		-Woverloaded-virtual -Wno-overflow \
		-Wsign-promo -Wshadow
#TGTOPTS		:= -mmmx -msse -mfpmath=sse -msse2
INLINEOPTS	:= -fvisibility-inlines-hidden -fno-threadsafe-statics -fno-enforce-eh-specs

CXXFLAGS	:= ${WARNOPTS} ${TGTOPTS} -ffreestanding -nostdlib -fno-builtin -I../../libc/include \
	-I../../lib/h
LDFLAGS		:= -nostdlib -Wl,--build-id=none
LIBS		:=
ifdef DEBUG
    CXXFLAGS	+= -O0 -g
    LDFLAGS	+= -rdynamic
else
    CXXFLAGS	+= -Os -g0 -DNDEBUG=1 -fomit-frame-pointer ${INLINEOPTS}
    LDFLAGS	+= -s -Wl,-gc-sections
endif
ifdef NOLIBSTDCPP
    LD		:= gcc
    STAL_LIBS	:= $(BUILD)/libsupc++.a ../../libgcc/libgcc_eh.a
    LIBS	:= ${STAL_LIBS}
endif
O		:= $(BUILD)/libcpp/ustl/

slib_lnk	= lib$1.so
slib_son	= lib$1.so.${MAJOR}.${MINOR}
slib_tgt	= lib$1.so.${MAJOR}.${MINOR}
slib_flags	= -shared -Wl,-soname=$1 -Wl,-T,../shld.conf
