#
# Makefile for multest ROM
#

BUILD = ../../build

.PHONY:		all install run clean

all:		multest-dos.exo

install:	multest-dos.exo

multest-dos.exo:	multest.exo
		cp multest.exo multest-dos.exo
		unix2dos multest-dos.exo

multest.exo:	multest.bin
		$(BUILD)/bin/bin2exo 0 multest.bin multest.exo

multest.bin:	multest.o
		$(BUILD)/bin/ld -h -rc 0xE0000000 -o multest.bin multest.o

multest.o:	multest.s
		$(BUILD)/bin/as -o multest.o multest.s

multest.s:	mkmultest
		./mkmultest > multest.s

mkmultest:	mkmultest.c
		gcc -Wall -o mkmultest mkmultest.c

run:		multest.bin
		$(BUILD)/bin/sim -i -t 1 -r multest.bin

clean:
		rm -f *~ mkmultest multest.s multest.o
		rm -f multest.bin multest.exo multest-dos.exo
