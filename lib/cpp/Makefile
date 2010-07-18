ROOT = ../..
BUILDL = $(BUILD)/lib/cpp
SUBDIRS = $(filter-out $(shell find src -name "*.*"),$(shell find src -name "*"))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -name "*.d")
STLIB = $(BUILD)/libstdc++.a
DYNLIBNAME = libstdc++.so
DYNLIB = $(BUILD)/$(DYNLIBNAME)

CFLAGS = $(CPPDEFFLAGS)

# sources
CSRC = $(shell find src -name "*.cpp")

# objects
COBJS = $(patsubst %.cpp,$(BUILDL)/%.o,$(CSRC))
CPICOBJS = $(patsubst %.cpp,$(BUILDL)/%_pic.o,$(CSRC))

.PHONY:		all clean

all: $(BUILDDIRS) $(STLIB) $(DYNLIB)

$(STLIB): $(COBJS)
		@echo "	" AR $(STLIB)
		@ar rcs $(STLIB) $(COBJS)
		$(ROOT)/tools/linklib.sh $(STLIB)

$(DYNLIB): $(CPICOBJS)
		@echo "	" LINKING $(DYNLIB)
		$(CPPC) $(CFLAGS) -nodefaultlibs -shared -Wl,-soname,$(DYNLIBNAME) \
			-o $(DYNLIB) $(CPICOBJS) -lc -lsupc++ -lgcc -L$(ROOT)/build/dist/lib;
		$(ROOT)/tools/linklib.sh $(DYNLIB)
		$(ROOT)/tools/disk.sh copy $(DYNLIB) /lib/$(DYNLIBNAME)

$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/%.o: %.cpp
		@echo "	" CC $<
		@$(CPPC) $(CFLAGS) -o $@ -c $< -MD

$(BUILDL)/%_pic.o: %.cpp
		@echo "	" CC $<
		@$(CPPC) $(CFLAGS) -fPIC -o $@ -c $< -MD

-include $(DEPS)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(COBJS) $(CPICOBJS) $(STLIB) $(DYNLIB) $(DEPS)
