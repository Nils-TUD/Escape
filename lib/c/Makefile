ROOT = ../..
BUILDL = $(BUILD)/lib/c
LIBDIR = $(ROOT)/lib/basic
GCCLIBDIR = $(ROOT)/lib/gcc
SUBDIRS = $(filter-out $(shell find src -name "*.*"),$(shell find src -name "*"))
LSUBDIRS = $(filter-out $(wildcard $(LIBDIR)/src/*.*),$(wildcard $(LIBDIR)/src/*))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS)) $(patsubst $(LIBDIR)/%,$(BUILDL)/lib/%,$(LSUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -name "*.d")
STLIB = $(BUILD)/libc.a
DYNLIBNAME = libc.so
DYNLIB = $(BUILD)/$(DYNLIBNAME)
SHLDCONF = ../shld.conf

CC = gcc
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs $(CDEFFLAGS) -I$(ROOT)/include

# sources
CSRC = $(shell find src -name "*.c")
ASRC = $(filter-out startup.s,$(shell find src -name "*.s"))
LSRC = $(shell find $(LIBDIR)/src -name "*.c")
START = startup.s
SYSCALLS = syscalls.s

# objects
COBJS = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
CPICOBJS = $(patsubst %.c,$(BUILDL)/%_pic.o,$(CSRC))
AOBJS = $(patsubst %.s,$(BUILDL)/%.s.o,$(ASRC))
APICOBJS = $(patsubst %.s,$(BUILDL)/%_pic.s.o,$(ASRC))
LOBJ = $(patsubst $(LIBDIR)/%.c,$(BUILDL)/lib/%.o,$(LSRC))
LPICOBJ = $(patsubst $(LIBDIR)/%.c,$(BUILDL)/lib/%_pic.o,$(LSRC))
STARTO = $(BUILD)/libc_startup.o
# for long long arithmetic
GCCLIB = $(GCCLIBDIR)/_divdi3.object \
	$(GCCLIBDIR)/_moddi3.object \
	$(GCCLIBDIR)/_udivdi3.object \
	$(GCCLIBDIR)/_umoddi3.object

.PHONY:		all clean

all: $(STLIB) $(DYNLIB) $(STARTO)

$(STLIB): $(BUILDDIRS) $(COBJS) $(AOBJS) $(LOBJ) $(GCCLIB)
		@echo "	" AR $(STLIB)
		@ar rcs $(STLIB) $(COBJS) $(AOBJS) $(LOBJ) $(GCCLIB)

$(DYNLIB):	$(BUILDDIRS) $(CPICOBJS) $(APICOBJS) $(LPICOBJ) $(GCCLIB) $(SHLDCONF)
		@echo "	" LINKING $(DYNLIB)
		@$(CC) $(CFLAGS) -shared -Wl,--build-id=none -Wl,-T,$(SHLDCONF) -Wl,-soname,$(DYNLIBNAME) \
			-o $(DYNLIB) $(CPICOBJS) $(APICOBJS) $(LPICOBJ) $(GCCLIB);
		$(ROOT)/tools/disk.sh copy $(DYNLIB) /lib/$(DYNLIBNAME)
		
$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/%.o: %.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MMD

$(BUILDL)/%_pic.o: %.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -fPIC -o $@ -c $< -MMD

$(BUILDL)/%_pic.s.o: %.s $(SYSCALLS)
		@echo "	" AS $<
		@nasm $(ASMFLAGS) -D SHAREDLIB -o $@ $<

$(BUILDL)/%.s.o: %.s $(SYSCALLS)
		@echo "	" AS $<
		@nasm $(ASMFLAGS) -o $@ $<

$(BUILDL)/lib/%.o:	$(LIBDIR)/%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MMD

$(BUILDL)/lib/%_pic.o:	$(LIBDIR)/%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -fPIC -o $@ -c $< -MMD

$(STARTO): $(START)
		@echo "	" AS $(START)
		@nasm $(ASMFLAGS) -o $(STARTO) $(START)

-include $(DEPS)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(COBJS) $(CPICOBJS) $(AOBJS) $(APICOBJS) $(LOBJ) $(LPICOBJ) $(STLIB) \
			$(DEPS) $(STARTO) $(DYNLIB)
