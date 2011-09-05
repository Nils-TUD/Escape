#
# Makefile for memtest2 ROM
#

BUILD = ../../build

.PHONY:		all install run clean

all:		memtest2-dos.exo

install:	memtest2-dos.exo

memtest2-dos.exo:	memtest2.exo
		cp memtest2.exo memtest2-dos.exo
		unix2dos memtest2-dos.exo

memtest2.exo:	memtest2.bin
		$(BUILD)/bin/bin2exo 0 memtest2.bin memtest2.exo

memtest2.bin:	memtest2.o
		$(BUILD)/bin/ld -h -rc 0xE0000000 \
		  -o memtest2.bin memtest2.o

memtest2.o:	memtest2.s
		$(BUILD)/bin/as -o memtest2.o memtest2.s

run:		memtest2.bin
		$(BUILD)/bin/sim -i -t 1 -r memtest2.bin

clean:
		rm -f *~ memtest2.o memtest2.bin memtest2.exo memtest2-dos.exo
