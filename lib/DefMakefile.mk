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
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs $(CDEFFLAGS) -I$(ROOT)/include

# sources 
CSRC = $(shell find $(SUBDIRS) -mindepth 0 -maxdepth 1 -name "*.c")

# put all libc-object-files into the archive
LIBCOBJ = $(shell find $(BUILD)/lib/c -mindepth 0 -maxdepth 5 -name "*.o")
COBJ = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
CPICOBJS = $(patsubst %.c,$(BUILDL)/%_pic.o,$(CSRC))

.PHONY: all clean

all:	$(BUILDDIRS) $(STLIB) $(DYNLIB)

$(STLIB): $(COBJ) $(LIBCOBJ)
		@echo "	" AR $(STLIB)
		@ar rcs $(STLIB) $(COBJ) $(LIBCOBJ)

$(DYNLIB):	$(CPICOBJS) $(SHLDCONF)
		@echo "	" LINKING $(DYNLIB)
		@$(CC) $(CFLAGS) -shared -Wl,--build-id=none -Wl,-T,$(SHLDCONF) -Wl,-soname,$(DYNLIBNAME) \
			-o $(DYNLIB) $(CPICOBJS);
		$(ROOT)/tools/disk.sh copy $(DYNLIB) /lib/$(DYNLIBNAME)

$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/%.o:		%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MMD

$(BUILDL)/%_pic.o: %.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -fPIC -o $@ -c $< -MMD

-include $(DEPS)

clean:
		rm -f $(STLIB) $(DYNLIB) $(DEPS) $(COBJ) $(CPICOBJS)
