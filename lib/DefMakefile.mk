ROOT = ../..
BUILDL = $(BUILD)/lib/$(NAME)
SUBDIRS = . $(filter-out Makefile $(wildcard *.*),$(wildcard *))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -mindepth 0 -maxdepth 1 -name "*.d")
STLIB = $(BUILD)/lib$(NAME).a
DYNLIBNAME = lib$(NAME).so
DYNLIB = $(BUILD)/$(DYNLIBNAME)

AR = $(ROOT)/build/dist/bin/i586-elf-escape-ar
CC = $(ROOT)/build/dist/bin/i586-elf-escape-gcc
LD = $(ROOT)/build/dist/bin/i586-elf-escape-ld
CFLAGS = $(CDEFFLAGS)

CSRC = $(shell find $(SUBDIRS) -mindepth 0 -maxdepth 1 -name "*.c")
COBJ = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
CPICOBJS = $(patsubst %.c,$(BUILDL)/%_pic.o,$(CSRC))

.PHONY: all clean

all:	$(BUILDDIRS) $(STLIB) $(DYNLIB)

$(STLIB): $(COBJ)
		@echo "	" AR $(STLIB)
		@$(AR) rcs $(STLIB) $(COBJ)

$(DYNLIB):	$(CPICOBJS)
		@echo "	" LINKING $(DYNLIB)
		@$(LD) -shared -soname $(DYNLIBNAME) -o $(DYNLIB) $(CPICOBJS)
		$(ROOT)/tools/disk.sh copy $(DYNLIB) /lib/$(DYNLIBNAME)

$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/%.o:		%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MD

$(BUILDL)/%_pic.o: %.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -fPIC -o $@ -c $< -MD

-include $(DEPS)

clean:
		rm -f $(STLIB) $(DYNLIB) $(DEPS) $(COBJ) $(CPICOBJS)
