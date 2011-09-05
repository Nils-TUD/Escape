#
# Makefile for jalrtest ROM
#

BUILD = ../../build

.PHONY:		all install run clean

all:		jalrtest-dos.exo

install:	jalrtest-dos.exo

jalrtest-dos.exo:	jalrtest.exo
		cp jalrtest.exo jalrtest-dos.exo
		unix2dos jalrtest-dos.exo

jalrtest.exo:	jalrtest.bin
		$(BUILD)/bin/bin2exo 0 jalrtest.bin jalrtest.exo

jalrtest.bin:	jalrtest.o
		$(BUILD)/bin/ld -h -rc 0xE0000000 \
		  -o jalrtest.bin jalrtest.o

jalrtest.o:	jalrtest.s
		$(BUILD)/bin/as -o jalrtest.o jalrtest.s

run:		jalrtest.bin
		$(BUILD)/bin/sim -i -t 1 -r jalrtest.bin

clean:
		rm -f *~ jalrtest.o jalrtest.bin jalrtest.exo jalrtest-dos.exo
