NAME=calc
ROOT = ../../..
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
LIBCA = $(BUILD)/libc.a

CC = gcc
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs -I$(LIBC)/include -I$(LIB)/h \
	-Wl,-T,$(LDCONF) -Wl,--build-id=none $(CDEFFLAGS) $(ADDFLAGS)

ifeq ($(LINKTYPE),static)
	ADDLIBS += $(LIBCA)
endif

# sources
LEXSRC = calc.l
LEXGEN = lex.c
BISONSRC = calc.y
BISONGEN = parser.c
CSRC = $(filter-out ./$(BISONGEN) ./$(LEXGEN),\
	$(shell find $(SUBDIRS) -mindepth 0 -maxdepth 1 -name "*.c"))

# objects
START = $(BUILD)/libc_startup.o
COBJ = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
GENOBJ = $(patsubst %.c,$(BUILDL)/%.o,$(LEXGEN) $(BISONGEN))

.PHONY: all clean

all:	$(APPCPY) $(BISONGEN) $(LEXGEN) $(BIN)

$(BIN):	$(BUILDDIRS) $(APPDST) $(LDCONF) $(COBJ) $(GENOBJ) $(START) $(ADDLIBS)
		@echo "	" LINKING $(BIN)
ifeq ($(LINKTYPE),static)
		$(CC) $(CFLAGS) -o $(BIN) $(START) $(COBJ) $(GENOBJ) $(ADDLIBS);
else
		$(CC) $(CFLAGS) $(DLNKFLAGS) -o $(BIN) -lc $(START) $(COBJ) $(GENOBJ) $(ADDLIBS);
endif
		@echo "	" COPYING ON DISK
		$(ROOT)/tools/disk.sh copy $(BIN) /bin/$(NAME)

$(LEXGEN):	$(LEXSRC)
		flex -o $(LEXGEN) --nounistd < $(LEXSRC)

$(BISONGEN):	$(BISONSRC)
		bison -d -o $(BISONGEN) $(BISONSRC)

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
		rm -f $(APPCPY) $(BIN) $(COBJ) $(DEPS) $(LEXGEN) $(BISONGEN) $(GENOBJ)
