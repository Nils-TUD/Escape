# general
BUILD=../build
DISK=$(BUILD)/disk.img
DISKMOUNT=../disk
BINNAME=kernel.bin
BIN=$(BUILD)/$(BINNAME)
LDCONF=src/ld.conf
OSTITLE=hrniels-OS

CC = gcc
CFLAGS = -g -fno-inline -ffreestanding -nostdlib -nostartfiles -nodefaultlibs -Wall -ansi \
				 -Wextra -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-prototypes \
				 -Wmissing-declarations -Wredundant-decls -Wnested-externs -Winline -Wno-long-long \
				 -Wstrict-prototypes

# sources
ASRC=src/kernel.asm
ASRCEND=src/end.asm
CSRC=$(wildcard src/*.c)

# objects
AOBJ=$(BUILD)/kernel_asm.o
AOBJEND=$(BUILD)/end.o
COBJ=$(patsubst src/%.c,$(BUILD)/%.o,$(CSRC))

SYMBOLS=$(BUILD)/ksymbols.txt
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
		../tools/gensymbols.sh $(BIN) > $(SYMBOLS)
		@echo "===== COMPILING SYMBOL-C-FILE =====";
		$(CC) $(CFLAGS) -o $(patsubst src/%.c,$(BUILD)/%.o,$(SYMBOLSRC)) -c $(SYMBOLSRC)
		@echo "===== LINKING AGAIN =====";
		ld -T $(LDCONF) -o $(BIN) $(AOBJ) $(COBJ) $(AOBJEND);
		@echo "===== BUILDING SYMBOL-FILE =====";
		../tools/gensymbols.sh $(BIN) > $(SYMBOLS)
		@echo "===== COMPILING SYMBOL-C-FILE =====";
		$(CC) $(CFLAGS) -o $(patsubst src/%.c,$(BUILD)/%.o,$(SYMBOLSRC)) -c $(SYMBOLSRC)
		@echo "===== LINKING AGAIN =====";
		ld -T $(LDCONF) -o $(BIN) $(AOBJ) $(COBJ) $(AOBJEND);
		@echo "===== COPYING BINARY =====";
		sudo mount -o loop $(DISK) $(DISKMOUNT);
		cp $(BIN) $(DISKMOUNT);
		sudo umount $(DISKMOUNT);

$(AOBJ):	$(ASRC) $(ASRCEND)
		@echo "===== ASSEMBLING =====";
		nasm -g -f elf -o $(AOBJ) $(ASRC);
		nasm -g -f elf -o $(AOBJEND) $(ASRCEND);

$(BUILD)/%.o:		src/%.c
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD)/depend.mak:	$(CSRC)
		@echo "===== GENERATING DEPENDENCIES =====";
		$(CC) $(CFLAGS) -MM $(CSRC) > $(BUILD)/depend.mak;
		@# prefix all files with the build-path (otherwise make wouldn't find them)
		sed --in-place -e "s/\([a-zA-Z_]*\).o:/$(subst /,\/,$(BUILD))\/\1.o:/g" $(BUILD)/depend.mak;

-include $(BUILD)/depend.mak

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(BUILD)/depend.mak $(BUILD)/*.o $(BIN) $(COBJ) $(AOBJ) \
			$(BUILD)/tools_gensymbols.c $(BUILD)/tools_gensymbols.o;
