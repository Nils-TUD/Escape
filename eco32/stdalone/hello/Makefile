#
# Makefile for "hello", a program for testing the boot loader
#

BUILD = ../../build

SRC = hello.s
OBJ = hello.o
BIN = hello.bin
MAP = hello.map

.PHONY:		all install run clean

all:		$(BIN)

install:	$(BIN)
		mkdir -p $(BUILD)/stdalone
		cp $(BIN) $(BUILD)/stdalone
		cp $(MAP) $(BUILD)/stdalone

run:		$(BIN)
		$(BUILD)/bin/sim -i -t 1 -l $(BIN)

$(BIN):		$(OBJ)
		$(BUILD)/bin/ld -h -rc 0xC0000000 \
		  -m $(MAP) -o $(BIN) $(OBJ)

$(OBJ):		$(SRC)
		$(BUILD)/bin/as -o $(OBJ) $(SRC)

clean:
		rm -f *~ $(OBJ) $(BIN) $(MAP)
