NAME=fs
BUILD=../../build
DISK=$(BUILD)/disk.img
DISKMOUNT=../../disk
BIN=$(BUILD)/service_$(NAME).bin
DUMP=$(BUILD)/service_$(NAME).dump
DEP=$(BUILD)/service_$(NAME).dep
LIBC=../../libc
LDCONF=$(LIBC)/ld.conf

CC = gcc
CFLAGS = -g -nostdlib -nostartfiles -nodefaultlibs -I$(LIBC)/h -I../../lib/h \
	-Wl,-T,$(LDCONF) $(CWFLAGS) 
CSRC=$(wildcard *.c)

LIBCA=$(BUILD)/libc.a
START=$(BUILD)/libc_startup.o
COBJ=$(patsubst %.c,$(BUILD)/service_$(NAME)_%.o,$(CSRC))
LIBCOBJS=$(filter-out $(START),$(wildcard $(BUILD)/libc_*.o))

.PHONY: all clean

all:	$(BIN) $(DUMP)

$(BIN):	$(LDCONF) $(COBJ) $(START) $(LIBCA)
		@echo "===== COMPILING & LINKING =====";
		$(CC) $(CFLAGS) -o $(BIN) $(START) $(COBJ) $(LIBCA);

$(BUILD)/service_$(NAME)_%.o:		%.c
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(DUMP):	$(BIN)
		@echo "===== GENERATING DUMP =====";
		$(BUILD)/dump $(BIN) $(DUMP)

$(DEP):	$(CSRC)
		@echo "===== GENERATING DEPENDENCIES =====";
		$(CC) $(CFLAGS) -MM $(CSRC) > $(DEP);
		@# prefix all files with the build-path (otherwise make wouldn't find them)
		sed --in-place -e "s/\([a-zA-Z_]*\).o:/$(subst /,\/,$(BUILD)\/service_$(NAME)_)\1.o:/g" $(DEP);

-include $(DEP)

clean:
		rm -f $(BIN) $(DUMP) $(COBJ) $(DEP)
