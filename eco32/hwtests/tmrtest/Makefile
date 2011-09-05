#
# Makefile for tmrtest ROM
#

BUILD = ../../build

.PHONY:		all install run clean

all:		tmrtest-dos.exo

install:	tmrtest-dos.exo

tmrtest-dos.exo:	tmrtest.exo
		cp tmrtest.exo tmrtest-dos.exo
		unix2dos tmrtest-dos.exo

tmrtest.exo:	tmrtest.bin
		$(BUILD)/bin/bin2exo 0 tmrtest.bin tmrtest.exo

tmrtest.bin:	tmrtest.o
		$(BUILD)/bin/ld -h -rc 0xE0000000 \
		  -o tmrtest.bin tmrtest.o

tmrtest.o:	tmrtest.s
		$(BUILD)/bin/as -o tmrtest.o tmrtest.s

run:		tmrtest.bin
		$(BUILD)/bin/sim -i -t 1 -r tmrtest.bin

clean:
		rm -f *~ tmrtest.o tmrtest.bin tmrtest.exo tmrtest-dos.exo
