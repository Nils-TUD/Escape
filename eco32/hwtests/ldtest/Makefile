#
# Makefile for ldtest ROM
#

BUILD = ../../build

.PHONY:		all install run clean

all:		ldtest-dos.exo

install:	ldtest-dos.exo

ldtest-dos.exo:	ldtest.exo
		cp ldtest.exo ldtest-dos.exo
		unix2dos ldtest-dos.exo

ldtest.exo:	ldtest.bin
		$(BUILD)/bin/bin2exo 0 ldtest.bin ldtest.exo

ldtest.bin:	ldtest.o
		$(BUILD)/bin/ld -h -rc 0xE0000000 -o ldtest.bin ldtest.o

ldtest.o:	ldtest.s
		$(BUILD)/bin/as -o ldtest.o ldtest.s

run:		ldtest.bin
		$(BUILD)/bin/sim -i -t 1 -r ldtest.bin

clean:
		rm -f *~ ldtest.o ldtest.bin ldtest.exo ldtest-dos.exo
