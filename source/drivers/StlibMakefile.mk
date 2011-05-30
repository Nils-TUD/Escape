ROOT = ../../..
BUILDL = $(BUILD)/drivers/$(NAME)
BIN = $(BUILD)/driver_$(NAME).bin
MAP = $(BUILD)/driver_$(NAME).map
SUBDIRS = $(shell find . -type d | grep -v '\.svn')
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDL) -name "*.d")

CFLAGS = -static -Wl,-Bstatic $(CDEFFLAGS) $(ADDFLAGS)

# sources
CSRC = $(filter-out $(shell find ./arch -name "*.c" 2> /dev/null),$(shell find . -name "*.c"))
CSRC += $(shell find ./arch/$(ARCH) -name "*.c" 2> /dev/null)

# objects
COBJ = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))

.PHONY: all clean

-include $(ROOT)/sysdeps.mk

all:	$(BUILDDIRS) $(BIN) $(MAP)

$(BIN):	$(DEP_START) $(DEP_DEFLIBS) $(COBJ) $(ADDLIBS)
		@echo "	" LINKING $(BIN)
		@$(CC) $(CFLAGS) -o $(BIN) $(COBJ) $(ADDLIBS);

$(MAP): $(BIN)
		@echo "	" GEN MAP $@
		@$(NM) -S $(BIN) | $(ROOT)/tools/createmap-mmix.php > $@

$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/%.o:		%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MD

-include $(DEPS)

clean:
		rm -f $(BIN) $(MAP) $(COBJ) $(DEPS)
