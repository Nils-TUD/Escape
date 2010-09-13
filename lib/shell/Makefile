NAME = shell
ROOT = ../..
BUILDL = $(BUILD)/lib/$(NAME)
SUBDIRS = . $(filter-out Makefile $(wildcard *.*),$(wildcard *))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -mindepth 0 -maxdepth 1 -name "*.d")
STLIB = $(BUILD)/lib$(NAME).a
DYNLIBNAME = lib$(NAME).so
DYNLIB = $(BUILD)/$(DYNLIBNAME)

CFLAGS = $(CDEFFLAGS)

# sources 
LEXSRC = script.l
LEXGEN = lex.c
BISONSRC = script.y
BISONGEN = parser.c
BISONDEBUG = parser.out
CSRC = $(filter-out ./$(BISONGEN) ./$(LEXGEN),\
	$(shell find $(SUBDIRS) -mindepth 0 -maxdepth 1 -name "*.c"))

# objects
COBJ = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
CPICOBJS = $(patsubst %.c,$(BUILDL)/%_pic.o,$(CSRC))
GENOBJ = $(patsubst %.c,$(BUILDL)/%.o,$(LEXGEN) $(BISONGEN))
PICGENOBJ = $(patsubst %.c,$(BUILDL)/%_pic.o,$(LEXGEN) $(BISONGEN))

.PHONY: all clean

all:	$(BUILDDIRS) $(BISONGEN) $(LEXGEN) $(STLIB) $(DYNLIB)

$(STLIB): $(COBJ) $(GENOBJ)
		@echo "	" AR $(STLIB)
		@$(AR) rcs $(STLIB) $(COBJ) $(GENOBJ)
		$(ROOT)/tools/linklib.sh $(STLIB)

$(DYNLIB):	$(CPICOBJS) $(PICGENOBJ) $(SHLDCONF)
		@echo "	" LINKING $(DYNLIB)
		@$(LD) --eh-frame-hdr -shared -soname $(DYNLIBNAME) -o $(DYNLIB) $(CPICOBJS) $(PICGENOBJ);
		$(ROOT)/tools/linklib.sh $(DYNLIB)

$(LEXGEN):	$(LEXSRC)
		@echo "	" GEN $@
		@flex -o $(LEXGEN) --nounistd < $(LEXSRC)

$(BISONGEN):	$(BISONSRC)
		@echo "	" GEN $@
		@bison -t -d --report=all --report-file=$(BISONDEBUG) -o $(BISONGEN) $(BISONSRC)

$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/%.o:		%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -D YYERROR_VERBOSE -o $@ -c $< -MD

$(BUILDL)/%_pic.o: %.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -D YYERROR_VERBOSE -fPIC -o $@ -c $< -MD

-include $(DEPS)

clean:
		rm -f $(STLIB) $(DEPS) $(COBJ) $(CPICOBJS) $(LEXGEN) $(BISONGEN) $(GENOBJ) $(PICGENOBJ)
