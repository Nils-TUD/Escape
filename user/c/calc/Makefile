NAME=calc
ROOT = ../../..
BUILDL = $(BUILD)/user/c/$(NAME)
BIN = $(BUILD)/user_$(NAME).bin
SUBDIRS = . $(filter-out Makefile $(wildcard *.*),$(wildcard *))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -mindepth 0 -maxdepth 1 -name "*.d")
APP = $(NAME).app
APPCPY = $(BUILD)/apps/$(APP)

CFLAGS = $(CDEFFLAGS) $(ADDFLAGS)
ifeq ($(LINKTYPE),static)
	CFLAGS += -Wl,-Bstatic
endif

# sources
LEXSRC = calc.l
LEXGEN = lex.c
BISONSRC = calc.y
BISONGEN = parser.c
CSRC = $(filter-out ./$(BISONGEN) ./$(LEXGEN),\
	$(shell find $(SUBDIRS) -mindepth 0 -maxdepth 1 -name "*.c"))

# objects
COBJ = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
GENOBJ = $(patsubst %.c,$(BUILDL)/%.o,$(LEXGEN) $(BISONGEN))

.PHONY: all clean

all:	$(APPCPY) $(BISONGEN) $(LEXGEN) $(BIN)

$(BIN):	$(BUILDDIRS) $(APPDST) $(COBJ) $(GENOBJ) $(ADDLIBS)
		@echo "	" LINKING $(BIN)
		$(CC) $(CFLAGS) -o $(BIN) $(COBJ) $(GENOBJ) $(ADDLIBS);
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
		@$(CC) $(CFLAGS) -o $@ -c $< -MD

-include $(DEPS)

clean:
		rm -f $(APPCPY) $(BIN) $(COBJ) $(DEPS) $(LEXGEN) $(BISONGEN) $(GENOBJ)
