BUILDL = $(BUILD)/libc
LIBDIR = ../lib
GCCLIBDIR = ../libgcc
SUBDIRS = $(filter-out $(shell find src -name "*.*"),$(shell find src -name "*"))
LSUBDIRS = $(filter-out $(wildcard $(LIBDIR)/src/*.*),$(wildcard $(LIBDIR)/src/*))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS)) $(patsubst $(LIBDIR)/%,$(BUILDL)/lib/%,$(LSUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -name "*.d")
LIB = $(BUILD)/libc.a

CC = gcc
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs $(CDEFFLAGS) -Iinclude -I$(LIBDIR)/h

# sources
CSRC = $(shell find $(SUBDIRS) -name "*.c")
ASRC = $(filter-out startup.s,$(shell find $(SUBDIRS) -name "*.s"))
LSRC = $(shell find $(LIBDIR)/src -name "*.c")
START = startup.s
SYSCALLS = syscalls.s

# objects
COBJS = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
AOBJS = $(patsubst %.s,$(BUILDL)/%.s.o,$(ASRC))
LOBJ = $(patsubst $(LIBDIR)/%.c,$(BUILDL)/lib/%.o,$(LSRC))
STARTO = $(BUILD)/libc_startup.o
# for long long arithmetic
GCCLIB = $(GCCLIBDIR)/_divdi3.object \
	$(GCCLIBDIR)/_moddi3.object \
	$(GCCLIBDIR)/_udivdi3.object \
	$(GCCLIBDIR)/_umoddi3.object

.PHONY:		all clean

all: $(LIB)

$(LIB): $(BUILDDIRS) $(COBJS) $(AOBJS) $(LOBJ) $(STARTO) $(GCCLIB)
		@echo "	" AR $(LIB)
		@ar rcs $(LIB) $(COBJS) $(AOBJS) $(LOBJ) $(GCCLIB)
		
$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/%.o: %.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MMD

$(BUILDL)/%.s.o: %.s $(SYSCALLS)
		@echo "	" AS $<
		@nasm $(ASMFLAGS) -o $@ $<

$(BUILDL)/lib/%.o:	$(LIBDIR)/%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MMD

$(STARTO): $(START)
		@echo "	" AS $(START)
		@nasm $(ASMFLAGS) -o $(STARTO) $(START)

-include $(DEPS)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(COBJS) $(AOBJS) $(LOBJ) $(LIB) $(DEPS) $(STARTO)
