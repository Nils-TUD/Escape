# general
BUILD=../../build
DISK=$(BUILD)/disk.img
DISKMOUNT=../../disk
BINNAME=kernel_test.bin
BIN=$(BUILD)/$(BINNAME)
LDCONF=../ld.conf
DEP=$(BUILD)/test.dep

CC = gcc
CFLAGS = -g -fno-inline -ffreestanding -nostdlib -nostartfiles -nodefaultlibs -Wall -ansi \
				 -Wextra -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-prototypes \
				 -Wmissing-declarations -Wredundant-decls -Wnested-externs -Winline -Wno-long-long \
				 -Wstrict-prototypes

# sources
TSRC=$(wildcard *.c)
KSRC=$(filter-out ../src/kernel.c,$(wildcard ../src/*.c))

# objects
AOBJ=$(BUILD)/kernel_asm.o
AOBJEND=$(BUILD)/end.o
KOBJ=$(patsubst ../src/%.c,$(BUILD)/%.o,$(KSRC))
TOBJ=$(patsubst %.c,$(BUILD)/test_%.o,$(TSRC))

SYMBOLS=$(BUILD)/tsymbols.txt
SYMBOLSCRIPT=../../tools/gensymbols.sh
SYMBOLSRC=../src/ksymbols.c

.PHONY: all pre clean

all:	$(BIN)

$(BIN):	$(LDCONF) $(AOBJ) $(KOBJ) $(TOBJ)
		@echo "===== LINKING =====";
		ld -T $(LDCONF) -o $(BIN) $(AOBJ) $(KOBJ) $(TOBJ) $(AOBJEND);
		@# Note that we link 2 additional times because otherwise the symbol-table is incorrect
		@# since it's size is 0 at the first time. In the second part the size is correct
		@# and therefore the symbol-table will be generated correctly :)
		@echo "===== BUILDING SYMBOL-FILE =====";
		$(SYMBOLSCRIPT) $(BIN) > $(SYMBOLS)
		@echo "===== COMPILING SYMBOL-C-FILE =====";
		$(CC) $(CFLAGS) -o $(patsubst ../src/%.c,$(BUILD)/t%.o,$(SYMBOLSRC)) -c $(SYMBOLSRC)
		@echo "===== LINKING AGAIN =====";
		ld -T $(LDCONF) -o $(BIN) $(AOBJ) $(KOBJ) $(TOBJ) $(AOBJEND);
		@echo "===== BUILDING SYMBOL-FILE =====";
		$(SYMBOLSCRIPT) $(BIN) > $(SYMBOLS)
		@echo "===== COMPILING SYMBOL-C-FILE =====";
		$(CC) $(CFLAGS) -o $(patsubst ../src/%.c,$(BUILD)/t%.o,$(SYMBOLSRC)) -c $(SYMBOLSRC)
		@echo "===== LINKING AGAIN =====";
		ld -T $(LDCONF) -o $(BIN) $(AOBJ) $(KOBJ) $(TOBJ) $(AOBJEND);
		@echo "===== COPYING BINARY =====";
		sudo mount -o loop $(DISK) $(DISKMOUNT) || true;
		cp $(BIN) $(DISKMOUNT);
		sudo umount $(DISKMOUNT);

$(BUILD)/test_%.o:		%.c
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(DEP):	$(TSRC)
		@echo "===== GENERATING DEPENDENCIES =====";
		$(CC) $(CFLAGS) -MM $(TSRC) > $(DEP);
		@# prefix all files with the build-path (otherwise make wouldn't find them)
		sed --in-place -e "s/\([a-zA-Z_]*\).o:/$(subst /,\/,$(BUILD))\/\1.o:/g" $(DEP);

-include $(DEP)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(DEP) $(BIN) $(TOBJ);
