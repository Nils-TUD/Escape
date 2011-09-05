#
# Makefile for kbdtest ROM
#

BUILD = ../../build

KBDTEST_SRC = start.s main.c lib.c serial.s end.s

.PHONY:		all install run clean

all:		kbdtest-dos.exo

install:	kbdtest-dos.exo

kbdtest-dos.exo:	kbdtest.exo
		cp kbdtest.exo kbdtest-dos.exo
		unix2dos kbdtest-dos.exo

kbdtest.exo:	kbdtest.bin
		$(BUILD)/bin/bin2exo 0 kbdtest.bin kbdtest.exo

kbdtest.bin:	$(KBDTEST_SRC)
		$(BUILD)/bin/lcc -A -Wo-rom -Wl-rd -Wl0xC03F0000 \
		  -Wl-m -Wlkbdtest.map -o kbdtest.bin $(KBDTEST_SRC)

run:		kbdtest.bin
		$(BUILD)/bin/sim -i -c -t 1 -r kbdtest.bin

clean:
		rm -f *~
		rm -f kbdtest.map kbdtest.bin kbdtest.exo kbdtest-dos.exo
