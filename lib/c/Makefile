ROOT = ../..
BUILDL = $(BUILD)/lib/c
LIBDIR = $(ROOT)/lib/basic
SUBDIRS = $(filter-out $(shell find . -name "*.*"),$(shell find . -name "*"))
LSUBDIRS = $(filter-out $(wildcard $(LIBDIR)/*.*),$(wildcard $(LIBDIR)/*))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS)) $(patsubst $(LIBDIR)/%,$(BUILDL)/lib/%,$(LSUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -name "*.d")
STLIB = $(BUILD)/libc.a
DYNLIBNAME = libc.so
DYNLIB = $(BUILD)/$(DYNLIBNAME)

CFLAGS = -nostdlib -nostartfiles -nodefaultlibs $(CDEFFLAGS)

# sources
START = startup.s
SYSCALLS = syscalls.s
CSRC = $(shell find . -name "*.c")
ASRC = $(filter-out ./$(START),$(shell find . -name "*.s"))
LSRC = $(shell find $(LIBDIR) -name "*.c")

# objects
STARTO = $(BUILD)/libc_startup.o
COBJS = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
CPICOBJS = $(patsubst %.c,$(BUILDL)/%_pic.o,$(CSRC))
AOBJS = $(patsubst %.s,$(BUILDL)/%.s.o,$(ASRC))
APICOBJS = $(patsubst %.s,$(BUILDL)/%_pic.s.o,$(ASRC))
LOBJ = $(patsubst $(LIBDIR)/%.c,$(BUILDL)/lib/%.o,$(LSRC))
LPICOBJ = $(patsubst $(LIBDIR)/%.c,$(BUILDL)/lib/%_pic.o,$(LSRC))

.PHONY:		all clean

all: $(BUILDDIRS) $(STLIB) $(DYNLIB) $(STARTO)

$(STLIB): $(COBJS) $(AOBJS) $(LOBJ)
		@echo "	" AR $(STLIB)
		@$(AR) rcs $(STLIB) $(COBJS) $(AOBJS) $(LOBJ)

$(DYNLIB): $(CPICOBJS) $(APICOBJS) $(LPICOBJ)
		@echo "	" LINKING $(DYNLIB)
		@$(LD) -shared -soname $(DYNLIBNAME) -o $(DYNLIB) \
			$(CPICOBJS) $(APICOBJS) $(LPICOBJ)
		$(ROOT)/tools/disk.sh copy $(DYNLIB) /lib/$(DYNLIBNAME)
		
$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/%.o: %.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MD

$(BUILDL)/%_pic.o: %.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -fPIC -o $@ -c $< -MD

$(BUILDL)/%_pic.s.o: %.s $(SYSCALLS)
		@echo "	" AS $<
		@nasm $(ASMFLAGS) -D SHAREDLIB -o $@ $<

$(BUILDL)/%.s.o: %.s $(SYSCALLS)
		@echo "	" AS $<
		@nasm $(ASMFLAGS) -o $@ $<

$(BUILDL)/lib/%.o:	$(LIBDIR)/%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MD

$(BUILDL)/lib/%_pic.o:	$(LIBDIR)/%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -fPIC -o $@ -c $< -MD

$(STARTO): $(START)
		@echo "	" AS $(START)
		@nasm $(ASMFLAGS) -o $(STARTO) $(START)

-include $(DEPS)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(COBJS) $(CPICOBJS) $(AOBJS) $(APICOBJS) $(LOBJ) $(LPICOBJ) $(STLIB) \
			$(DEPS) $(STARTO) $(DYNLIB)
