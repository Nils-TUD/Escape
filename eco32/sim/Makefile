#
# Makefile for ECO32 simulator
#

BUILD = ../build

CC = gcc
CFLAGS = -Wall -g -I./getline -O3
LDFLAGS = -L./getline -L/usr/X11R6/lib -g
LDLIBS = -lgetline -lX11 -lpthread -lm

SRCS = main.c console.c error.c except.c command.c \
       instr.c asm.c disasm.c cpu.c mmu.c memory.c \
       timer.c dspkbd.c term.c disk.c output.c graph.c \
       trace/trace.c trace/simple.c trace/tree.c trace/map.c
OBJS = $(patsubst %.c,%.o,$(SRCS))
BIN = sim

.PHONY:		all install clean

all:		$(BIN)

install:	$(BIN)
		mkdir -p $(BUILD)/bin
		cp $(BIN) $(BUILD)/bin

getline/libgetline.a:
		$(MAKE) -C getline

$(BIN):		$(OBJS) getline/libgetline.a
		$(CC) $(LDFLAGS) -o $(BIN) $(OBJS) $(LDLIBS)

%.o:		%.c
		$(CC) $(CFLAGS) -o $@ -c $<

depend.mak:
		$(CC) -MM -MG $(CFLAGS) $(SRCS) > depend.mak

-include depend.mak

clean:
		$(MAKE) -C getline clean
		rm -f *~ $(OBJS) $(BIN) depend.mak
