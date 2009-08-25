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
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs -Wl,-T,$(LDCONF) $(CDEFFLAGS) \
	-I$(ROOT)/lib/h -I$(LIBC)/include

ASRC=$(wildcard *.s)
AOBJ=$(patsubst %.s,$(BUILD)/user_$(NAME)_%.o,$(ASRC))

CSRC=$(wildcard *.c)
COBJ=$(patsubst %.c,$(BUILD)/user_$(NAME)_c%.o,$(CSRC))
LIBCA=$(BUILD)/libc.a

.PHONY: all clean

all:	$(BIN) $(DUMP)

$(BIN):	$(LDCONF) $(AOBJ) $(COBJ)
		@echo "	" LINKING $(BIN)
		@$(CC) $(CFLAGS) -o $(BIN) $(AOBJ) $(COBJ) $(LIBCA);

$(BUILD)/user_$(NAME)_%.o: %.s
		@echo "	" AS $<
		@nasm $(ASMFLAGS) -o $@ $<

$(BUILD)/user_$(NAME)_c%.o: %.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $<

$(DUMP):	$(BIN)
		@echo "	" GENERATING DUMP
		@$(BUILD)/dump $(BIN) > $(DUMP)

clean:
		rm -f $(BIN) $(DUMP) $(AOBJ) $(COBJ)
