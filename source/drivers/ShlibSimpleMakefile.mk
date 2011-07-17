ROOT = ../../..
BUILDL = $(BUILD)/drivers/$(NAME)
BIN = $(BUILD)/driver_$(NAME).bin
MAP = $(BUILD)/driver_$(NAME).map
DEPS = $(wildcard $(BUILDL)/*.d)

LIBDEPS += $(BUILD)/libc.a $(BUILD)/libg.a $(BUILD)/libm.a
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

$(BIN):	$(DEP_START) $(DEP_DEFLIBS) $(COBJ) $(LIBDEPS)
	@echo "	" LINKING $(BIN)
	@$(CC) $(CFLAGS) -o $(BIN) $(COBJ) $(ADDLIBS);

$(MAP): $(BIN)
	@echo "	" GEN MAP $@
	@$(ROOT)/tools/createmap-$(ARCH) $(BIN) > $@

$(BUILDL):
	@if [ ! -d $@ ]; then mkdir -p $@; fi

$(BUILDL)/%.o:	%.c
	@echo "	" CC $<
	@$(CC) $(CFLAGS) -o $@ -c $< -MD

-include $(DEPS)

clean:
	rm -f $(BIN) $(MAP) $(COBJ) $(DEPS)
