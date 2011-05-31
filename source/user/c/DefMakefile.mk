ROOT = ../../..
BUILDL = $(BUILD)/user/c/$(NAME)
BIN = $(BUILD)/user_$(NAME).bin
MAP = $(BUILD)/user_$(NAME).map
SUBDIRS = . $(filter-out Makefile $(wildcard *.*),$(wildcard *))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -mindepth 0 -maxdepth 1 -name "*.d")

CFLAGS = $(CDEFFLAGS) $(ADDFLAGS)
ifeq ($(LINKTYPE),static)
	CFLAGS += -static -Wl,-Bstatic
endif

# sources
CSRC = $(shell find $(SUBDIRS) -mindepth 0 -maxdepth 1 -name "*.c")

# objects
COBJ = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))

.PHONY: all clean

-include $(ROOT)/sysdeps.mk

all:	$(BUILDDIRS) $(APPCPY) $(BIN) $(MAP)

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
		rm -f $(APPCPY) $(BIN) $(MAP) $(COBJ) $(DEPS)
