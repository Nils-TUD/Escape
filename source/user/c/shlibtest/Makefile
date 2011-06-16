NAME=shlibtest
ROOT = ../../..
BUILDL = $(BUILD)/user/c/$(NAME)
BIN = $(BUILD)/user_$(NAME).bin
SHLIBNAME = lib$(NAME).so
SECLIBNAME = lib$(NAME)2.so
SHLIB = $(BUILD)/$(SHLIBNAME)
SECLIB = $(BUILD)/$(SECLIBNAME)
SUBDIRS = . $(filter-out Makefile $(wildcard *.*),$(wildcard *))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -mindepth 0 -maxdepth 1 -name "*.d")

CFLAGS = $(CDEFFLAGS) $(ADDFLAGS)

# sources
LIBSRC = shltlib.c
SECLIBSRC = seclib.c
CSRC = shltmain.c

# objects
LIBOBJ = $(patsubst %.c,$(BUILDL)/lib%.o,$(LIBSRC))
SECLIBOBJ = $(patsubst %.c,$(BUILDL)/lib2%.o,$(SECLIBSRC))
COBJ = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))

.PHONY: all clean

-include $(ROOT)/sysdeps.mk

all:	$(BUILDDIRS) $(APPCPY) $(BIN)

$(BIN):	$(DEP_START) $(DEP_DEFLIBS) $(COBJ) $(SHLIB) $(SECLIB)
	@echo "	" LINKING $(BIN)
	@$(CC) $(CFLAGS) -o $(BIN) -l$(NAME) -l$(NAME)2 $(COBJ);

$(SECLIB): $(DEP_START) $(DEP_DEFLIBS) $(SECLIBOBJ)
	@echo "	" LINKING $(SECSHLIB)
	@$(LD) -shared -soname $(SECLIBNAME) -o $(SECLIB) $(SECLIBOBJ);
	@$(ROOT)/tools/linklib.sh $(SECLIB)

$(SHLIB): $(DEP_START) $(DEP_DEFLIBS) $(LIBOBJ)
	@echo "	" LINKING $(SHLIB)
	@$(LD) -shared -soname $(SHLIBNAME) -o $(SHLIB) $(LIBOBJ);
	@$(ROOT)/tools/linklib.sh $(SHLIB)

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
