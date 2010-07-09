# general
ROOT = ../..
BUILDK = $(BUILD)/kernel/src
BUILDT = $(BUILD)/kernel/test
BIN = $(BUILD)/kernel_test.bin
LDCONF = ../ld.conf
LIB = $(ROOT)/lib/basic
GCCLIBDIR = $(ROOT)/lib/gcc
DEPS = $(wildcard $(BUILDT)/*.d)
SUBDIRS = ../src $(filter-out ../src/Makefile $(wildcard ../src/*.*),$(wildcard ../src/*))
LSUBDIRS = $(filter-out $(wildcard $(LIB)/*.*),$(wildcard $(LIB)/*))
BUILDDIRS = $(BUILDT) $(patsubst $(LIB)/%,$(BUILDT)/lib/%,$(LSUBDIRS))

CC = gcc
CFLAGS = -ffreestanding -nostdlib -nostartfiles -nodefaultlibs $(CDEFFLAGS) \
	-D TESTING=1 -D IN_KERNEL=1 -I$(ROOT)/include

# sources
TSRC = $(wildcard *.c)
KSRC = $(filter-out ../src/kernel.c,$(shell find $(SUBDIRS) -mindepth 0 -maxdepth 1 -name "*.c"))
LSRC = $(shell find $(LIB) -name "*.c")

# objects
AOBJ = $(BUILDK)/kernela.o
TOBJ = $(patsubst %.c,$(BUILDT)/%.o,$(TSRC))
KOBJ = $(patsubst %.c,$(BUILDK)/%.o,$(KSRC))
LOBJ = $(patsubst $(LIB)/%.c,$(BUILDT)/lib/%.o,$(LSRC))
# for long long arithmetic
GCCLIB = $(GCCLIBDIR)/_divdi3.object \
	$(GCCLIBDIR)/_moddi3.object \
	$(GCCLIBDIR)/_udivdi3.object \
	$(GCCLIBDIR)/_umoddi3.object

SYMBOLS = $(BUILD)/kernelt_symbols.txt
SYMBOLSCRIPT = $(ROOT)/tools/gensymbols.sh
SYMBOLSRC = ../src/ksymbols.c
SYMBOLOBJ = $(patsubst ../src/%.c,$(BUILDT)/%.o,$(SYMBOLSRC))

.PHONY: all pre clean

all: $(BIN)

$(BIN):	$(BUILDDIRS) $(LDCONF) $(AOBJ) $(KOBJ) $(TOBJ) $(LOBJ) $(GCCLIB)
		@echo "	" LINKING $(BIN)
		@ld -T $(LDCONF) -o $(BIN) $(AOBJ) $(KOBJ) $(TOBJ) $(LOBJ) $(GCCLIB);
		@# Note that we link 2 additional times because otherwise the symbol-table is incorrect
		@# since it's size is 0 at the first time. In the second pass the size is correct
		@# and therefore the symbol-table will be generated correctly in the third pass :)
		@echo "	" BUILDING SYMBOL-FILE
		@$(SYMBOLSCRIPT) $(BIN) > $(SYMBOLS)
		@echo "	" CC $(SYMBOLSRC)
		@$(CC) $(CFLAGS) -o $(SYMBOLOBJ) -c $(SYMBOLSRC)
		@echo "	" LINKING $(BIN)
		@ld -T $(LDCONF) -o $(BIN) $(AOBJ) $(KOBJ) $(TOBJ) $(LOBJ) $(GCCLIB);
		@echo "	" BUILDING SYMBOL-FILE
		@$(SYMBOLSCRIPT) $(BIN) > $(SYMBOLS)
		@echo "	" CC $(SYMBOLSRC)
		@$(CC) $(CFLAGS) -o $(SYMBOLOBJ) -c $(SYMBOLSRC)
		@echo "	" LINKING $(BIN)
		@ld -T $(LDCONF) -o $(BIN) $(AOBJ) $(KOBJ) $(TOBJ) $(LOBJ) $(GCCLIB);
		@echo "	" COPYING BINARY
		$(ROOT)/tools/disk.sh copy $(BIN) /boot

$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDT)/%.o:		%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MMD

$(BUILDT)/lib/%.o:	$(LIB)/%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MMD

-include $(DEPS)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(DEPS) $(BIN) $(TOBJ) $(LOBJ) $(SYMBOLS) $(SYMBOLOBJ);
