BUILD = ../build
BUILDL = $(BUILD)/libcpp
LIBDIR = ../lib
LIBCDIR = ../libc
SUBDIRS = src src/esc src/esc/gui
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -name "*.d")
LIB = $(BUILD)/libcpp.a

CC = g++
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs $(CPPDEFFLAGS) -Iinclude -I$(LIBDIR)/h \
	-I$(LIBCDIR)/include

# sources
CSRC = $(shell find $(SUBDIRS) -name "*.cpp")
START = startup.s

# objects
COBJS = $(patsubst %.cpp,$(BUILDL)/%.o,$(CSRC))
# for long long arithmetic
GCCLIBDIR = ../libgcc
GCCLIB = $(GCCLIBDIR)/_divdi3.object \
	$(GCCLIBDIR)/_moddi3.object \
	$(GCCLIBDIR)/_udivdi3.object \
	$(GCCLIBDIR)/_umoddi3.object
LIBCOBJS = $(shell find $(BUILD)/libc -name "*.o") $(GCCLIB)
STARTO = $(BUILD)/libcpp_startup.o

.PHONY:		all clean

all: $(LIB)

$(LIB): $(BUILDDIRS) $(COBJS) $(STARTO) $(LIBCOBJS)
		@echo "	" AR $(LIB)
		@ar rcs $(LIB) $(LIBCOBJS) $(COBJS)
		
$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/%.o: %.cpp
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MMD

$(STARTO): $(START)
		@echo "	" AS $(START)
		@nasm $(ASMFLAGS) -o $(STARTO) $(START)

-include $(DEPS)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(COBJS) $(LIB) $(DEPS) $(STARTO)
