NAME = initloader
ROOT = ../../..
BUILDL = $(BUILD)/user/c/$(NAME)
BIN = $(BUILD)/user_$(NAME).bin
DUMP = $(BUILD)/user_$(NAME).dump
LIBC = $(ROOT)/lib/c
LDCONF = $(ROOT)/lib/ld.conf
SUBDIRS = . $(filter-out Makefile $(wildcard *.*),$(wildcard *))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -mindepth 0 -maxdepth 1 -name "*.d")

CC = gcc
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs -Wl,-T,$(LDCONF) -Wl,--build-id=none $(CDEFFLAGS) \
	-I$(ROOT)/include

# sources
ASRC = $(wildcard *.s)
AOBJ = $(patsubst %.s,$(BUILDL)/a%.o,$(ASRC))
SYSCALLS = $(LIBC)/syscalls.s

# objects
CSRC = $(wildcard *.c)
COBJ = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
LIBCA = $(BUILD)/libc.a

.PHONY: all clean

all:	$(BIN) $(DUMP)

$(BIN):	$(BUILDDIRS) $(LDCONF) $(AOBJ) $(COBJ)
		@echo "	" LINKING $(BIN)
		@$(CC) $(CFLAGS) -o $(BIN) $(AOBJ) $(COBJ) $(LIBCA);

$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/a%.o: %.s	$(SYSCALLS)
		@echo "	" AS $<
		@nasm $(ASMFLAGS) -o $@ $<

$(BUILDL)/%.o: %.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MMD

$(DUMP):	$(BIN)
		@echo "	" GENERATING DUMP
		@$(BUILD)/dump $(BIN) > $(DUMP)

-include $(DEPS)

clean:
		rm -f $(BIN) $(DEPS) $(DUMP) $(AOBJ) $(COBJ)
