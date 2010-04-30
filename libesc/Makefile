BUILDL = $(BUILD)/libesc
LIBDIR = ../lib
LIBCDIR = ../libc
GCCLIBDIR = ../libgcc
SUBDIRS = $(filter-out $(shell find src -name "*.*"),$(shell find src -name "*"))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -name "*.d")
STLIB = $(BUILD)/libesc.a
DYNLIBNAME = libesc.so
DYNLIB = $(BUILD)/$(DYNLIBNAME)
SHLDCONF = $(LIBCDIR)/shld.conf

CC = gcc
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs $(CDEFFLAGS) -Iinclude -I$(LIBDIR)/h \
	-I$(LIBCDIR)/include

# sources
CSRC = $(shell find src -name "*.c")

# objects
COBJS = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
CPICOBJS = $(patsubst %.c,$(BUILDL)/%_pic.o,$(CSRC))
# for long long arithmetic
GCCLIB = $(GCCLIBDIR)/_divdi3.object \
	$(GCCLIBDIR)/_moddi3.object \
	$(GCCLIBDIR)/_udivdi3.object \
	$(GCCLIBDIR)/_umoddi3.object

.PHONY:		all clean

all: $(STLIB) $(DYNLIB)

$(STLIB): $(BUILDDIRS) $(COBJS) $(GCCLIB)
		@echo "	" AR $(STLIB)
		@ar rcs $(STLIB) $(COBJS) $(GCCLIB)

$(DYNLIB):	$(BUILDDIRS) $(CPICOBJS) $(GCCLIB) $(SHLDCONF)
		@echo "	" LINKING $(DYNLIB)
		@$(CC) $(CFLAGS) -shared -Wl,--build-id=none -Wl,-T,$(SHLDCONF) -Wl,-soname,$(DYNLIBNAME) \
			-o $(DYNLIB) $(CPICOBJS) $(GCCLIB);
		../tools/disk.sh copy $(DYNLIB) /lib/$(DYNLIBNAME)

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
-include $(DEPS)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(COBJS) $(CPICOBJS) $(STLIB) $(DYNLIB) $(DEPS)
