#
# Makefile for ECO32 ROM monitor
#

BUILD = ../../build

SRC = start.s main.c command.c instr.c asm.c disasm.c \
      boot.c cpu.c mmu.c getline.c romlib.c \
      keyboard.s ../kbdtbls/kbdtbls.s display.s \
      serial.s sctio-ctl.s sctio-ser.s end.s

.PHONY:		all install clean

all:		monitor.bin monitor-dos.exo

install:	monitor.bin monitor-dos.exo
		mkdir -p $(BUILD)/monitor
		cp monitor.bin $(BUILD)/monitor
		cp monitor-dos.exo $(BUILD)/monitor

monitor-dos.exo:	monitor.exo
		cp monitor.exo monitor-dos.exo
		unix2dos monitor-dos.exo

monitor.exo:	monitor.bin
		$(BUILD)/bin/bin2exo 0 monitor.bin monitor.exo

monitor.bin:	$(SRC)
		$(BUILD)/bin/lcc -A -Wo-rom -Wl-rd -Wl0xC03F0000 \
		  -Wl-m -Wlmonitor.map -o monitor.bin $(SRC)

clean:
		rm -f *~
		rm -f monitor.map monitor.bin
		rm -f monitor.exo monitor-dos.exo
