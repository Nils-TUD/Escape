#
# Makefile for "wrtmbr", a program to write a dummy MBR to the disk
#

BUILD = ../../build

SRC = start.s main.c end.s
BIN = wrtmbr.bin
MAP = wrtmbr.map

.PHONY:		all install run clean

all:		$(BIN)

install:	$(BIN)
		mkdir -p $(BUILD)/stdalone
		cp $(BIN) $(BUILD)/stdalone
		cp $(MAP) $(BUILD)/stdalone

run:		$(BIN)
		$(BUILD)/bin/sim -i -t 1 -l $(BIN) -d $(BUILD)/run/disk.img

$(BIN):		$(SRC) mbr/mbr.dump
		$(BUILD)/bin/lcc -A -Wo-kernel \
		  -Wl-m -Wl$(MAP) -o $(BIN) $(SRC)

mbr/mbr.dump:
		$(MAKE) -C mbr

clean:
		$(MAKE) -C mbr clean
		rm -f *~ $(BIN) $(MAP)
