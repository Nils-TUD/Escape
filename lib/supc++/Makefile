ROOT = ../..
BUILDL = $(BUILD)/lib/supc++
LIBCDIR = $(ROOT)/lib/c
SUBDIRS = .
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -name "*.d")
LIB = $(BUILD)/libsupc++.a

CPP = g++
CPPFLAGS = -nostdlib -nostartfiles -nodefaultlibs $(CPPDEFFLAGS) \
	-I. -I$(ROOT)/include

# sources
CPPSRC = $(shell find . -name "*.cc")

# objects
CPPOBJS = $(patsubst %.cc,$(BUILDL)/%.o,$(CPPSRC))
LIBCOBJS = $(filter-out $(shell find $(BUILD)/lib/c -name "*_pic*"), \
	$(shell find $(BUILD)/lib/c -name "*.o"))

.PHONY:		all clean

all: $(LIB)

$(LIB): $(BUILDDIRS) $(CPPOBJS) $(LIBCOBJS)
		@echo "	" AR $(LIB)
		@ar rcs $(LIB) $(CPPOBJS) $(LIBCOBJS)

$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/%.o: %.cc
		@echo "	" CC $<
		@$(CPP) $(CPPFLAGS) -o $@ -c $< -MMD

-include $(DEPS)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(CPPOBJS) $(LIB) $(DEPS)
