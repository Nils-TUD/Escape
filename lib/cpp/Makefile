ROOT = ../..
BUILDL = $(BUILD)/lib/cpp
LIBCDIR = $(ROOT)/lib/c
GCCLIBDIR = $(ROOT)/lib/gcc
SUBDIRS = $(filter-out $(shell find src -name "*.*"),$(shell find src -name "*"))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -name "*.d")
STLIB = $(BUILD)/libcpp.a
DYNLIBNAME = libcpp.so
DYNLIB = $(BUILD)/$(DYNLIBNAME)
SHLDCONF = ../shld.conf

CC = g++
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs $(CPPDEFFLAGS) -I$(ROOT)/include \
	-I$(ROOT)/include/cpp

# sources
CSRC = $(shell find src -name "*.cpp")
START = startup.s

# objects
COBJS = $(patsubst %.cpp,$(BUILDL)/%.o,$(CSRC))
CPICOBJS = $(patsubst %.cpp,$(BUILDL)/%_pic.o,$(CSRC))
# for long long arithmetic
GCCLIB = $(GCCLIBDIR)/_divdi3.object \
	$(GCCLIBDIR)/_moddi3.object \
	$(GCCLIBDIR)/_udivdi3.object \
	$(GCCLIBDIR)/_umoddi3.object
LIBCPICOBJS = $(shell find $(BUILD)/lib/c -name "*_pic.o") \
	$(shell find $(BUILD)/lib/c -name "*_pic.s.o") \
	$(GCCLIB)
LIBCOBJS = $(filter-out $(LIBCPICOBJS),$(shell find $(BUILD)/lib/c -name "*.o") \
	$(shell find $(BUILD)/lib/c -name "*.s.o") \
	$(GCCLIB))
STARTO = $(BUILD)/libcpp_startup.o

.PHONY:		all clean

all: $(BUILDDIRS) $(STLIB) $(DYNLIB) $(STARTO)

$(STLIB): $(COBJS) $(LIBCOBJS)
		@echo "	" AR $(STLIB)
		@ar rcs $(STLIB) $(LIBCOBJS) $(COBJS)

$(DYNLIB): $(CPICOBJS) $(LIBCPICOBJS)
		@echo "	" LINKING $(DYNLIB)
		@$(CC) $(CFLAGS) -shared -Wl,--build-id=none -Wl,-T,$(SHLDCONF) -Wl,-soname,$(DYNLIBNAME) \
			-o $(DYNLIB) $(LIBCPICOBJS) $(CPICOBJS);
		$(ROOT)/tools/disk.sh copy $(DYNLIB) /lib/$(DYNLIBNAME)
		
$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/%.o: %.cpp
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MD

$(BUILDL)/%_pic.o: %.cpp
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -fPIC -o $@ -c $< -MD

$(STARTO): $(START)
		@echo "	" AS $(START)
		@nasm $(ASMFLAGS) -o $(STARTO) $(START)

-include $(DEPS)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(COBJS) $(CPICOBJS) $(STLIB) $(DYNLIB) $(DEPS) $(STARTO)
