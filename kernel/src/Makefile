# general
ROOT=../..
BUILD=$(ROOT)/build
BUILDK=$(BUILD)/kernel/src
BIN=$(BUILD)/kernel.bin
LDCONF=../ld.conf
LIB=$(ROOT)/lib
SUBDIRS=. $(filter-out Makefile $(wildcard *.*),$(wildcard *))
BUILDDIRS=$(addprefix $(BUILDK)/,$(SUBDIRS))
DEPS=$(shell find $(BUILDDIRS) -mindepth 0 -maxdepth 1 -name "*.d")

CC = gcc
CFLAGS = -ffreestanding -nostdlib -nostartfiles -nodefaultlibs $(CDEFFLAGS) \
	-D IN_KERNEL=1 -I../h -I$(LIB)/h

# sources
ASRC=../kernel.asm
CSRC=$(shell find $(SUBDIRS) -mindepth 0 -maxdepth 1 -name "*.c")
LSRC=$(wildcard $(LIB)/src/*.c)

# objects
AOBJ=$(BUILDK)/kernela.o
COBJ=$(patsubst %.c,$(BUILDK)/%.o,$(CSRC))
LOBJ=$(patsubst $(LIB)/src/%.c,$(BUILDK)/lib_%.o,$(LSRC))

SYMBOLS=$(BUILD)/kernel_symbols.txt
SYMBOLSCRIPT=$(ROOT)/tools/gensymbols.sh
SYMBOLSRC=ksymbols.c
SYMBOLOBJ=$(patsubst %.c,$(BUILDK)/%.o,$(SYMBOLSRC))

.PHONY: all pre clean

all:	$(BIN)

$(BIN):	$(BUILDDIRS) $(SYMBOLS) $(SERVICES) $(LDCONF) $(AOBJ) $(COBJ) $(LOBJ)
		@echo "	" LINKING $(BIN)
		@ld -T $(LDCONF) -o $(BIN) $(AOBJ) $(COBJ) $(LOBJ);
		@# Note that we link 2 additional times because otherwise the symbol-table is incorrect
		@# since it's size is 0 at the first time. In the second pass the size is correct
		@# and therefore the symbol-table will be generated correctly in the third pass :)
		@echo "	" BUILDING SYMBOL-FILE
		@$(SYMBOLSCRIPT) $(BIN) > $(SYMBOLS)
		@echo "	" CC $(SYMBOLSRC)
		@$(CC) $(CFLAGS) -o $(SYMBOLOBJ) -c $(SYMBOLSRC)
		@echo "	" LINKING $(BIN)
		@ld -T $(LDCONF) -o $(BIN) $(AOBJ) $(COBJ) $(LOBJ);
		@echo "	" BUILDING SYMBOL-FILE
		@$(SYMBOLSCRIPT) $(BIN) > $(SYMBOLS)
		@echo "	" CC $(SYMBOLSRC)
		@$(CC) $(CFLAGS) -o $(SYMBOLOBJ) -c $(SYMBOLSRC)
		@echo "	" LINKING $(BIN)
		@ld -T $(LDCONF) -o $(BIN) $(AOBJ) $(COBJ) $(LOBJ);
		@echo "	" COPYING BINARY
		$(ROOT)/tools/cp2disk.sh $(BIN) /boot

$(SYMBOLS):
		@if [ ! -f $(SYMBOLS) ]; then echo "" > $(SYMBOLS); fi
		
$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(AOBJ):	$(ASRC)
		@echo "	" AS $(ASRC)
		@nasm $(ASMFLAGS) -o $(AOBJ) $(ASRC);

$(BUILDK)/%.o:		%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MMD

$(BUILDK)/lib_%.o:	$(LIB)/src/%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MMD

-include $(DEPS)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(DEPS) $(BIN) $(COBJ) $(AOBJ) $(LOBJ) $(SYMBOLS) $(SYMBOLOBJ)
