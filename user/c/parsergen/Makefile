NAME=parsergen
ROOT = ../../..
BUILD = $(ROOT)/build
BUILDL = $(BUILD)/user/c/$(NAME)
BIN = $(BUILD)/user_$(NAME).bin
LIB = $(ROOT)/lib
LIBC = $(ROOT)/libc
LDCONF = $(LIBC)/ld.conf
SUBDIRS = . $(filter-out Makefile $(wildcard *.*),$(wildcard *))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -mindepth 0 -maxdepth 1 -name "*.d")
APP = $(NAME).app
APPCPY = $(BUILD)/apps/$(APP)

CC = gcc
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs -I$(LIBC)/include -I$(LIB)/h \
	-Wl,-T,$(LDCONF) -Wl,--build-id=none $(CDEFFLAGS) $(ADDFLAGS)

# sources
LEXSRC = test.l
LEXGEN = lex.yy.c
CSRC = $(filter-out ./$(LEXGEN),$(shell find $(SUBDIRS) -mindepth 0 -maxdepth 1 -name "*.c"))

# objects
LIBCA = $(BUILD)/libc.a
START = $(BUILD)/libc_startup.o
COBJ = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
LEXOBJ = $(patsubst %.c,$(BUILDL)/%.o,$(LEXGEN))

.PHONY: all clean

all:	$(APPCPY) $(LEXGEN) $(BIN)

$(BIN):	$(BUILDDIRS) $(APPDST) $(LDCONF) $(COBJ) $(LEXOBJ) $(START) $(LIBCA) $(ADDLIBS)
		@echo "	" LINKING $(BIN)
		$(CC) $(CFLAGS) -o $(BIN) $(START) $(COBJ) $(LEXOBJ) $(LIBCA) $(ADDLIBS);
		@echo "	" COPYING ON DISK
		$(ROOT)/tools/disk.sh copy $(BIN) /bin/$(NAME)

$(LEXGEN):	$(LEXSRC)
		flex -o $(LEXGEN) --nounistd < $(LEXSRC)

$(APPCPY):	$(APP)
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
		rm -f $(APPCPY) $(BIN) $(COBJ) $(DEPS) $(LEXGEN)
