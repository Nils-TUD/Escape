ROOT = ../..
BUILDL = $(BUILD)/lib/$(NAME)
SUBDIRS = $(shell find . -type d | grep -v '\.svn')
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDL) -name "*.d")
STLIB = $(BUILD)/lib$(NAME).a

ifneq ($(LINKTYPE),static)
	DYNLIBNAME = lib$(NAME).so
	DYNLIB = $(BUILD)/$(DYNLIBNAME)
endif

CFLAGS = $(CDEFFLAGS) $(ADDFLAGS)

CSRC = $(shell find $(SUBDIRS) -name "*.c")
COBJ = $(patsubst %.c,$(BUILDL)/%.o,$(CSRC))
CPICOBJS = $(patsubst %.c,$(BUILDL)/%_pic.o,$(CSRC))

.PHONY: all clean

all:	$(BUILDDIRS) $(STLIB) $(DYNLIB)

$(STLIB): $(COBJ)
	@echo "	" AR $(STLIB)
	@$(AR) rcs $(STLIB) $(COBJ)
	@$(ROOT)/tools/linklib.sh $(STLIB)

$(DYNLIB):	$(CPICOBJS)
	@echo "	" LINKING $(DYNLIB)
	@$(CC) $(CFLAGS) -shared -Wl,-shared -Wl,-soname,$(DYNLIBNAME) -o $(DYNLIB) \
		$(CPICOBJS) $(ADDLIBS)
	@$(ROOT)/tools/linklib.sh $(DYNLIB)

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
