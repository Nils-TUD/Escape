ROOT = ../../..
BUILDL = $(BUILD)/user/c/$(NAME)
BIN = $(BUILD)/user_$(NAME).bin
MAP = $(BUILD)/user_$(NAME).map
SUBDIRS = $(shell find . -type d | grep -v '\.svn')
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDL) -name "*.d")

LIBDEPS += $(BUILD)/libc.a $(BUILD)/libg.a $(BUILD)/libm.a
CFLAGS = $(CDEFFLAGS) $(ADDFLAGS)
ifeq ($(LINKTYPE),static)
	CFLAGS += -static -Wl,-Bstatic
endif

# sources
CSRC = $(filter-out $(shell find ./arch -name "*.c" 2>/dev/null),$(shell find . -name "*.c"))
CSRC += $(shell find ./arch/$(ARCH) -name "*.c" 2>/dev/null)
ASRC = $(shell find ./arch/$(ARCH) -name "*.s" 2>/dev/null)

# objects
COBJ = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
AOBJ = $(patsubst %.s,$(BUILDL)/%.a.o,$(ASRC))

.PHONY: all clean

-include $(ROOT)/sysdeps.mk

all:	$(BUILDDIRS) $(APPCPY) $(BIN) $(MAP)

$(BIN):	$(DEP_START) $(DEP_DEFLIBS) $(COBJ) $(AOBJ) $(LIBDEPS)
	@echo "	" LINKING $(BIN)
	@$(CC) $(CFLAGS) -o $(BIN) $(COBJ) $(AOBJ) $(ADDLIBS);

$(MAP): $(BIN)
	@echo "	" GEN MAP $@
	@$(ROOT)/tools/createmap-$(ARCH) $(BIN) > $@

$(BUILDDIRS):
	@for i in $(BUILDDIRS); do \
		if [ ! -d $$i ]; then mkdir -p $$i; fi \
	done;

$(BUILDL)/%.o:	%.c
	@echo "	" CC $<
	@$(CC) $(CFLAGS) -o $@ -c $< -MD

$(BUILDL)/%.a.o:	%.s
	@echo "	" CPP $<
	@$(CPP) -MD -MT $@ -MF $@.d $< > $@.tmp
	@echo "	" AS $<
	@$(AS) $(ASFLAGS) -o $@ $@.tmp
	@rm -f $@.tmp

-include $(DEPS)

clean:
	rm -f $(APPCPY) $(BIN) $(MAP) $(COBJ) $(AOBJ) $(DEPS)
