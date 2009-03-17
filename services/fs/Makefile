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
CEXT2SRC=$(wildcard ext2/*.c)

LIBCA=$(BUILD)/libc.a
START=$(BUILD)/libc_startup.o
COBJ=$(patsubst %.c,$(BUILD)/service_$(NAME)_%.o,$(CSRC))
CEXT2OBJ=$(patsubst ext2/%.c,$(BUILD)/service_$(NAME)_ext2_%.o,$(CEXT2SRC))

.PHONY: all clean

all:	$(BIN) $(DUMP)

$(BIN):	$(LDCONF) $(COBJ) $(CEXT2OBJ) $(START) $(LIBCA)
		@echo "===== COMPILING & LINKING =====";
		$(CC) $(CFLAGS) -o $(BIN) $(START) $(COBJ) $(CEXT2OBJ) $(LIBCA);

$(BUILD)/service_$(NAME)_%.o:		%.c
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD)/service_$(NAME)_ext2_%.o:		ext2/%.c
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(DUMP):	$(BIN)
		@echo "===== GENERATING DUMP =====";
		$(BUILD)/dump $(BIN) $(DUMP)

$(DEP):	$(CEXT2SRC) $(CSRC)
		@echo "===== GENERATING DEPENDENCIES =====";
		$(CC) $(CFLAGS) -MM $(CEXT2SRC) $(CSRC) > $(DEP);
		@# prefix all files with the build-path (otherwise make wouldn't find them)
		sed --in-place -e "s/\([a-zA-Z_]*\).o:/$(subst /,\/,$(BUILD)\/service_$(NAME)_)\1.o:/g" $(DEP);
		@for i in $(patsubst ext2/%.c,%,$(wildcard ext2/*.c)) ; do \
			sed --in-place -e "s/$(subst /,\/,$(BUILD)/service_$(NAME)_)$$i.o:/$(subst /,\/,$(BUILD)/service_$(NAME)_ext2_)$$i.o:/g" $(DEP); \
		done;

-include $(DEP)

clean:
		rm -f $(BIN) $(DUMP) $(COBJ) $(CEXT2OBJ) $(DEP)
