NAME=cpptest
BUILD=../../build
DISK=$(BUILD)/disk.img
DISKMOUNT=../../disk
BIN=$(BUILD)/user_$(NAME).bin
DEP=$(BUILD)/user_$(NAME).dep
LIBC=../../libc
LIBCPP=../../libcpp
LDCONF=$(LIBCPP)/ld.conf

CC = g++
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs -I$(LIBCPP)/include -I$(LIBC)/include \
	-I../../lib/h -Wl,-T,$(LDCONF) $(CPPDEFFLAGS) -fno-exceptions -fno-rtti
CSRC=$(wildcard *.cpp)

LIBCPPA=$(BUILD)/libcpp.a
START=$(BUILD)/libcpp_startup.o
COBJ=$(patsubst %.cpp,$(BUILD)/user_$(NAME)_%.o,$(CSRC))

.PHONY: all clean

all:	$(BIN)

$(BIN):	$(LDCONF) $(COBJ) $(START) $(LIBCPPA)
		@echo "===== COMPILING & LINKING =====";
		$(CC) $(CFLAGS) -o $(BIN) $(START) $(COBJ) $(LIBCPPA);
		@echo "===== COPYING ON DISK =====";
		@make -C ../../ mounthdd
		sudo cp $(BIN) $(DISKMOUNT)/bin/$(NAME)
		@make -C ../../ umounthdd

$(BUILD)/user_$(NAME)_%.o:		%.cpp
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(DEP):	$(CSRC)
		@echo "===== GENERATING DEPENDENCIES =====";
		$(CC) $(CFLAGS) -MM $(CSRC) > $(DEP);
		@# prefix all files with the build-path (otherwise make wouldn't find them)
		sed --in-place -e "s/\([a-zA-Z_]*\).o:/$(subst /,\/,$(BUILD)\/user_$(NAME)_)\1.o:/g" $(DEP);

-include $(DEP)

clean:
		rm -f $(BIN) $(COBJ) $(DEP)
