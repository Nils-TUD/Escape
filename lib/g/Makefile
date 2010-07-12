ROOT = ../..
BUILDL = $(BUILD)/lib/g
SUBDIRS = $(filter-out $(shell find . -name "*.*"),$(shell find . -name "*"))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -name "*.d")
STLIB = $(BUILD)/libg.a

CC = $(ROOT)/build/dist/bin/i586-elf-escape-gcc
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs $(CDEFFLAGS)

# sources
CSRC = $(shell find . -name "*.c")
# objects
COBJS = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))

.PHONY:		all clean

all: $(BUILDDIRS) $(STLIB)

$(STLIB): $(COBJS)
		@echo "	" AR $(STLIB)
		@ar rcs $(STLIB) $(COBJS)

$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/%.o: %.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MD

-include $(DEPS)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(COBJS) $(STLIB) $(DEPS)
