#
# Makefile for brtest ROM
#

BUILD = ../../build

.PHONY:		all install run clean

all:		brtest-dos.exo

install:	brtest-dos.exo

brtest-dos.exo:	brtest.exo
		cp brtest.exo brtest-dos.exo
		unix2dos brtest-dos.exo

brtest.exo:	brtest.bin
		$(BUILD)/bin/bin2exo 0 brtest.bin brtest.exo

brtest.bin:	brtest.o
		$(BUILD)/bin/ld -h -rc 0xE0000000 -o brtest.bin brtest.o

brtest.o:	brtest.s
		$(BUILD)/bin/as -o brtest.o brtest.s

brtest.s:	mkbrtest
		./mkbrtest > brtest.s

mkbrtest:	mkbrtest.c
		gcc -Wall -o mkbrtest mkbrtest.c

run:		brtest.bin
		$(BUILD)/bin/sim -i -t 1 -r brtest.bin

clean:
		rm -f *~ mkbrtest brtest.s brtest.o
		rm -f brtest.bin brtest.exo brtest-dos.exo
