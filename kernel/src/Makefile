# general
ROOT = ../..
BUILDK = $(BUILD)/kernel/src
BIN = $(BUILD)/kernel.bin
LDCONF = ../ld.conf
LIB = $(ROOT)/lib/basic
GCCLIBDIR = $(ROOT)/lib/gcc
SUBDIRS = . $(filter-out Makefile $(wildcard *.*),$(wildcard *))
LSUBDIRS = $(filter-out $(wildcard $(LIB)/src/*.*),$(wildcard $(LIB)/src/*))
BUILDDIRS = $(addprefix $(BUILDK)/,$(SUBDIRS)) $(patsubst $(LIB)/%,$(BUILDK)/lib/%,$(LSUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -mindepth 0 -maxdepth 1 -name "*.d")

CC = gcc
CFLAGS =  -ffreestanding -nostdlib -nostartfiles -nodefaultlibs $(CDEFFLAGS) \
	-D IN_KERNEL=1 -I../h -I$(LIB)/h
# for profiling:
# -finstrument-functions -finstrument-functions-exclude-file-list=ksymbols.c,$(LIB)/src/profile.c

# sources
ASRC = ../kernel.asm
CSRC = $(shell find $(SUBDIRS) -mindepth 0 -maxdepth 1 -name "*.c")
LSRC = $(shell find $(LIB)/src -name "*.c")

# objects
AOBJ = $(BUILDK)/kernela.o
COBJ = $(patsubst %.c,$(BUILDK)/%.o,$(CSRC))
LOBJ = $(patsubst $(LIB)/%.c,$(BUILDK)/lib/%.o,$(LSRC))
# for long long arithmetic
GCCLIB = $(GCCLIBDIR)/_divdi3.object \
	$(GCCLIBDIR)/_moddi3.object \
	$(GCCLIBDIR)/_udivdi3.object \
	$(GCCLIBDIR)/_umoddi3.object

SYMBOLS = $(BUILD)/kernel_symbols.txt
SYMBOLSCRIPT = $(ROOT)/tools/gensymbols.sh
SYMBOLSRC = ksymbols.c
SYMBOLOBJ = $(patsubst %.c,$(BUILDK)/%.o,$(SYMBOLSRC))

.PHONY: all pre clean

all:	$(BIN)

$(BIN):	$(BUILDDIRS) $(SYMBOLS) $(DRIVERS) $(LDCONF) $(AOBJ) $(COBJ) $(LOBJ) $(GCCLIB)
		@echo "	" LINKING $(BIN)
		@ld -T $(LDCONF) -o $(BIN) $(AOBJ) $(COBJ) $(LOBJ) $(GCCLIB);
		@# Note that we link 2 additional times because otherwise the symbol-table is incorrect
		@# since it's size is 0 at the first time. In the second pass the size is correct
		@# and therefore the symbol-table will be generated correctly in the third pass :)
		@echo "	" BUILDING SYMBOL-FILE
		@$(SYMBOLSCRIPT) $(BIN) > $(SYMBOLS)
		@echo "	" CC $(SYMBOLSRC)
		@$(CC) $(CFLAGS) -o $(SYMBOLOBJ) -c $(SYMBOLSRC)
		@echo "	" LINKING $(BIN)
		@ld -T $(LDCONF) -o $(BIN) $(AOBJ) $(COBJ) $(LOBJ) $(GCCLIB);
		@echo "	" BUILDING SYMBOL-FILE
		@$(SYMBOLSCRIPT) $(BIN) > $(SYMBOLS)
		@echo "	" CC $(SYMBOLSRC)
		@$(CC) $(CFLAGS) -o $(SYMBOLOBJ) -c $(SYMBOLSRC)
		@echo "	" LINKING $(BIN)
		@ld -T $(LDCONF) -o $(BIN) $(AOBJ) $(COBJ) $(LOBJ) $(GCCLIB);
		@echo "	" COPYING BINARY
		$(ROOT)/tools/disk.sh copy $(BIN) /boot

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

$(BUILDK)/lib/%.o:	$(LIB)/%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MMD

-include $(DEPS)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(DEPS) $(BIN) $(COBJ) $(AOBJ) $(LOBJ) $(SYMBOLS) $(SYMBOLOBJ)
