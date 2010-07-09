ROOT = ../../..
BUILDL = $(BUILD)/user/cpp/$(NAME)
BIN = $(BUILD)/user_$(NAME).bin
LIBC = $(ROOT)/lib/c
LIBCPP = $(ROOT)/lib/cpp
LDCONF = $(ROOT)/lib/ld.conf
SUBDIRS = . $(filter-out Makefile $(wildcard *.*),$(wildcard *))
BUILDDIRS = $(addprefix $(BUILDL)/,$(SUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -mindepth 0 -maxdepth 1 -name "*.d")

CC = g++
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs -I$(ROOT)/include -I$(ROOT)/include/cpp \
	-Wl,-T,$(LDCONF) -Wl,--build-id=none $(CPPDEFFLAGS) -fno-exceptions $(ADDFLAGS)

# sources
CSRC = $(shell find $(SUBDIRS) -mindepth 0 -maxdepth 1 -name "*.cpp")

# objects
LIBCA = $(BUILD)/libc.a
LIBCPPA = $(BUILD)/libcpp.a
START = $(BUILD)/libcpp_startup.o
COBJ = $(patsubst %.cpp,$(BUILDL)/%.o,$(CSRC))

ifeq ($(LINKTYPE),static)
	ADDLIBS += $(LIBCPPA)
endif

.PHONY: all clean

all:	$(BIN)

$(BIN):	$(BUILDDIRS) $(LDCONF) $(COBJ) $(START) $(ADDLIBS)
		@echo "	" LINKING $(BIN)
ifeq ($(LINKTYPE),static)
		@$(CC) $(CFLAGS) -o $(BIN) $(START) $(LIBCA) $(COBJ) $(ADDLIBS);
else
		@$(CC) $(CFLAGS) $(DLNKFLAGS) -o $(BIN) -lcpp $(START) $(COBJ) $(ADDLIBS);
endif
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
