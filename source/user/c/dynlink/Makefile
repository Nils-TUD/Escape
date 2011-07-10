NAME = dynlink
ROOT = ../../..
BUILDL = $(BUILD)/user/c/$(NAME)
BIN = $(BUILD)/user_$(NAME).bin
LDCONF = ld.conf
SUBDIRS = $(shell find . -type d | grep -v '\.svn')
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDL) -name "*.d")

LIBDEPS += $(BUILD)/libc.a $(BUILD)/libg.a $(BUILD)/libm.a
CFLAGS = -static -Wl,-Bstatic -nostartfiles $(CDEFFLAGS) $(ADDFLAGS)

# sources
CSRC = $(shell find . -name "*.c")
ASTART = startup.s

# objects
COBJ = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
START = $(patsubst %.s,$(BUILDL)/%.s.o,$(ASTART))

.PHONY: all clean

-include $(ROOT)/sysdeps.mk

all:	$(BUILDDIRS) $(BIN)

$(BIN):	$(DEP_START) $(DEP_DEFLIBS) $(LDCONF) $(COBJ) $(START) $(LIBDEPS)
	@echo "	" LINKING $(BIN)
	@$(CC) $(CFLAGS) -Wl,-T,$(LDCONF) -o $(BIN) $(START) $(COBJ)

$(BUILDDIRS):
	@for i in $(BUILDDIRS); do \
		if [ ! -d $$i ]; then mkdir -p $$i; fi \
	done;

$(BUILDL)/%.o:		%.c
	@echo "	" CC $<
	@$(CC) $(CFLAGS) -o $@ -c $< -MD

$(BUILDL)/%.s.o:	%.s
	@echo "	" CPP $<
	@$(CPP) -MD -MT $@ -MF $@.d $< > $@.tmp
	@echo "	" AS $<
	#@$(AS) $(ASFLAGS) --defsym CALLTRACE_PID=10 -o $@ $@.tmp
	@$(AS) $(ASFLAGS) -o $@ $@.tmp
	@rm -f $@.tmp

-include $(DEPS)

clean:
	rm -f $(BIN) $(COBJ) $(DEPS)
