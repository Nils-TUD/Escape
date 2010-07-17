ROOT = ../../..
BUILDL = $(BUILD)/user/cpp/$(NAME)
BIN = $(BUILD)/user_$(NAME).bin
LIBC = $(ROOT)/lib/c
LIBCPP = $(ROOT)/lib/cpp
SUBDIRS = . $(filter-out Makefile $(wildcard *.*),$(wildcard *))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -mindepth 0 -maxdepth 1 -name "*.d")

CFLAGS = $(CPPDEFFLAGS) $(ADDFLAGS)
ifeq ($(LINKTYPE),static)
	CFLAGS += -Wl,-Bstatic
endif

# sources
CSRC = $(shell find $(SUBDIRS) -mindepth 0 -maxdepth 1 -name "*.cpp")

# objects
COBJ = $(patsubst %.cpp,$(BUILDL)/%.o,$(CSRC))

.PHONY: all clean

all:	$(BUILDDIRS) $(BIN)

$(BIN):	$(COBJ)
		@echo "	" LINKING $(BIN)
		@$(CPPC) $(CFLAGS) -L$(ROOT)/build/dist/lib -o $(BIN) $(COBJ) -lsupc++;
		@echo "	" COPYING ON DISK
		$(ROOT)/tools/disk.sh copy $(BIN) /bin/$(NAME)

$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/%.o:		%.cpp
		@echo "	" CC $<
		@$(CPPC) $(CFLAGS) -o $@ -c $< -MD

-include $(DEPS)

clean:
		rm -f $(BIN) $(COBJ) $(DEPS)
