ROOT = ../../..
BUILDL = $(BUILD)/user/cpp/$(NAME)
BIN = $(BUILD)/user_$(NAME).bin
MAP = $(BUILD)/user_$(NAME).map
LIBC = $(ROOT)/lib/c
LIBCPP = $(ROOT)/lib/cpp
SUBDIRS = $(shell find . -type d | grep -v '\.svn')
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDL) -name "*.d")

LIBDEPS += $(BUILD)/libc.a $(BUILD)/libg.a $(BUILD)/libm.a $(BUILD)/libstdc++.a
CFLAGS = $(CPPDEFFLAGS) $(ADDFLAGS)
ifeq ($(LINKTYPE),static)
	CFLAGS += -static -Wl,-Bstatic
endif

# sources
CSRC = $(shell find . -name "*.cpp")

# objects
COBJ = $(patsubst %.cpp,$(BUILDL)/%.o,$(CSRC))

.PHONY: all clean

-include $(ROOT)/sysdeps.mk

all:	$(BUILDDIRS) $(BIN) $(MAP)

$(BIN):	$(DEP_START) $(DEP_DEFLIBS) $(COBJ) $(LIBDEPS)
	@echo "	" LINKING $(BIN)
	@$(CPPC) $(CFLAGS) -o $(BIN) $(COBJ) $(ADDLIBS);

$(MAP): $(BIN)
	@echo "	" GEN MAP $@
	@$(ROOT)/tools/createmap-$(ARCH) $(BIN) > $@

$(BUILDDIRS):
	@for i in $(BUILDDIRS); do \
		if [ ! -d $$i ]; then mkdir -p $$i; fi \
	done;

$(BUILDL)/%.o:		%.cpp
	@echo "	" CC $<
	@$(CPPC) $(CFLAGS) -o $@ -c $< -MD

-include $(DEPS)

clean:
	rm -f $(BIN) $(MAP) $(COBJ) $(DEPS)
