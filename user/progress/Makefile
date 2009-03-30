NAME=progress
BUILD=../../build
DISK=$(BUILD)/disk.img
DISKMOUNT=../../disk
BIN=$(BUILD)/user_$(NAME).bin
DEP=$(BUILD)/user_$(NAME).dep
LIBC=../../libc
LDCONF=$(LIBC)/ld.conf

CC = gcc
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs -I$(LIBC)/esc/h -I$(LIBC)/std/h -I../../lib/h \
	-Wl,-T,$(LDCONF) $(CDEFFLAGS) 
CSRC=$(wildcard *.c)

LIBCA=$(BUILD)/libc.a
START=$(BUILD)/libc_startup.o
COBJ=$(patsubst %.c,$(BUILD)/user_$(NAME)_%.o,$(CSRC))

.PHONY: all clean

all:	$(BIN)

$(BIN):	$(LDCONF) $(COBJ) $(START) $(LIBCA)
		@echo "===== COMPILING & LINKING =====";
		$(CC) $(CFLAGS) -o $(BIN) $(START) $(COBJ) $(LIBCA);
		@echo "===== COPYING ON DISK =====";
		@make -C ../../ mounthdd
		sudo cp $(BIN) $(DISKMOUNT)/bin/$(NAME)
		@make -C ../../ umounthdd

$(BUILD)/user_$(NAME)_%.o:		%.c
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(DEP):	$(CSRC)
		@echo "===== GENERATING DEPENDENCIES =====";
		$(CC) $(CFLAGS) -MM $(CSRC) > $(DEP);
		@# prefix all files with the build-path (otherwise make wouldn't find them)
		sed --in-place -e "s/\([a-zA-Z_]*\).o:/$(subst /,\/,$(BUILD)\/user_$(NAME)_)\1.o:/g" $(DEP);

-include $(DEP)

clean:
		rm -f $(BIN) $(COBJ) $(DEP)
