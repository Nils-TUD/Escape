NAME=shell
BUILD=../../build
DISK=$(BUILD)/disk.img
DISKMOUNT=../../disk
BIN=$(BUILD)/user_$(NAME).bin
DUMP=$(BUILD)/user_$(NAME).dump
LIBC=../../libc
LDCONF=$(LIBC)/ld.conf
LIBCA=$(BUILD)/libc.a
START=$(BUILD)/libc_startup.o
LIBCOBJS=$(filter-out $(START),$(wildcard $(BUILD)/libc_*.o))

CC = gcc
CFLAGS = -g -nostdlib -nostartfiles -nodefaultlibs -I$(LIBC)/h -I../../lib/h -I$(LIBC)/test \
	-Wl,-T,$(LDCONF) $(CWFLAGS) 
CSRC=$(wildcard *.c) $(wildcard cmd/*.c)

.PHONY: all clean

all:	$(BIN) $(DUMP)

$(BIN):	$(LDCONF) $(CSRC) $(START) $(LIBCA)
		@echo "===== COMPILING & LINKING =====";
		$(CC) $(CFLAGS) -o $(BIN) $(START) $(CSRC) $(LIBCA);
		@echo "===== COPYING BINARY =====";
		sudo mount -o loop $(DISK) $(DISKMOUNT);
		cp $(BIN) $(DISKMOUNT);
		sudo umount $(DISKMOUNT);

$(DUMP):	$(BIN)
		@echo "===== GENERATING DUMP =====";
		$(BUILD)/dump $(BIN) $(DUMP)

clean:
		rm -f $(BIN) $(DUMP)
