BUILDL = $(BUILD)/libsupc++
LIBDIR = ../lib
LIBCDIR = ../libc
SUBDIRS = . unwind
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -name "*.d")
LIB = $(BUILD)/libsupc++.a

CPP = g++
CPPFLAGS = -nostdlib -nostartfiles -nodefaultlibs $(CPPDEFFLAGS) -I.

# sources
CPPSRC = $(shell find . -name "*.cc")

# objects
CPPOBJS = $(patsubst %.cc,$(BUILDL)/%.o,$(CPPSRC))

.PHONY:		all clean

all: $(LIB)

$(LIB): $(BUILDDIRS) $(CPPOBJS)
		@echo "	" AR $(LIB)
		@ar rcs $(LIB) $(CPPOBJS)

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
