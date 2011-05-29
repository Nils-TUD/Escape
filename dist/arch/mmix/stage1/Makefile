ROOT = ../../../..
BUILDL = $(BUILD)/stage1
BIN = $(BUILD)/stage1.bin
ELF = $(BUILD)/stage1.elf.bin
MAP = $(BUILD)/stage1.map
SUBDIRS = . $(filter-out Makefile $(wildcard *.*),$(wildcard *))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -mindepth 0 -maxdepth 1 -name "*.d")

CFLAGS = -ffreestanding -nostdlib -nostartfiles -nodefaultlibs $(CDEFFLAGS)

# sources
ASRC = br.s
LDCONF = ld.conf

# objects
AOBJ = $(patsubst %.s,$(BUILDL)/%.s.o,$(ASRC))

.PHONY: all clean

all:		$(BUILDDIRS) $(BIN) $(MAP) $(ELF)

$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(ELF):	$(AOBJ) $(LDCONF)
		@echo "	" LINKING $@
		@$(CC) $(CFLAGS) -Wl,-T,$(LDCONF) -o $(ELF) $(AOBJ)

$(MAP): $(ELF)
		@echo "	" GEN MAP $@
		@$(NM) -S $(ELF) | $(ROOT)/tools/createmap-mmix.php > $@

$(BIN):	$(ELF)
		@echo "	" GEN BIN $@
		@$(OBJCOPY) -O binary $(ELF) $(BIN)
		@echo "	" COPYING TO DISK
		@dd if=$(BIN) of=$(BUILD)/hd.img bs=512 count=1 conv=notrunc

$(BUILDL)/%.s.o:	%.s
		@echo "	" AS $<
		@$(AS) -o $@ $<

-include $(DEP)

clean:
		rm -f *~ $(AOBJ) $(BIN) $(ELF) $(DEP) $(MAP)
