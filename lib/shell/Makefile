NAME = shell
ROOT = ../..
BUILDL = $(BUILD)/lib/$(NAME)
LIB = $(ROOT)/lib/basic
LIBC = $(ROOT)/lib/c
SUBDIRS = . $(filter-out Makefile $(wildcard *.*),$(wildcard *))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -mindepth 0 -maxdepth 1 -name "*.d")
STLIB = $(BUILD)/lib$(NAME).a
DYNLIBNAME = lib$(NAME).so
DYNLIB = $(BUILD)/$(DYNLIBNAME)
SHLDCONF = ../shld.conf

CC = gcc
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs -I$(LIBC)/include -I$(LIB)/h $(CDEFFLAGS)

# sources 
LEXSRC = script.l
LEXGEN = lex.c
BISONSRC = script.y
BISONGEN = parser.c
BISONDEBUG = parser.out
CSRC = $(filter-out ./$(BISONGEN) ./$(LEXGEN),\
	$(shell find $(SUBDIRS) -mindepth 0 -maxdepth 1 -name "*.c"))

# put all libc-object-files into the archive
LIBCOBJ = $(shell find $(BUILD)/lib/c -mindepth 0 -maxdepth 5 -name "*.o")
COBJ = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
CPICOBJS = $(patsubst %.c,$(BUILDL)/%_pic.o,$(CSRC))
GENOBJ = $(patsubst %.c,$(BUILDL)/%.o,$(LEXGEN) $(BISONGEN))
PICGENOBJ = $(patsubst %.c,$(BUILDL)/%_pic.o,$(LEXGEN) $(BISONGEN))

.PHONY: all clean

all:	$(BUILDDIRS) $(BISONGEN) $(LEXGEN) $(STLIB) $(DYNLIB)

$(STLIB): $(COBJ) $(GENOBJ) $(LIBCOBJ)
		@echo "	" AR $(STLIB)
		@ar rcs $(STLIB) $(COBJ) $(GENOBJ) $(LIBCOBJ)

$(DYNLIB):	$(CPICOBJS) $(PICGENOBJ) $(SHLDCONF)
		@echo "	" LINKING $(DYNLIB)
		@$(CC) $(CFLAGS) $(DLNKFLAGS) -shared -Wl,--build-id=none -Wl,-T,$(SHLDCONF) \
			-Wl,-soname,$(DYNLIBNAME) -o $(DYNLIB) $(CPICOBJS) $(PICGENOBJ) -lc;
		$(ROOT)/tools/disk.sh copy $(DYNLIB) /lib/$(DYNLIBNAME)

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
		@$(CC) $(CFLAGS) -D YYERROR_VERBOSE -o $@ -c $< -MMD

$(BUILDL)/%_pic.o: %.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -D YYERROR_VERBOSE -fPIC -o $@ -c $< -MMD

-include $(DEPS)

clean:
		rm -f $(STLIB) $(DEPS) $(COBJ) $(CPICOBJS) $(LEXGEN) $(BISONGEN) $(GENOBJ) $(PICGENOBJ)
