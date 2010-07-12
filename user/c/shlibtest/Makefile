NAME=shlibtest
ROOT = ../../..
BUILDL = $(BUILD)/user/c/$(NAME)
BIN = $(BUILD)/user_$(NAME).bin
SHLIBNAME = lib$(NAME).so
SECLIBNAME = lib$(NAME)2.so
SHLIB = $(BUILD)/$(SHLIBNAME)
SECLIB = $(BUILD)/$(SECLIBNAME)
LIBC = $(ROOT)/lib/c
LDCONF = $(ROOT)/lib/ld.conf
SHLDCONF = $(ROOT)/lib/shld.conf
SUBDIRS = . $(filter-out Makefile $(wildcard *.*),$(wildcard *))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -mindepth 0 -maxdepth 1 -name "*.d")

CC = gcc
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs -I$(ROOT)/include \
	-Wl,--build-id=none $(CDEFFLAGS) $(ADDFLAGS)

# sources
LIBSRC = shltlib.c
SECLIBSRC = seclib.c
CSRC = shltmain.c

# objects
LIBCA = $(BUILD)/libc.a
START = $(BUILD)/libc_startup.o
LIBOBJ = $(patsubst %.c,$(BUILDL)/lib%.o,$(LIBSRC))
SECLIBOBJ = $(patsubst %.c,$(BUILDL)/lib2%.o,$(SECLIBSRC))
COBJ = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))

.PHONY: all clean

all:	$(APPCPY) $(BIN)

$(BIN):	$(BUILDDIRS) $(LDCONF) $(COBJ) $(START) $(LIBCA) $(SHLIB) $(SECLIB)
		@echo "	" LINKING $(BIN)
		@$(CC) $(CFLAGS) $(DLNKFLAGS) -Wl,-T,$(LDCONF) -o $(BIN) -l$(NAME) -l$(NAME)2 \
			$(START) $(COBJ) $(LIBCA);
		@echo "	" COPYING ON DISK
		$(ROOT)/tools/disk.sh copy $(BIN) /bin/$(NAME)

$(SECLIB): $(SECLIBOBJ) $(SHLDCONF)
		@echo "	" LINKING $(SECSHLIB)
		@$(CC) $(CFLAGS) -Wl,-T,$(SHLDCONF) -shared -Wl,-soname,$(SECLIBNAME) -o $(SECLIB) \
			$(SECLIBOBJ) $(LIBCA);
		@echo "	" COPYING ON DISK
		$(ROOT)/tools/disk.sh copy $(SECLIB) /lib/$(SECLIBNAME)

$(SHLIB): $(LIBOBJ) $(SHLDCONF)
		@echo "	" LINKING $(SHLIB)
		@$(CC) $(CFLAGS) -Wl,-T,$(SHLDCONF) -shared -Wl,-soname,$(SHLIBNAME) -o $(SHLIB) $(LIBOBJ) $(LIBCA);
		@echo "	" COPYING ON DISK
		$(ROOT)/tools/disk.sh copy $(SHLIB) /lib/$(SHLIBNAME)

$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/lib2%.o:		%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -fPIC -o $@ -c $< -MD

$(BUILDL)/lib%.o:		%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -fPIC -o $@ -c $< -MD

$(BUILDL)/%.o:		%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MD

-include $(DEPS)

clean:
		rm -f $(BIN) $(SECLIB) $(SHLIB) $(COBJ) $(SECLIBOBJ) $(LIBOBJ) $(DEPS)
