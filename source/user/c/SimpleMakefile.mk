ROOT = ../../..
BUILDL = $(BUILD)/user/c/$(NAME)
BIN = $(BUILD)/user_$(NAME).bin
MAP = $(BUILD)/user_$(NAME).map
DEPS = $(wildcard $(BUILDL)/*.d)

CFLAGS = $(CDEFFLAGS) $(ADDFLAGS)
ifeq ($(LINKTYPE),static)
	CFLAGS += -static -Wl,-Bstatic
endif

# sources
CSRC = $(wildcard *.c)

# objects
COBJ = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))

.PHONY: all clean

-include $(ROOT)/sysdeps.mk

all:	$(BUILDL) $(BIN) $(MAP)

$(BIN):	$(DEP_START) $(DEP_DEFLIBS) $(COBJ) $(ADDLIBS)
	@echo "	" LINKING $(BIN)
	@$(CC) $(CFLAGS) -o $(BIN) $(COBJ) $(ADDLIBS);

$(MAP): $(BIN)
	@echo "	" GEN MAP $@
	@$(NM) -S $(BIN) | $(ROOT)/tools/createmap-mmix.php > $@

$(BUILDL):
	@if [ ! -d $@ ]; then mkdir -p $@; fi

$(BUILDL)/%.o:	%.c
	@echo "	" CC $<
	@$(CC) $(CFLAGS) -o $@ -c $< -MD

-include $(DEPS)

clean:
	rm -f $(BIN) $(MAP) $(COBJ) $(DEPS)
