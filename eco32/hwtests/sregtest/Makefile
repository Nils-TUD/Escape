#
# Makefile for sregtest ROM
#

BUILD = ../../build

.PHONY:		all install run clean

all:		sregtest-dos.exo

install:	sregtest-dos.exo

sregtest-dos.exo:	sregtest.exo
		cp sregtest.exo sregtest-dos.exo
		unix2dos sregtest-dos.exo

sregtest.exo:	sregtest.bin
		$(BUILD)/bin/bin2exo 0 sregtest.bin sregtest.exo

sregtest.bin:	sregtest.o
		$(BUILD)/bin/ld -h -rc 0xE0000000 -o sregtest.bin sregtest.o

sregtest.o:	sregtest.s
		$(BUILD)/bin/as -o sregtest.o sregtest.s

run:		sregtest.bin
		$(BUILD)/bin/sim -i -t 1 -r sregtest.bin

clean:
		rm -f *~ sregtest.o sregtest.bin sregtest.exo sregtest-dos.exo
