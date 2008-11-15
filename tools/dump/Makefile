#
# Makefile for dump utility
#

# adjust name
NAME = dump
BUILD = ../../build

CC = gcc
CFLAGS = -Wall -pedantic -ansi

SRC = dump.c
BIN = $(BUILD)/$(NAME)

.PHONY:		all install clean

all:		$(BIN)

install:	$(BIN)

$(BIN):		$(SRC)
		@echo "===== BUILDING $(BIN) =====";
		$(CC) $(CFLAGS) -o $(BIN) $(SRC)

clean:
		rm -f *~ $(BIN)
