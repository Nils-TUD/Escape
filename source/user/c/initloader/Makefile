NAME = initloader
ROOT = ../../..
BUILDL = $(BUILD)/user/c/$(NAME)
BIN = $(BUILD)/user_$(NAME).bin
DUMP = $(BUILD)/user_$(NAME).dump
MAP = $(BUILD)/user_$(NAME).map
LIBC = $(ROOT)/lib/c
SUBDIRS = $(shell find . -type d | grep -v '\.svn')
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDL) -name "*.d")

CFLAGS = -static -Wl,-Bstatic -nostartfiles $(CDEFFLAGS)

ASRC = $(wildcard $(ARCH)/*.s)
AOBJ = $(patsubst %.s,$(BUILDL)/%.o,$(ASRC))

.PHONY: all clean

-include $(ROOT)/sysdeps.mk

all:	$(BUILDDIRS) $(BIN) $(MAP) $(DUMP)

$(BIN):	$(DEP_START) $(DEP_DEFLIBS) $(AOBJ)
	@echo "	" LINKING $(BIN)
	@$(CC) $(CFLAGS) -o $(BIN) $(AOBJ);

$(MAP): $(BIN)
	@echo "	" GEN MAP $@
	@$(ROOT)/tools/createmap-$(ARCH) $(BIN) > $@

$(BUILDDIRS):
	@for i in $(BUILDDIRS); do \
		if [ ! -d $$i ]; then mkdir -p $$i; fi \
	done;

$(BUILDL)/%.o: %.s
	@echo "	" CPP $<
	@$(CPP) -MD -MT $@ -MF $@.d $< > $@.tmp
	@echo "	" AS $<
	@$(AS) $(ASFLAGS) -o $@ $@.tmp
	@rm -f $@.tmp

$(DUMP):	$(BIN)
	@echo "	" GENERATING DUMP
	@$(BUILD)/dump $(BIN) > $(DUMP)

-include $(DEPS)

clean:
	rm -f $(BIN) $(MAP) $(DEPS) $(DUMP) $(COBJ)
