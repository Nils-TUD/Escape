# general
BUILD=../../build
DISK=$(BUILD)/disk.img
DISKMOUNT=../../disk
BINNAME=kernel_test.bin
BIN=$(BUILD)/$(BINNAME)
LDCONF=../ld.conf
DEP=$(BUILD)/test.dep
LIB=../../lib

CC = gcc
CFLAGS = -fno-inline -ffreestanding -nostdlib -nostartfiles -nodefaultlibs $(CDEFFLAGS) \
	-D TESTING=1 -D IN_KERNEL=1 -I../h -I$(LIB)/h

# sources
TSRC=$(wildcard *.c)
KSRC=$(filter-out ../src/kernel.c,$(wildcard ../src/*.c))
LSRC=$(wildcard $(LIB)/src/*.c)

# objects
AOBJ=$(BUILD)/kernel_asm.o
AOBJEND=$(BUILD)/end.o
KOBJ=$(patsubst ../src/%.c,$(BUILD)/testk_%.o,$(KSRC))
TOBJ=$(patsubst %.c,$(BUILD)/test_%.o,$(TSRC))
LOBJ=$(patsubst $(LIB)/src/%.c,$(BUILD)/test_lib_%.o,$(LSRC))

SYMBOLS=$(BUILD)/tsymbols.txt
SYMBOLSCRIPT=../../tools/gensymbols.sh
SYMBOLSRC=../src/ksymbols.c
SYMBOLOBJ=$(patsubst ../src/%.c,$(BUILD)/t%.o,$(SYMBOLSRC))

.PHONY: all pre clean

all: $(BIN)

$(BIN):	$(LDCONF) $(AOBJ) $(KOBJ) $(TOBJ) $(LOBJ)
		@echo "===== LINKING =====";
		ld -T $(LDCONF) -o $(BIN) $(AOBJ) $(KOBJ) $(TOBJ) $(LOBJ) $(AOBJEND);
		@# Note that we link 2 additional times because otherwise the symbol-table is incorrect
		@# since it's size is 0 at the first time. In the second part the size is correct
		@# and therefore the symbol-table will be generated correctly :)
		@echo "===== BUILDING SYMBOL-FILE =====";
		$(SYMBOLSCRIPT) $(BIN) > $(SYMBOLS)
		@echo "===== COMPILING SYMBOL-C-FILE =====";
		$(CC) $(CFLAGS) -o $(SYMBOLOBJ) -c $(SYMBOLSRC)
		@echo "===== LINKING AGAIN =====";
		ld -T $(LDCONF) -o $(BIN) $(AOBJ) $(KOBJ) $(TOBJ) $(LOBJ) $(AOBJEND);
		@echo "===== BUILDING SYMBOL-FILE =====";
		$(SYMBOLSCRIPT) $(BIN) > $(SYMBOLS)
		@echo "===== COMPILING SYMBOL-C-FILE =====";
		$(CC) $(CFLAGS) -o $(SYMBOLOBJ) -c $(SYMBOLSRC)
		@echo "===== LINKING AGAIN =====";
		ld -T $(LDCONF) -o $(BIN) $(AOBJ) $(KOBJ) $(TOBJ) $(LOBJ) $(AOBJEND);
		@echo "===== COPYING BINARY =====";
		@make -C ../../ mounthdd
		sudo cp $(BIN) $(DISKMOUNT)/boot
		@make -C ../../ umounthdd

$(BUILD)/testk_%.o:	../src/%.c
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD)/test_%.o:		%.c
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD)/test_lib_%.o:	$(LIB)/src/%.c
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(DEP):	$(TSRC) $(KSRC) $(LSRC)
		@echo "===== GENERATING DEPENDENCIES =====";
		if [ ! -f $(SYMBOLS) ]; then echo "" > $(SYMBOLS); fi
		$(CC) $(CFLAGS) -MM $(TSRC) $(KSRC) $(LSRC) > $(DEP);
		@# prefix all files with the build-path (otherwise make wouldn't find them)
		sed --in-place -e "s/\([a-zA-Z_]*\).o:/$(subst /,\/,$(BUILD))\/test_\1.o:/g" $(DEP);

-include $(DEP)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(DEP) $(BIN) $(TOBJ) $(KOBJ) $(LOBJ) $(SYMBOLS) $(SYMBOLOBJ);
