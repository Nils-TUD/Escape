NAME=initloader
ROOT=../../..
BUILD=$(ROOT)/build
DISK=$(BUILD)/disk.img
DISKMOUNT=$(ROOT)/disk
BIN=$(BUILD)/user_$(NAME).bin
DUMP=$(BUILD)/user_$(NAME).dump
LIBC=$(ROOT)/libc
LDCONF=$(LIBC)/ld.conf

CC = gcc
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs -Wl,-T,$(LDCONF) $(CDEFFLAGS) 

ASRC=$(wildcard *.s)
AOBJ=$(patsubst %.s,$(BUILD)/user_$(NAME)_%.o,$(ASRC))

.PHONY: all clean

all:	$(BIN) $(DUMP)

$(BIN):	$(LDCONF) $(AOBJ)
		@echo "===== COMPILING & LINKING =====";
		$(CC) $(CFLAGS) -o $(BIN) $(AOBJ);

$(BUILD)/user_$(NAME)_%.o: %.s
		@echo "===== ASSEMBLING $< =====";
		nasm $(ASMFLAGS) -o $@ $<

$(DUMP):	$(BIN)
		@echo "===== GENERATING DUMP =====";
		$(BUILD)/dump $(BIN) > $(DUMP)

clean:
		rm -f $(BIN) $(DUMP) $(AOBJ)
