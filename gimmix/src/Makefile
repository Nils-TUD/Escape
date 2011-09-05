BUILD = ../build
INC = ../include
BUILDL = $(BUILD)/src
BINNAME = gimmix
BIN = $(BUILD)/$(BINNAME)
SUBDIRS = $(shell find . -type d | grep -v '\.svn')
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -name "*.d")
LIBS = $(BUILD)/libgetline.a $(BUILD)/libmmix.a -lX11 -lpthread -lm

LEXSRC = cli/lang/scanner.l
LEXGEN = cli/lang/lex.c
BISONSRC = cli/lang/parser.y
BISONGEN = cli/lang/parser.c
BISONDEBUG = $(BUILDL)/cli/lang/parser.out

CSRC = $(filter-out ./$(BISONGEN) ./$(LEXGEN),$(shell find . -name "*.c"))
COBJ = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
GENOBJ = $(patsubst %.c,$(BUILDL)/%.o,$(BISONGEN) $(LEXGEN))

CFLAGS += --coverage -L/usr/X11R6/lib

all:	$(BUILDDIRS) $(BUILD)/abstime.h $(BIN)

$(BUILDDIRS):
	@for i in $(BUILDDIRS); do \
		if [ ! -d $$i ]; then mkdir -p $$i; fi \
	done;

$(BUILD)/abstime.h:
	@echo "	" GENERATING $@
	@$(BUILD)/abstime > $@

$(BIN): $(GENOBJ)	$(COBJ) $(LIBS)
	@echo "	" LINKING $@
	@$(CC) $(CFLAGS) -o $@ $(COBJ) $(GENOBJ) $(LIBS)

$(LEXGEN):	$(LEXSRC)
	@echo "	" GEN $@
	@flex -o $(LEXGEN) < $(LEXSRC)

$(BISONGEN):	$(BISONSRC)
	@echo "	" GEN $@
	@bison -t -d --report=all --report-file=$(BISONDEBUG) -o $(BISONGEN) $(BISONSRC)

$(BUILDL)/%.o:	%.c
	@echo "	" CC $<
	@$(CC) $(CFLAGS) -I $(INC) -o $@ -c $< -MD

-include $(DEPS)

clean:
	rm -f $(BIN) $(COBJ) $(GENOBJ) $(LEXGEN) $(BISONGEN) $(BISONDEBUG) $(DEPS)
