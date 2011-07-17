ROOT = ../../../..
BUILDL = $(BUILD)/stage2
LIB = $(ROOT)/lib/basic
BIN = $(BUILD)/stage2.bin
MAP = $(BUILD)/stage2.map
ELF = $(BUILD)/stage2.elf.bin
SUBDIRS = $(shell find . -type d)
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDL) -name "*.d")

CFLAGS = -ffreestanding -nostartfiles -nodefaultlibs -nostdlib $(CDEFFLAGS)

# sources
CSRC = $(shell find . -name "*.c")
LSRC = $(shell find $(LIB)/arch/$(ARCH) -name "*.c")
LASRC = $(shell find $(LIB)/arch/$(ARCH) -name "*.s")
ASTART = c0.s
ASRC = 
AEND = c1.s
LDCONF = ld.conf

# objects
COBJ = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
LOBJ = $(patsubst $(LIB)/arch/$(ARCH)/%.c,$(BUILDL)/%.l.o,$(LSRC))
LOBJ += $(patsubst $(LIB)/arch/$(ARCH)/%.s,$(BUILDL)/%.l.a.o,$(LASRC))
AOBJ = $(patsubst %.s,$(BUILDL)/%.s.o,$(ASRC))
AOBJS = $(BUILDL)/c0.o
AOBJE = $(BUILDL)/c1.o
AOBJALL = $(AOBJS) $(AOBJ) $(AOBJE)

.PHONY: all clean

all:		$(BUILDDIRS) $(BIN) $(ELF) $(MAP)

$(BUILDDIRS):
	@for i in $(BUILDDIRS); do \
		if [ ! -d $$i ]; then mkdir -p $$i; fi \
	done;

$(ELF):	$(COBJ) $(AOBJALL) $(LOBJ) $(LDCONF)
	@echo "	" LINKING $@
	@$(CC) $(CFLAGS) -Wl,-T,$(LDCONF) -o $(ELF) $(AOBJS) $(COBJ) $(AOBJ) $(LOBJ) $(AOBJE)

$(MAP): $(ELF)
	@echo "	" GEN MAP $@
	@$(ROOT)/tools/createmap-$(ARCH) $(ELF) > $@

$(BIN): $(ELF)
	@echo "	" GEN BIN $@
	@$(OBJCOPY) -S $(ELF) $(BIN)
	@echo "	" COPYING TO DISK
	@dd if=$(BIN) of=$(BUILD)/hd.img bs=512 seek=1 conv=notrunc

$(BUILDL)/%.o:		%.c
	@echo "	" CC $<
	@$(CC) $(CFLAGS) -c -o $@ $< -MD

$(BUILDL)/%.l.o:	$(LIB)/arch/$(ARCH)/%.c
	@echo "	" CC $<
	@$(CC) $(CFLAGS) -c -o $@ $< -MD

$(BUILDL)/%.o:	%.s
	@echo "	" AS $<
	@$(AS) -o $@ $<

$(BUILDL)/%.l.a.o:	$(LIB)/arch/$(ARCH)/%.s
	@echo "	" AS $<
	@$(AS) -o $@ $<

-include $(DEP)

clean:
	rm -f *~ $(COBJ) $(AOBJALL) $(LOBJ) $(BIN) $(ELF) $(DEP) $(MAP)
