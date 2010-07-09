ROOT = ../..
BUILDL = $(BUILD)/lib/d
LIBDIR = $(ROOT)/lib/basic
LIBCDIR = $(ROOT)/lib/c
GCCLIBDIR = $(ROOT)/lib/gcc
SUBDIRS = $(filter-out Makefile $(shell find . -name "*.*"),$(shell find . -name "*"))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -name "*.d")
LIB = $(BUILD)/libd.a

CC = gcc
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs $(CDEFFLAGS) -I$(ROOT)/include
DC = dmd
DFLAGS = $(DDEFFLAGS) -version=Escape -I. -Itango/core -Itango/core/vendor \
	-Itango/core/rt/compiler/dmd

# sources
CSRC = $(shell find $(SUBDIRS) -name "*.c")
DSRC = $(shell find $(SUBDIRS) -name "*.d")
START = startup.s

# objects
COBJS = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
DOBJS = $(patsubst %.d,$(BUILDL)/%.o,$(DSRC))

# for long long arithmetic
GCCLIB = $(GCCLIBDIR)/_divdi3.object \
	$(GCCLIBDIR)/_moddi3.object \
	$(GCCLIBDIR)/_udivdi3.object \
	$(GCCLIBDIR)/_umoddi3.object
LIBCOBJS = $(shell find $(BUILD)/lib/c -name "*.o") $(GCCLIB)
STARTO = $(BUILD)/libd_startup.o

.PHONY:		all clean

all: $(LIB)

$(LIB): $(BUILDDIRS) $(COBJS) $(DOBJS) $(STARTO) $(LIBCOBJS)
		@echo "	" AR $(LIB)
		@ar rcs $(LIB) $(LIBCOBJS) $(COBJS) $(DOBJS)
		
$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/%.o: %.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -MMD -c -o $@ $<

$(BUILDL)/%.o: %.d
		@echo "	" DC $<
		@$(DC) $(DFLAGS) -c -of$@ $<

$(STARTO): $(START)
		@echo "	" AS $(START)
		@nasm $(ASMFLAGS) -o $(STARTO) $(START)

-include $(DEPS)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(COBJS) $(DOBJS) $(LIB) $(DEPS) $(STARTO)
