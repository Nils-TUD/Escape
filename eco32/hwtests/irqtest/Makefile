#
# Makefile for irqtest ROM
#

BUILD = ../../build

.PHONY:		all install run clean

all:		irqtest-dos.exo

install:	irqtest-dos.exo

irqtest-dos.exo:	irqtest.exo
		cp irqtest.exo irqtest-dos.exo
		unix2dos irqtest-dos.exo

irqtest.exo:	irqtest.bin
		$(BUILD)/bin/bin2exo 0 irqtest.bin irqtest.exo

irqtest.bin:	irqtest.o
		$(BUILD)/bin/ld -h -rc 0xE0000000 \
		  -o irqtest.bin irqtest.o

irqtest.o:	irqtest.s
		$(BUILD)/bin/as -o irqtest.o irqtest.s

run:		irqtest.bin
		$(BUILD)/bin/sim -i -t 1 -r irqtest.bin

clean:
		rm -f *~ irqtest.o irqtest.bin irqtest.exo irqtest-dos.exo
