# general
BUILD=../build
DISK=$(BUILD)/disk.img
DISKMOUNT=../disk
BINNAME=kernel.bin
BIN=$(BUILD)/$(BINNAME)
LDCONF=ld.conf
DEP=$(BUILD)/kernel.dep
LIB=../lib

CC = gcc
CFLAGS = -fno-inline -ffreestanding -nostdlib -nostartfiles -nodefaultlibs $(CDEFFLAGS) \
	-D IN_KERNEL=1 -I$(LIB)/h

# sources
ASRC=kernel.asm
ASRCEND=end.asm
CSRC=$(wildcard src/*.c)
LSRC=$(wildcard $(LIB)/src/*.c)

# objects
AOBJ=$(BUILD)/kernel_asm.o
AOBJEND=$(BUILD)/end.o
COBJ=$(patsubst src/%.c,$(BUILD)/%.o,$(CSRC))
LOBJ=$(patsubst $(LIB)/src/%.c,$(BUILD)/kernel_lib_%.o,$(LSRC))

SYMBOLS=$(BUILD)/ksymbols.txt
SYMBOLSCRIPT=../tools/gensymbols.sh
SYMBOLSRC=src/ksymbols.c
SYMBOLOBJ=$(patsubst src/%.c,$(BUILD)/%.o,$(SYMBOLSRC))

# we have to put ata and fs into the kernel
# TODO we can put them as modules in grub, too, right?
SERVDIRNAMES = ata fs
SERVBINS = $(patsubst %,$(BUILD)/service_%.bin,$(SERVDIRNAMES))
SERVICES = $(BUILD)/services.txt

.PHONY: all pre clean

all:	$(BIN)

$(BIN):	$(SERVICES) $(LDCONF) $(AOBJ) $(COBJ) $(LOBJ)
		@echo "===== LINKING =====";
		ld -T $(LDCONF) -o $(BIN) $(AOBJ) $(COBJ) $(LOBJ) $(AOBJEND);
		@# Note that we link 2 additional times because otherwise the symbol-table is incorrect
		@# since it's size is 0 at the first time. In the second part the size is correct
		@# and therefore the symbol-table will be generated correctly :)
		@echo "===== BUILDING SYMBOL-FILE =====";
		$(SYMBOLSCRIPT) $(BIN) > $(SYMBOLS)
		@echo "===== COMPILING SYMBOL-C-FILE =====";
		$(CC) $(CFLAGS) -o $(SYMBOLOBJ) -c $(SYMBOLSRC)
		@echo "===== LINKING AGAIN =====";
		ld -T $(LDCONF) -o $(BIN) $(AOBJ) $(COBJ) $(LOBJ) $(AOBJEND);
		@echo "===== BUILDING SYMBOL-FILE =====";
		$(SYMBOLSCRIPT) $(BIN) > $(SYMBOLS)
		@echo "===== COMPILING SYMBOL-C-FILE =====";
		$(CC) $(CFLAGS) -o $(SYMBOLOBJ) -c $(SYMBOLSRC)
		@echo "===== LINKING AGAIN =====";
		ld -T $(LDCONF) -o $(BIN) $(AOBJ) $(COBJ) $(LOBJ) $(AOBJEND);
		@echo "===== COPYING BINARY =====";
		@make -C ../ mounthdd
		sudo cp $(BIN) $(DISKMOUNT)
		@make -C ../ umounthdd

$(SERVICES): $(SERVBINS)
		@echo "" > $(SERVICES)
		@for i in $(SERVDIRNAMES) ; do \
			echo "static u8 service_"$$i"[] = {" >> $(SERVICES); \
			echo "		#include \"../../build/service_$$i.dump\"" >> $(SERVICES); \
			echo "};" >> $(SERVICES); \
		done;
		@echo "" >> $(SERVICES)
		@echo "static sProcData services[] = {" >> $(SERVICES)
		@for i in $(SERVDIRNAMES) ; do \
			echo "	{\"$$i\", service_"$$i"}," >> $(SERVICES); \
		done;
		@echo "};" >> $(SERVICES)

$(AOBJ):	$(ASRC) $(ASRCEND)
		@echo "===== ASSEMBLING =====";
		nasm $(ASMFLAGS) -o $(AOBJ) $(ASRC);
		nasm $(ASMFLAGS) -o $(AOBJEND) $(ASRCEND);

$(BUILD)/%.o:		src/%.c
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD)/kernel_lib_%.o:	$(LIB)/src/%.c
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(DEP):	$(SERVICES) $(CSRC) $(LSRC)
		@echo "===== GENERATING DEPENDENCIES =====";
		if [ ! -f $(SYMBOLS) ]; then echo "" > $(SYMBOLS); fi
		$(CC) $(CFLAGS) -MM $(CSRC) $(LSRC) > $(DEP);
		@# prefix all files with the build-path (otherwise make wouldn't find them)
		sed --in-place -e "s/\([a-zA-Z_]*\).o:/$(subst /,\/,$(BUILD))\/\1.o:/g" $(DEP);

-include $(DEP)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(DEP) $(BIN) $(COBJ) $(AOBJ) $(AOBJEND) $(LOBJ) $(SYMBOLS) $(SYMBOLOBJ) \
			$(BUILD)/tools_gensymbols.c $(BUILD)/tools_gensymbols.o $(SERVICES);
