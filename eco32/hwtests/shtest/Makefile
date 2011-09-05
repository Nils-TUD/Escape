#
# Makefile for shtest ROM
#

BUILD = ../../build

.PHONY:		all install run clean

all:		shtest-dos.exo

install:	shtest-dos.exo

shtest-dos.exo:	shtest.exo
		cp shtest.exo shtest-dos.exo
		unix2dos shtest-dos.exo

shtest.exo:	shtest.bin
		$(BUILD)/bin/bin2exo 0 shtest.bin shtest.exo

shtest.bin:	shtest.o
		$(BUILD)/bin/ld -h -rc 0xE0000000 -o shtest.bin shtest.o

shtest.o:	shtest.s
		$(BUILD)/bin/as -o shtest.o shtest.s

shtest.s:	mkshtest
		./mkshtest > shtest.s

mkshtest:	mkshtest.c
		gcc -Wall -o mkshtest mkshtest.c

run:		shtest.bin
		$(BUILD)/bin/sim -i -t 1 -r shtest.bin

clean:
		rm -f *~ mkshtest shtest.s shtest.o
		rm -f shtest.bin shtest.exo shtest-dos.exo
