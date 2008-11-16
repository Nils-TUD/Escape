# general
BUILD=../build
DISK=$(BUILD)/disk.img
DISKMOUNT=../disk
BINNAME=kernel.bin
BIN=$(BUILD)/$(BINNAME)
LDCONF=ld.conf
DEP=$(BUILD)/kernel.dep

CC = gcc
CFLAGS = -g -fno-inline -ffreestanding -nostdlib -nostartfiles -nodefaultlibs $(CWFLAGS)

# sources
ASRC=kernel.asm
ASRCEND=end.asm
CSRC=$(wildcard src/*.c)

# objects
AOBJ=$(BUILD)/kernel_asm.o
AOBJEND=$(BUILD)/end.o
COBJ=$(patsubst src/%.c,$(BUILD)/%.o,$(CSRC))

SYMBOLS=$(BUILD)/ksymbols.txt
SYMBOLSCRIPT=../tools/gensymbols.sh
SYMBOLSRC=src/ksymbols.c

.PHONY: all pre clean

all:	pre $(BIN)

# ensure that the symbols exist
pre:
		if [ ! -f $(SYMBOLS) ]; then echo "" > $(SYMBOLS); fi

$(BIN):	$(LDCONF) $(AOBJ) $(COBJ)
		@echo "===== LINKING =====";
		ld -T $(LDCONF) -o $(BIN) $(AOBJ) $(COBJ) $(AOBJEND);
		@# Note that we link 2 additional times because otherwise the symbol-table is incorrect
		@# since it's size is 0 at the first time. In the second part the size is correct
		@# and therefore the symbol-table will be generated correctly :)
		@echo "===== BUILDING SYMBOL-FILE =====";
		$(SYMBOLSCRIPT) $(BIN) > $(SYMBOLS)
		@echo "===== COMPILING SYMBOL-C-FILE =====";
		$(CC) $(CFLAGS) -o $(patsubst src/%.c,$(BUILD)/%.o,$(SYMBOLSRC)) -c $(SYMBOLSRC)
		@echo "===== LINKING AGAIN =====";
		ld -T $(LDCONF) -o $(BIN) $(AOBJ) $(COBJ) $(AOBJEND);
		@echo "===== BUILDING SYMBOL-FILE =====";
		$(SYMBOLSCRIPT) $(BIN) > $(SYMBOLS)
		@echo "===== COMPILING SYMBOL-C-FILE =====";
		$(CC) $(CFLAGS) -o $(patsubst src/%.c,$(BUILD)/%.o,$(SYMBOLSRC)) -c $(SYMBOLSRC)
		@echo "===== LINKING AGAIN =====";
		ld -T $(LDCONF) -o $(BIN) $(AOBJ) $(COBJ) $(AOBJEND);
		@echo "===== COPYING BINARY =====";
		sudo mount -o loop $(DISK) $(DISKMOUNT) || true;
		cp $(BIN) $(DISKMOUNT);
		sudo umount $(DISKMOUNT);

$(AOBJ):	$(ASRC) $(ASRCEND)
		@echo "===== ASSEMBLING =====";
		nasm -g -f elf -o $(AOBJ) $(ASRC);
		nasm -g -f elf -o $(AOBJEND) $(ASRCEND);

$(BUILD)/%.o:		src/%.c
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(DEP):	$(CSRC)
		@echo "===== GENERATING DEPENDENCIES =====";
		$(CC) $(CFLAGS) -MM $(CSRC) > $(DEP);
		@# prefix all files with the build-path (otherwise make wouldn't find them)
		sed --in-place -e "s/\([a-zA-Z_]*\).o:/$(subst /,\/,$(BUILD))\/\1.o:/g" $(DEP);

-include $(DEP)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(DEP) $(BIN) $(COBJ) $(AOBJ) $(AOBJEND) $(SYMBOLS) \
			$(BUILD)/tools_gensymbols.c $(BUILD)/tools_gensymbols.o;
