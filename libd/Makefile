BUILDL = $(BUILD)/libd
LIBDIR = ../lib
LIBCDIR = ../libc
SUBDIRS = $(filter-out Makefile $(shell find . -name "*.*"),$(shell find . -name "*"))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -name "*.d")
LIB = $(BUILD)/libd.a

CC = gcc
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs $(CDEFFLAGS) -Iinclude -I$(LIBDIR)/h \
	-I$(LIBCDIR)/include
DC = $(BUILD)/dmd

# sources
CSRC = $(shell find $(SUBDIRS) -name "*.c")
DSRC = $(shell find $(SUBDIRS) -name "*.d")
START = startup.s

# objects
COBJS = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
DOBJS = $(patsubst %.d,$(BUILDL)/%.o,$(DSRC))

# for long long arithmetic
GCCLIBDIR = ../libgcc
GCCLIB = $(GCCLIBDIR)/_divdi3.object \
	$(GCCLIBDIR)/_moddi3.object \
	$(GCCLIBDIR)/_udivdi3.object \
	$(GCCLIBDIR)/_umoddi3.object
LIBCOBJS = $(shell find $(BUILD)/libc -name "*.o") $(GCCLIB)
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
		@echo "	" CC $<
		@$(DC) $(DDEFFLAGS) -noboundscheck -c -of$@ $<

$(STARTO): $(START)
		@echo "	" AS $(START)
		@nasm $(ASMFLAGS) -o $(STARTO) $(START)

-include $(DEPS)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(COBJS) $(DOBJS) $(LIB) $(DEPS) $(STARTO)
