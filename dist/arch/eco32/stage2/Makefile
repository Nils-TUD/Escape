ROOT = ../../../..
BUILDL = $(BUILD)/stage2
BIN = $(BUILD)/stage2.bin
MAP = $(BUILD)/stage2.map
ELF = $(BUILD)/stage2.elf.bin
SUBDIRS = . $(filter-out Makefile $(wildcard *.*),$(wildcard *))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -mindepth 0 -maxdepth 1 -name "*.d")

CFLAGS = -ffreestanding -nostdlib -nostartfiles -nodefaultlibs $(CDEFFLAGS)

# sources
CSRC = $(shell find . -name "*.c")
ASTART = c0.s
ASRC = sctio-ctl.s
AEND = c1.s
LDCONF = ld.conf

# objects
COBJ = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
AOBJ = $(patsubst %.s,$(BUILDL)/%.s.o,$(ASRC))
AOBJS = $(BUILDL)/c0.s.o
AOBJE = $(BUILDL)/c1.s.o
AOBJALL = $(AOBJS) $(AOBJ) $(AOBJE)

.PHONY: all clean

all:		$(BUILDDIRS) $(BIN) $(ELF) $(MAP)

$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(ELF):	$(COBJ) $(AOBJALL) $(LOBJ) $(LDCONF)
		@echo "	" LINKING $@
		@$(CC) $(CFLAGS) -Wl,-T,$(LDCONF) -o $(ELF) $(AOBJS) $(COBJ) $(AOBJ) $(AOBJE)

$(MAP): $(ELF)
		@echo "	" GEN MAP $@
		@$(NM) -S $(ELF) | $(ROOT)/tools/createmap-mmix.php > $@

$(BIN): $(ELF)
		@echo "	" GEN BIN $@
		@$(OBJCOPY) -O binary $(ELF) $(BIN)
		@echo "	" COPYING TO DISK
		@dd if=$(BIN) of=$(BUILD)/hd.img bs=512 seek=1 conv=notrunc

$(BUILDL)/%.o:		%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -c -o $@ $< -MD

$(BUILDL)/%.s.o:	%.s
		@echo "	" AS $<
		@$(AS) -o $@ $<

-include $(DEP)

clean:
		rm -f *~ $(COBJ) $(AOBJALL) $(LOBJ) $(BIN) $(DEP) $(MAP)
