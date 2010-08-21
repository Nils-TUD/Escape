NAME = dynlink
ROOT = ../../..
BUILDL = $(BUILD)/user/c/$(NAME)
BIN = $(BUILD)/user_$(NAME).bin
LDCONF = ld.conf
SUBDIRS = . $(filter-out Makefile $(wildcard *.*),$(wildcard *))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -mindepth 0 -maxdepth 1 -name "*.d")

CFLAGS = -static -Wl,-Bstatic -nostartfiles $(CDEFFLAGS) $(ADDFLAGS)

# sources
CSRC = $(shell find $(SUBDIRS) -mindepth 0 -maxdepth 1 -name "*.c")
ASTART = startup.s

# objects
COBJ = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
START = $(patsubst %.s,$(BUILDL)/%.s.o,$(ASTART))

.PHONY: all clean

-include $(ROOT)/sysdeps.mk

all:	$(BUILDDIRS) $(BIN)

$(BIN):	$(DEP_START) $(DEP_DEFLIBS) $(LDCONF) $(COBJ) $(START)
		@echo "	" LINKING $(BIN)
		@$(CC) $(CFLAGS) -Wl,-T,$(LDCONF) -o $(BIN) $(START) $(COBJ)
		@echo "	" COPYING ON DISK
		$(ROOT)/tools/disk.sh copy $(BIN) /bin/$(NAME)

$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/%.o:		%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MD

$(BUILDL)/%.s.o:	%.s
		@echo "	" AS $<
		#@$(AS) $(ASFLAGS) --defsym CALLTRACE_PID=10 -o $@ $< -MD $(BUILDL)/startup.d
		@$(AS) $(ASFLAGS) -o $@ $< -MD $(BUILDL)/startup.d

-include $(DEPS)

clean:
		rm -f $(BIN) $(COBJ) $(DEPS)
