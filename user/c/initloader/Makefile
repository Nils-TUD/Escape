NAME = initloader
ROOT = ../../..
BUILDL = $(BUILD)/user/c/$(NAME)
BIN = $(BUILD)/user_$(NAME).bin
DUMP = $(BUILD)/user_$(NAME).dump
MAP = $(BUILD)/user_$(NAME).map
LIBC = $(ROOT)/lib/c
SUBDIRS = . $(filter-out Makefile $(wildcard *.*),$(wildcard *))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -mindepth 0 -maxdepth 1 -name "*.d")

CFLAGS = -static -Wl,-Bstatic -nostartfiles $(CDEFFLAGS)

ASRC = $(wildcard $(ARCH)/*.s)
AOBJ = $(patsubst %.s,$(BUILDL)/%.a.o,$(ASRC))
SYSCALLS = $(LIBC)/arch/$(ARCH)/syscalls.s

.PHONY: all clean

-include $(ROOT)/sysdeps.mk

all:	$(BUILDDIRS) $(BIN) $(MAP) $(DUMP)

$(BIN):	$(DEP_START) $(DEP_DEFLIBS) $(AOBJ)
		@echo "	" LINKING $(BIN)
		@$(CC) $(CFLAGS) -o $(BIN) $(AOBJ);

$(MAP): $(BIN)
		@echo "	" GEN MAP $@
		@$(NM) -S $(BIN) | $(ROOT)/tools/createmap-mmix.php > $@

$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/%.a.o: %.s	$(SYSCALLS)
		@echo "	" AS $<
		@$(AS) $(ASFLAGS) -o $@ $<

$(DUMP):	$(BIN)
		@echo "	" GENERATING DUMP
		@$(BUILD)/dump $(BIN) > $(DUMP)

-include $(DEPS)

clean:
		rm -f $(BIN) $(MAP) $(DEPS) $(DUMP) $(COBJ)
