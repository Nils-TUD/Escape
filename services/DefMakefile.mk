ROOT = ../..
BUILD = $(ROOT)/build
BUILDL = $(BUILD)/services/$(NAME)
BIN = $(BUILD)/service_$(NAME).bin
LIBC = $(ROOT)/libc
LDCONF = $(LIBC)/ld.conf
SUBDIRS = . $(filter-out Makefile $(wildcard *.*),$(wildcard *))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -mindepth 0 -maxdepth 1 -name "*.d")
APP = $(NAME).app
APPCPY = $(BUILD)/apps/$(APP)

CC = gcc
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs -I$(LIBC)/include -I../../lib/h -Wl,-T,$(LDCONF) $(CDEFFLAGS) 

# sources
CSRC = $(shell find $(SUBDIRS) -mindepth 0 -maxdepth 1 -name "*.c")

# objects
LIBCA = $(BUILD)/libc.a
START = $(BUILD)/libc_startup.o
COBJ = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))

.PHONY: all clean

all:	$(APPCPY) $(BIN)

$(BIN):	$(BUILDDIRS) $(APPDST) $(LDCONF) $(COBJ) $(START) $(LIBCA)
		@echo "	" LINKING $(BIN)
		@$(CC) $(CFLAGS) -o $(BIN) $(START) $(COBJ) $(LIBCA);
		@echo "	" COPYING ON DISK
		$(ROOT)/tools/disk.sh copy $(BIN) /sbin/$(NAME)

$(APPCPY): $(APP)
		$(ROOT)/tools/disk.sh copy $(APP) /apps/$(NAME)
		cp $(APP) $(APPCPY)

$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/%.o:		%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MMD

-include $(DEPS)

clean:
		rm -f $(APPCPY) $(BIN) $(COBJ) $(DEPS)
