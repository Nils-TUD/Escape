ROOT = ../../..
BUILDL = $(BUILD)/user/d/$(NAME)
BIN = $(BUILD)/user_$(NAME).bin
LIBD = $(ROOT)/lib/d
SUBDIRS = . $(filter-out Makefile $(wildcard *.*),$(wildcard *))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -mindepth 0 -maxdepth 1 -name "*.d")

DC = dmd
CFLAGS = $(CDEFFLAGS)
DFLAGS = $(DDEFFLAGS) -version=Escape -I$(LIBD) -I$(LIBD)/tango/core -I$(LIBD)/tango/core/vendor \
	-I$(LIBD)/tango/core/rt/compiler/dmd -L-L$(BUILD)
ifeq ($(LINKTYPE),static)
	CFLAGS += -static -Wl,-Bstatic
endif

# sources
DSRC = $(shell find $(SUBDIRS) -mindepth 0 -maxdepth 1 -name "*.d")

# objects
DOBJ = $(patsubst %.d,$(BUILDL)/%.o,$(DSRC))

.PHONY: all clean

-include $(ROOT)/sysdeps.mk

all:	$(BUILDDIRS) $(BIN)

$(BIN): $(DEP_START) $(DEP_DEFLIBS) $(DOBJ) $(ADDLIBS)
		@echo "	" LINKING $(BIN)
		@$(CC) $(CFLAGS) -o $(BIN) $(DOBJ) -ld $(ADDLIBS);

$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/%.o:		%.d
		@echo "	" DC $<
		@$(DC) $(DFLAGS) -of$@ -c $<

-include $(DEPS)

clean:
		rm -f $(BIN) $(DOBJ) $(DEPS)
