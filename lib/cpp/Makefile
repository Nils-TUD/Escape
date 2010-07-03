ROOT = ../..
BUILDL = $(BUILD)/lib/cpp
LIBDIR = $(ROOT)/lib/basic
LIBCDIR = $(ROOT)/lib/c
GCCLIBDIR = $(ROOT)/lib/gcc
SUBDIRS = $(filter-out $(shell find src -name "*.*"),$(shell find src -name "*"))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -name "*.d")
DYNLIBNAME = libcpp.so
DYNLIB = $(BUILD)/$(DYNLIBNAME)
SHLDCONF = ../shld.conf

CC = g++
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs $(CPPDEFFLAGS) -Iinclude -I$(LIBDIR)/h \
	-I$(LIBCDIR)/include

# sources
CSRC = $(shell find src -name "*.cpp")
START = startup.s

# objects
COBJS = $(patsubst %.cpp,$(BUILDL)/%.o,$(CSRC))
# for long long arithmetic
GCCLIB = $(GCCLIBDIR)/_divdi3.object \
	$(GCCLIBDIR)/_moddi3.object \
	$(GCCLIBDIR)/_udivdi3.object \
	$(GCCLIBDIR)/_umoddi3.object
LIBCOBJS = $(shell find $(BUILD)/lib/c -name "*_pic.o") \
	$(shell find $(BUILD)/lib/c -name "*_pic.s.o") \
	$(GCCLIB)
STARTO = $(BUILD)/libcpp_startup.o

.PHONY:		all clean

all: $(DYNLIB) $(STARTO)

$(DYNLIB): $(BUILDDIRS) $(COBJS) $(LIBCOBJS)
		@echo "	" LINKING $(DYNLIB)
		@$(CC) $(CFLAGS) -shared -Wl,--build-id=none -Wl,-T,$(SHLDCONF) -Wl,-soname,$(DYNLIBNAME) \
			-o $(DYNLIB) $(LIBCOBJS) $(COBJS);
		$(ROOT)/tools/disk.sh copy $(DYNLIB) /lib/$(DYNLIBNAME)
		
$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/%.o: %.cpp
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -fPIC -o $@ -c $< -MMD

$(STARTO): $(START)
		@echo "	" AS $(START)
		@nasm $(ASMFLAGS) -o $(STARTO) $(START)

-include $(DEPS)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(COBJS) $(DYNLIB) $(DEPS) $(STARTO)
