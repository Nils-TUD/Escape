NAME=cpptest
ROOT = ../../..
BUILDL = $(BUILD)/user/cpp/$(NAME)
BIN = $(BUILD)/user_$(NAME).bin
LIB = $(ROOT)/lib
LIBC = $(ROOT)/libc
LIBCPP = $(ROOT)/libcpp
LIBGCC = $(ROOT)/libgcc
LDCONF=$(LIBCPP)/ld.conf
SUBDIRS = . $(filter-out Makefile $(wildcard *.*),$(wildcard *))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -mindepth 0 -maxdepth 1 -name "*.d")
ADDLIBS = $(LIBGCC)/libgcc.a $(LIBGCC)/libgcc_eh.a

CC = g++
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs -I$(LIBCPP) -I$(LIBCPP)/ustl -I$(LIBC)/include \
	-I$(LIB)/h -Wl,-T,$(LDCONF) -Wl,--build-id=none $(CPPDEFFLAGS) $(ADDFLAGS)

# sources
CSRC = $(shell find $(SUBDIRS) -mindepth 0 -maxdepth 1 -name "*.cpp")

# objects
START = $(BUILD)/libcpp_startup.o
COBJ = $(patsubst %.cpp,$(BUILDL)/%.o,$(CSRC))

.PHONY: all clean

all:	$(BIN)

$(BIN):	$(BUILDDIRS) $(LDCONF) $(COBJ) $(START) $(ADDLIBS)
		@echo "	" LINKING $(BIN)
		@$(CC) $(CFLAGS) $(DLNKFLAGS) -o $(BIN) -lc -lustl $(START) $(COBJ) $(ADDLIBS);
		@echo "	" COPYING ON DISK
		$(ROOT)/tools/disk.sh copy $(BIN) /bin/$(NAME)

$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDL)/%.o:		%.cpp
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MMD

-include $(DEPS)

clean:
		rm -f $(BIN) $(COBJ) $(DEPS)
