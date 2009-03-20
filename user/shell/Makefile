NAME=shell
BUILD=../../build
DISK=$(BUILD)/disk.img
DISKMOUNT=../../disk
BIN=$(BUILD)/user_$(NAME).bin
DUMP=$(BUILD)/user_$(NAME).dump
DEP=$(BUILD)/user_$(NAME).dep
LIBC=../../libc
LDCONF=$(LIBC)/ld.conf

CC = gcc
CFLAGS = -g -nostdlib -nostartfiles -nodefaultlibs -I$(LIBC)/h -I../../lib/h -I$(LIBC)/test \
	-Wl,-T,$(LDCONF) $(CWFLAGS) 
CSRC=$(wildcard *.c)
CCMDSRC=$(wildcard cmd/*.c)

LIBCA=$(BUILD)/libc.a
START=$(BUILD)/libc_startup.o
COBJ=$(patsubst %.c,$(BUILD)/user_$(NAME)_%.o,$(CSRC))
CCMDOBJ=$(patsubst cmd/%.c,$(BUILD)/user_$(NAME)_cmd_%.o,$(CCMDSRC))

.PHONY: all clean

all:	$(BIN) $(DUMP)

$(BIN):	$(LDCONF) $(COBJ) $(CCMDOBJ) $(START) $(LIBCA)
		@echo "===== COMPILING & LINKING =====";
		$(CC) $(CFLAGS) -o $(BIN) $(START) $(COBJ) $(CCMDOBJ) $(LIBCA);
		@echo "===== COPYING ON DISK =====";
		@make -C ../../ mounthdd
		sudo cp $(BIN) $(DISKMOUNT)/apps/$(NAME)
		@make -C ../../ umounthdd

$(BUILD)/user_$(NAME)_%.o:		%.c
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD)/user_$(NAME)_cmd_%.o:		cmd/%.c
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(DUMP):	$(BIN)
		@echo "===== GENERATING DUMP =====";
		$(BUILD)/dump $(BIN) $(DUMP)

$(DEP):	$(CSRC) $(CCMDSRC)
		@echo "===== GENERATING DEPENDENCIES =====";
		$(CC) $(CFLAGS) -MM $(CSRC) $(CCMDSRC) > $(DEP);
		@# prefix all files with the build-path (otherwise make wouldn't find them)
		sed --in-place -e "s/\([a-zA-Z_]*\).o:/$(subst /,\/,$(BUILD)\/user_$(NAME)_)\1.o:/g" $(DEP);
		@for i in $(patsubst cmd/%.c,%,$(wildcard cmd/*.c)) ; do \
			sed --in-place -e "s/$(subst /,\/,$(BUILD)/user_$(NAME)_)$$i.o:/$(subst /,\/,$(BUILD)/user_$(NAME)_cmd_)$$i.o:/g" $(DEP); \
		done;

-include $(DEP)

clean:
		rm -f $(BIN) $(DUMP) $(DEP) $(COBJ) $(CCMDOBJ)
