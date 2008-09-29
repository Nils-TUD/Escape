# general
BUILD=../build
DISK=$(BUILD)/disk.img
DISKMOUNT=../disk
BINNAME=kernel.bin
BIN=$(BUILD)/$(BINNAME)
LDCONF=src/ld.conf
OSTITLE=hrniels-OS
DUMP=$(BUILD)/kernel.dump

CC = gcc
CFLAGS = -g -O2 -ffreestanding -Wall -ansi -nostdlib -nostartfiles -nodefaultlibs

# sources
ASRC=src/kernel.asm
ASRCEND=src/end.asm
CSRC=$(wildcard src/*.c)

# objects
AOBJ=$(BUILD)/kernel_asm.o
AOBJEND=$(BUILD)/end.o
COBJ=$(patsubst src/%.c,$(BUILD)/%.o,$(CSRC))

.PHONY: all clean

all:	$(BIN)

$(BIN):	$(LDCONF) $(AOBJ) $(COBJ)
		@echo "===== LINKING =====";
		ld -T $(LDCONF) -o $(BIN) $(AOBJ) $(COBJ) $(AOBJEND);
		@echo "===== GENERATING OBJECT DUMP =====";
		objdump -l -d $(BIN) > $(DUMP);
		@echo "===== COPYING BINARY =====";
		sudo mount -o loop $(DISK) $(DISKMOUNT);
		cp $(BIN) $(DISKMOUNT);
		sudo umount $(DISKMOUNT);

$(AOBJ):	$(ASRC) $(ASRCEND)
		@echo "===== ASSEMBLING =====";
		nasm -f elf -o $(AOBJ) $(ASRC);
		nasm -f elf -o $(AOBJEND) $(ASRCEND);

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
		rm -f $(DUMP) $(BUILD)/depend.mak $(BUILD)/*.o $(BIN) $(COBJ) $(AOBJ);
