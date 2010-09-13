# general
ROOT = ../..
BUILDK = $(BUILD)/kernel/src
BIN = $(BUILD)/kernel.bin
LDCONF = ../ld.conf
LIB = $(ROOT)/lib/basic
SUBDIRS = . $(filter-out Makefile $(wildcard *.*),$(wildcard *))
LSUBDIRS = $(filter-out $(wildcard $(LIB)/*.*),$(wildcard $(LIB)/*))
BUILDDIRS = $(addprefix $(BUILDK)/,$(SUBDIRS)) $(patsubst $(LIB)/%,$(BUILDK)/lib/%,$(LSUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -mindepth 0 -maxdepth 1 -name "*.d")

CFLAGS = -ffreestanding -nostdlib -nostartfiles -nodefaultlibs $(CDEFFLAGS) -D IN_KERNEL=1
# for profiling:
# -finstrument-functions -finstrument-functions-exclude-file-list=ksymbols.c,$(LIB)/src/profile.c

# sources
ASRC = $(shell find $(SUBDIRS) -mindepth 0 -maxdepth 1 -name "*.s")
CSRC = $(shell find $(SUBDIRS) -mindepth 0 -maxdepth 1 -name "*.c")
LSRC = $(shell find $(LIB) -name "*.c")

# objects
AOBJ = $(patsubst %.s,$(BUILDK)/%.s.o,$(ASRC))
COBJ = $(patsubst %.c,$(BUILDK)/%.o,$(CSRC))
LOBJ = $(patsubst $(LIB)/%.c,$(BUILDK)/lib/%.o,$(LSRC))

SYMBOLS = $(BUILD)/kernel_symbols.txt
SYMBOLSCRIPT = $(ROOT)/tools/gensymbols.sh
SYMBOLSRC = ksymbols.c
SYMBOLOBJ = $(patsubst %.c,$(BUILDK)/%.o,$(SYMBOLSRC))

.PHONY: all pre clean

all:	$(BIN)

$(BIN):	$(BUILDDIRS) $(SYMBOLS) $(DRIVERS) $(LDCONF) $(AOBJ) $(COBJ) $(LOBJ)
		@echo "	" LINKING $(BIN)
		@$(CC) $(CFLAGS) -Wl,-T,$(LDCONF) -o $(BIN) $(AOBJ) $(COBJ) $(LOBJ) -lgcc;
		@# Note that we link 2 additional times because otherwise the symbol-table is incorrect
		@# since it's size is 0 at the first time. In the second pass the size is correct
		@# and therefore the symbol-table will be generated correctly in the third pass :)
		@echo "	" BUILDING SYMBOL-FILE
		@$(SYMBOLSCRIPT) $(BIN) > $(SYMBOLS)
		@echo "	" CC $(SYMBOLSRC)
		@$(CC) $(CFLAGS) -o $(SYMBOLOBJ) -c $(SYMBOLSRC)
		@echo "	" LINKING $(BIN)
		@$(CC) $(CFLAGS) -Wl,-T,$(LDCONF) -o $(BIN) $(AOBJ) $(COBJ) $(LOBJ) -lgcc;
		@echo "	" BUILDING SYMBOL-FILE
		@$(SYMBOLSCRIPT) $(BIN) > $(SYMBOLS)
		@echo "	" CC $(SYMBOLSRC)
		@$(CC) $(CFLAGS) -o $(SYMBOLOBJ) -c $(SYMBOLSRC)
		@echo "	" LINKING $(BIN)
		@$(CC) $(CFLAGS) -Wl,-T,$(LDCONF) -o $(BIN) $(AOBJ) $(COBJ) $(LOBJ) -lgcc;

$(SYMBOLS):
		@if [ ! -f $(SYMBOLS) ]; then echo "" > $(SYMBOLS); fi
		
$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDK)/%.s.o: %.s
		@echo "	" AS $<
		@$(AS) $(ASFLAGS) -o $@ $<;

$(BUILDK)/%.o:		%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MD

$(BUILDK)/lib/%.o:	$(LIB)/%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MD

-include $(DEPS)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(DEPS) $(BIN) $(COBJ) $(AOBJ) $(LOBJ) $(SYMBOLS) $(SYMBOLOBJ)
