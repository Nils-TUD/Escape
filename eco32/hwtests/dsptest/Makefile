#
# Makefile for dsptest ROM
#

BUILD = ../../build

DSPTEST_SRC = start.s main.c lib.c serial.s end.s

.PHONY:		all install run clean

all:		dsptest-dos.exo

install:	dsptest-dos.exo

dsptest-dos.exo:	dsptest.exo
		cp dsptest.exo dsptest-dos.exo
		unix2dos dsptest-dos.exo

dsptest.exo:	dsptest.bin
		$(BUILD)/bin/bin2exo 0 dsptest.bin dsptest.exo

dsptest.bin:	$(DSPTEST_SRC)
		$(BUILD)/bin/lcc -A -Wo-rom -Wl-rd -Wl0xC03F0000 \
		  -Wl-m -Wldsptest.map -o dsptest.bin $(DSPTEST_SRC)

run:		dsptest.bin
		$(BUILD)/bin/sim -i -c -r dsptest.bin

clean:
		rm -f *~
		rm -f dsptest.map dsptest.bin dsptest.exo dsptest-dos.exo
