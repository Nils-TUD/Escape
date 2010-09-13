ROOT = ../..
BUILDL = $(BUILD)/lib/d
SUBDIRS = $(filter-out Makefile $(shell find . -name "*.*"),$(shell find . -name "*"))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -name "*.d")
LIB = $(BUILD)/libd.a

CFLAGS = $(CDEFFLAGS)
DC = dmd
DFLAGS = $(DDEFFLAGS) -version=Escape -I. -Itango/core -Itango/core/vendor \
	-Itango/core/rt/compiler/dmd

# sources
CSRC = $(shell find $(SUBDIRS) -name "*.c")
DSRC = $(shell find $(SUBDIRS) -name "*.d")

# objects
COBJS = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
DOBJS = $(patsubst %.d,$(BUILDL)/%.o,$(DSRC))

.PHONY:		all clean

all: $(BUILDDIRS) $(LIB)

$(LIB): $(COBJS) $(DOBJS)
		@echo "	" AR $(LIB)
		@ar rcs $(LIB) $(COBJS) $(DOBJS)
		$(ROOT)/tools/linklib.sh $(LIB)
		
$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/%.o: %.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -MD -c -o $@ $<

$(BUILDL)/%.o: %.d
		@echo "	" DC $<
		@$(DC) $(DFLAGS) -c -of$@ $<

-include $(DEPS)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(COBJS) $(DOBJS) $(LIB) $(DEPS)
