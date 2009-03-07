NAME=keyboard
BUILD=../../build
LIBDIR=../../lib
DISK=$(BUILD)/disk.img
DISKMOUNT=../../disk
BIN=$(BUILD)/service_$(NAME).bin
DUMP=$(BUILD)/service_$(NAME).dump
LIBC=../../libc
LDCONF=$(LIBC)/ld.conf

CC = gcc
CFLAGS = -g -nostdlib -nostartfiles -nodefaultlibs -I$(LIBC)/h -I../../lib/h -I$(LIBC)/test \
	-Wl,-T,$(LDCONF) $(CWFLAGS) 
CSRC=$(wildcard *.c)
LSRC=

LIBCA=$(BUILD)/libc.a
START=$(BUILD)/libc_startup.o
LIBCOBJS=$(filter-out $(START),$(wildcard $(BUILD)/libc_*.o))
LOBJ=$(patsubst $(LIBDIR)/src/%.c,$(BUILD)/service_lib_%.o,$(LSRC))

.PHONY: all clean

all:	$(BIN) $(DUMP)

$(BIN):	$(LDCONF) $(LOBJ) $(CSRC) $(START) $(LIBCA)
		@echo "===== COMPILING & LINKING =====";
		$(CC) $(CFLAGS) -o $(BIN) $(START) $(CSRC) $(LOBJ) $(LIBCA);
		@echo "===== COPYING BINARY =====";
		sudo mount -o loop $(DISK) $(DISKMOUNT);
		cp $(BIN) $(DISKMOUNT);
		sudo umount $(DISKMOUNT);

$(BUILD)/service_lib_%.o:	$(LIBDIR)/src/%.c
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(DUMP):	$(BIN)
		@echo "===== GENERATING DUMP =====";
		$(BUILD)/dump $(BIN) $(DUMP)

clean:
		rm -f $(BIN) $(DUMP) $(LOBJ)
