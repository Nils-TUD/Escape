ROOT = ../..
BUILDL = $(BUILD)/lib/c
LIBDIR = $(ROOT)/lib/basic
SUBDIRS = $(shell find . -type d | grep -v '\.svn')
LSUBDIRS = $(shell find $(LIBDIR) -type d | grep -v '\.svn')
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS)) $(patsubst $(LIBDIR)/%,$(BUILDL)/lib/%,$(LSUBDIRS))
DEPS = $(shell find $(BUILDL) -name "*.d")
STLIB = $(BUILD)/libc.a

ifneq ($(LINKTYPE),static)
	DYNLIBNAME = libc.so
	DYNLIB = $(BUILD)/$(DYNLIBNAME)
endif

CFLAGS = -nodefaultlibs $(CDEFFLAGS)
ifeq ($(LIBGCC),static)
	ADDLIBS = -lgcc 
endif

# sources
START = $(wildcard ./arch/$(ARCH)/crt*.s)
SYSCALLS = ./arch/$(ARCH)/syscalls.s
CSRC = $(filter-out $(shell find ./arch -name "*.c"),$(shell find . -name "*.c"))
CSRC += $(shell find ./arch/$(ARCH) -name "*.c")
ASRC = $(filter-out $(shell find ./arch -name "*.s"),$(shell find . -name "*.s"))
ASRC += $(filter-out $(SYSCALLS) $(START),$(shell find ./arch/$(ARCH) -name "*.s"))
LSRC = $(filter-out $(shell find $(LIBDIR)/arch -name "*.c"),$(shell find $(LIBDIR) -name "*.c"))
LSRC += $(shell find $(LIBDIR)/arch/$(ARCH) -name "*.c")
LASRC = $(shell find $(LIBDIR)/arch/$(ARCH) -name "*.s")

# objects
STARTO = $(patsubst ./arch/$(ARCH)/%.s,$(BUILD)/%.o,$(START))
STARTOPIC = $(patsubst ./arch/$(ARCH)/%.s,$(BUILD)/%S.o,$(START))
COBJS = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
CPICOBJS = $(patsubst %.c,$(BUILDL)/%_pic.o,$(CSRC))
AOBJS = $(patsubst %.s,$(BUILDL)/%.s.o,$(ASRC))
APICOBJS = $(patsubst %.s,$(BUILDL)/%_pic.s.o,$(ASRC))
LOBJ = $(patsubst $(LIBDIR)/%.c,$(BUILDL)/lib/%.o,$(LSRC))
LPICOBJ = $(patsubst $(LIBDIR)/%.c,$(BUILDL)/lib/%_pic.o,$(LSRC))
LAOBJS = $(patsubst $(LIBDIR)/%.s,$(BUILDL)/lib/%.s.o,$(LASRC))
LAPICOBJS = $(patsubst $(LIBDIR)/%.s,$(BUILDL)/lib/%_pic.s.o,$(LASRC))

.PHONY:		all clean

all: $(BUILDDIRS) $(STARTO) $(STARTOPIC) $(STLIB) $(DYNLIB)

$(STLIB): $(COBJS) $(AOBJS) $(LOBJ) $(LAOBJS)
		@echo "	" AR $(STLIB)
		@$(AR) rcs $(STLIB) $(COBJS) $(AOBJS) $(LOBJ) $(LAOBJS)
		@$(ROOT)/tools/linklib.sh $(STLIB)

$(DYNLIB): $(CPICOBJS) $(APICOBJS) $(LPICOBJ) $(LAPICOBJS)
		@echo "	" LINKING $(DYNLIB)
		@$(CC) $(CFLAGS) -shared -Wl,-shared -Wl,-soname,$(DYNLIBNAME) -o $(DYNLIB) $(CPICOBJS) \
			$(APICOBJS) $(LPICOBJ) $(LAPICOBJS) $(ADDLIBS)
		@$(ROOT)/tools/linklib.sh $(DYNLIB)
		
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

$(BUILDL)/lib/%.o:	$(LIBDIR)/%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MD

$(BUILDL)/lib/%_pic.o:	$(LIBDIR)/%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -fPIC -o $@ -c $< -MD

$(BUILDL)/%_pic.s.o: %.s
		@echo "	" CPP $<
		@$(CPP) -MD -MT $@ -MF $@.d $< > $@.tmp
		@echo "	" AS $<
		@$(AS) $(ASFLAGS) --defsym SHAREDLIB=1 -o $@ $@.tmp
		@rm -f $@.tmp

$(BUILDL)/%.s.o: %.s
		@echo "	" CPP $<
		@$(CPP) -MD -MT $@ -MF $@.d $< > $@.tmp
		@echo "	" AS $<
		@$(AS) $(ASFLAGS) -o $@ $@.tmp
		@rm -f $@.tmp

$(BUILDL)/lib/%_pic.s.o: $(LIBDIR)/%.s
		@echo "	" CPP $<
		@$(CPP) -MD -MT $@ -MF $@.d $< > $@.tmp
		@echo "	" AS $<
		@$(AS) $(ASFLAGS) --defsym SHAREDLIB=1 -o $@ $@.tmp
		@rm -f $@.tmp

$(BUILDL)/lib/%.s.o: $(LIBDIR)/%.s
		@echo "	" CPP $<
		@$(CPP) -MD -MT $@ -MF $@.d $< > $@.tmp
		@echo "	" AS $<
		@$(AS) $(ASFLAGS) -o $@ $@.tmp
		@rm -f $@.tmp

$(BUILD)/crt%S.o : arch/$(ARCH)/crt%.s
		@echo "	" CPP $<
		@$(CPP) -MD -MT $@ -MF $(patsubst $(BUILD)/%.o,$(BUILDL)/%.o.d,$@) $< > $@.tmp
		@echo "	" AS $<
		@$(AS) $(ASFLAGS) -defsym SHAREDLIB=1 -o $@ $@.tmp
		@$(ROOT)/tools/linklib.sh $@
		@rm -f $@.tmp

$(BUILD)/crt%.o : arch/$(ARCH)/crt%.s
		@echo "	" CPP $<
		@$(CPP) -MD -MT $@ -MF $(patsubst $(BUILD)/%.o,$(BUILDL)/%.o.d,$@) $< > $@.tmp
		@echo "	" AS $<
		@$(AS) $(ASFLAGS) -o $@ $@.tmp
		@$(ROOT)/tools/linklib.sh $@
		@rm -f $@.tmp

-include $(DEPS)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(COBJS) $(CPICOBJS) $(AOBJS) $(APICOBJS) $(LOBJ) $(LPICOBJ) $(LAOBJS) $(LAPICOBJS) \
			$(STLIB) $(DEPS) $(STARTO) $(STARTOPIC) $(DYNLIB)
