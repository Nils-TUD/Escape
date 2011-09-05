#
# Makefile for "dskchk", a program to check the hard disk interface
#

BUILD = ../../build

SRC = start.s main.c end.s
BIN = dskchk.bin
MAP = dskchk.map

.PHONY:		all install run clean

all:		$(BIN)

install:	$(BIN)
		mkdir -p $(BUILD)/stdalone
		cp $(BIN) $(BUILD)/stdalone
		cp $(MAP) $(BUILD)/stdalone

run:		$(BIN)
		$(BUILD)/bin/sim -i -t 1 -l $(BIN) -d $(BUILD)/run/disk.img

$(BIN):		$(SRC)
		$(BUILD)/bin/lcc -A -Wo-kernel \
		  -Wl-m -Wl$(MAP) -o $(BIN) $(SRC)

clean:
		rm -f *~ $(BIN) $(MAP)
