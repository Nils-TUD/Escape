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

CFLAGS = -nodefaultlibs $(CDEFFLAGS)
ifeq ($(LIBGCC),static)
	ADDLIBS = -lgcc 
endif

# sources
START = $(wildcard crt*.s)
SYSCALLS = syscalls.s
CSRC = $(shell find . -name "*.c")
ASRC = $(filter-out $(addprefix ./,$(START)),$(shell find . -name "*.s"))
LSRC = $(shell find $(LIBDIR) -name "*.c")

# objects
STARTO = $(patsubst %.s,$(BUILD)/%.o,$(START))
STARTOPIC = $(patsubst %.s,$(BUILD)/%S.o,$(START))
COBJS = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
CPICOBJS = $(patsubst %.c,$(BUILDL)/%_pic.o,$(CSRC))
AOBJS = $(patsubst %.s,$(BUILDL)/%.s.o,$(ASRC))
APICOBJS = $(patsubst %.s,$(BUILDL)/%_pic.s.o,$(ASRC))
LOBJ = $(patsubst $(LIBDIR)/%.c,$(BUILDL)/lib/%.o,$(LSRC))
LPICOBJ = $(patsubst $(LIBDIR)/%.c,$(BUILDL)/lib/%_pic.o,$(LSRC))

.PHONY:		all clean

all: $(BUILDDIRS) $(STARTO) $(STARTOPIC) $(STLIB) $(DYNLIB)

$(STLIB): $(COBJS) $(AOBJS) $(LOBJ)
		@echo "	" AR $(STLIB)
		@$(AR) rcs $(STLIB) $(COBJS) $(AOBJS) $(LOBJ)
		@$(ROOT)/tools/linklib.sh $(STLIB)

$(DYNLIB): $(CPICOBJS) $(APICOBJS) $(LPICOBJ)
		@echo "	" LINKING $(DYNLIB)
		@$(CC) $(CFLAGS) -shared -Wl,-shared -Wl,-soname,$(DYNLIBNAME) -o $(DYNLIB) $(CPICOBJS) \
			$(APICOBJS) $(LPICOBJ) $(ADDLIBS)
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

$(BUILDL)/%_pic.s.o: %.s $(SYSCALLS)
		@echo "	" AS $<
		@$(AS) $(ASFLAGS) --defsym SHAREDLIB=1 -o $@ $<

$(BUILDL)/%.s.o: %.s $(SYSCALLS)
		@echo "	" AS $<
		@$(AS) $(ASFLAGS) -o $@ $<

$(BUILDL)/lib/%.o:	$(LIBDIR)/%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MD

$(BUILDL)/lib/%_pic.o:	$(LIBDIR)/%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -fPIC -o $@ -c $< -MD

$(BUILD)/crt%S.o : crt%.s
		@echo "	" AS $<
		@$(AS) $(ASFLAGS) -defsym SHAREDLIB=1 -o $@ $<
		@$(ROOT)/tools/linklib.sh $@

$(BUILD)/crt%.o : crt%.s
		@echo "	" AS $<
		@$(AS) $(ASFLAGS) -o $@ $<
		@$(ROOT)/tools/linklib.sh $@

-include $(DEPS)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(COBJS) $(CPICOBJS) $(AOBJS) $(APICOBJS) $(LOBJ) $(LPICOBJ) $(STLIB) \
			$(DEPS) $(STARTO) $(STARTOPIC) $(DYNLIB)
