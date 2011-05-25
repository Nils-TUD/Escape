ROOT = ../../../..
BUILDL = $(BUILD)/stage1
BIN = $(BUILD)/stage1.bin
MAP = $(BUILD)/stage1.map
SUBDIRS = . $(filter-out Makefile $(wildcard *.*),$(wildcard *))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -mindepth 0 -maxdepth 1 -name "*.d")

CFLAGS = -ffreestanding -nostdlib -nostartfiles -nodefaultlibs $(CDEFFLAGS)

# sources
ASRC = br.s sctio-ctl.s
LDCONF = ld.conf

# objects
AOBJ = $(patsubst %.s,$(BUILDL)/%.s.o,$(ASRC))

.PHONY: all clean

all:		$(BUILDDIRS) $(BIN) $(MAP)

$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BIN):	$(AOBJ) $(LDCONF)
		@echo "	" LINKING $<
		@$(CC) $(CFLAGS) -Wl,-s -Wl,-x -Wl,-T,$(LDCONF) -o $(BIN) $(AOBJ)
		@echo "	" COPYING TO DISK
		@dd if=$(BIN) of=$(BUILD)/hd.img bs=512 count=1 conv=notrunc

$(MAP): $(BIN)
		@echo "	" GEN $@
		@$(LD) -s -x -T $(LDCONF) -o $(BIN) $(AOBJ) -M | gawk -f $(ROOT)/tools/createmap.awk > $@

$(BUILDL)/%.s.o:	%.s
		@echo "	" AS $<
		@$(AS) -o $@ $<

-include $(DEP)

clean:
		rm -f *~ $(AOBJ) $(BIN) $(DEP) $(MAP)
