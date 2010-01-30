NAME = shell
ROOT = ../../..
BUILDL = $(BUILD)/user/libs/$(NAME)
BIN = $(BUILD)/user_libs_$(NAME).a
LIB = $(ROOT)/lib
LIBC = $(ROOT)/libc
SUBDIRS = . $(filter-out Makefile $(wildcard *.*),$(wildcard *))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -mindepth 0 -maxdepth 1 -name "*.d")

CC = gcc
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs -I$(LIBC)/include -I$(LIB)/h \
	-Wl,-T,$(LDCONF) $(CDEFFLAGS)

# sources 
LEXSRC = script.l
LEXGEN = lex.c
BISONSRC = script.y
BISONGEN = parser.c
CSRC = $(filter-out ./$(BISONGEN) ./$(LEXGEN),\
	$(shell find $(SUBDIRS) -mindepth 0 -maxdepth 1 -name "*.c"))

# put all libc-object-files into the archive
LIBCOBJ = $(shell find $(BUILD)/libc -mindepth 0 -maxdepth 5 -name "*.o")
COBJ = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
GENOBJ = $(patsubst %.c,$(BUILDL)/%.o,$(LEXGEN) $(BISONGEN))

.PHONY: all clean

all:	$(BISONGEN) $(LEXGEN) $(BIN)

$(BIN): $(BUILDDIRS) $(COBJ) $(GENOBJ) $(LIBCOBJ)
		@echo "	" AR $(BIN)
		@ar rcs $(BIN) $(COBJ) $(GENOBJ) $(LIBCOBJ)

$(LEXGEN):	$(LEXSRC)
		flex -o $(LEXGEN) --nounistd < $(LEXSRC)

$(BISONGEN):	$(BISONSRC)
		bison -t -d -o $(BISONGEN) $(BISONSRC)

$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/%.o:		%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -D YYERROR_VERBOSE -o $@ -c $< -MMD

-include $(DEPS)

clean:
		rm -f $(BIN) $(DEPS) $(COBJ) $(LEXGEN) $(BISONGEN) $(GENOBJ)
