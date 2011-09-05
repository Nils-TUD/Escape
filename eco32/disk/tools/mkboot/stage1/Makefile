#
# Makefile to build the boot record
#

BUILD = ../../../../build

all:		br.bin

br.bin:		br.o
		$(BUILD)/bin/ld -h -rc 0xC0000000 -o br.bin br.o

br.o:		br.s
		$(BUILD)/bin/as -o br.o br.s

clean:
		rm -f *~ br.o br.bin
