#
# Makefile for remtest ROM
#

BUILD = ../../build

.PHONY:		all install run clean

all:		remtest-dos.exo

install:	remtest-dos.exo

remtest-dos.exo:	remtest.exo
		cp remtest.exo remtest-dos.exo
		unix2dos remtest-dos.exo

remtest.exo:	remtest.bin
		$(BUILD)/bin/bin2exo 0 remtest.bin remtest.exo

remtest.bin:	remtest.o
		$(BUILD)/bin/ld -h -rc 0xE0000000 -o remtest.bin remtest.o

remtest.o:	remtest.s
		$(BUILD)/bin/as -o remtest.o remtest.s

remtest.s:	mkremtest
		./mkremtest > remtest.s

mkremtest:	mkremtest.c
		gcc -Wall -o mkremtest mkremtest.c

run:		remtest.bin
		$(BUILD)/bin/sim -i -t 1 -r remtest.bin

clean:
		rm -f *~ mkremtest remtest.s remtest.o
		rm -f remtest.bin remtest.exo remtest-dos.exo
