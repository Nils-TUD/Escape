#
# Makefile for looptest ROM
#

BUILD = ../../build

.PHONY:		all install run clean

all:		looptest-dos.exo

install:	looptest-dos.exo

looptest-dos.exo:	looptest.exo
		cp looptest.exo looptest-dos.exo
		unix2dos looptest-dos.exo

looptest.exo:	looptest.bin
		$(BUILD)/bin/bin2exo 0 looptest.bin looptest.exo

looptest.bin:	looptest.o
		$(BUILD)/bin/ld -h -rc 0xE0000000 \
		  -o looptest.bin looptest.o

looptest.o:	looptest.s
		$(BUILD)/bin/as -o looptest.o looptest.s

looptest.s:	mklooptest
		./mklooptest > looptest.s

mklooptest:	mklooptest.c
		gcc -Wall -o mklooptest mklooptest.c

run:		looptest.bin
		$(BUILD)/bin/sim -i -t 1 -r looptest.bin

clean:
		rm -f *~ mklooptest looptest.s looptest.o
		rm -f looptest.bin looptest.exo looptest-dos.exo
