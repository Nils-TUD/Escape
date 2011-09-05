#
# Makefile to build the boot manager
#

BUILD = ../../../../build

all:		boot.bin

boot.bin:	c0.o boot.o iolib.o c1.o
		$(BUILD)/bin/ld -h -rc 0xC00F0000 -o boot.bin \
		  c0.o boot.o iolib.o c1.o

c0.o:		c0.s
		$(BUILD)/bin/as -o c0.o c0.s

boot.o:		boot.c
		$(BUILD)/bin/lcc -A -c -o boot.o boot.c

iolib.o:	iolib.s
		$(BUILD)/bin/as -o iolib.o iolib.s

c1.o:		c1.s
		$(BUILD)/bin/as -o c1.o c1.s

clean:
		rm -f *~ c0.o boot.o iolib.o c1.o boot.bin
