NAME=libctest
ROOT=../../..
BUILD=$(ROOT)/build
DISK=$(BUILD)/disk.img
DISKMOUNT=$(ROOT)/disk
BIN=$(BUILD)/user_$(NAME).bin
DEP=$(BUILD)/user_$(NAME).dep
LIB=$(ROOT)/lib
LIBC=$(ROOT)/libc
LDCONF=$(LIBC)/ld.conf

CC = gcc
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs -I$(LIBC)/include -I$(LIB)/h \
	-Wl,-T,$(LDCONF) $(CDEFFLAGS) 
CSRC=$(wildcard *.c)
CCMDSRC=$(wildcard tests/*.c)

LIBCA=$(BUILD)/libc.a
START=$(BUILD)/libc_startup.o
COBJ=$(patsubst %.c,$(BUILD)/user_$(NAME)_%.o,$(CSRC))
CCMDOBJ=$(patsubst tests/%.c,$(BUILD)/user_$(NAME)_tests_%.o,$(CCMDSRC))

.PHONY: all clean

all:	$(BIN)

$(BIN):	$(LDCONF) $(COBJ) $(CCMDOBJ) $(START) $(LIBCA)
		@echo "===== LINKING =====";
		$(CC) $(CFLAGS) -o $(BIN) $(START) $(COBJ) $(CCMDOBJ) $(LIBCA);
		@echo "===== COPYING ON DISK =====";
		@make -C $(ROOT) mounthdd
		$(SUDO) cp $(BIN) $(DISKMOUNT)/bin/$(NAME)
		@make -C $(ROOT) umounthdd

$(BUILD)/user_$(NAME)_%.o:		%.c
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD)/user_$(NAME)_tests_%.o:		tests/%.c
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(DEP):	$(CSRC) $(CCMDSRC)
		@echo "===== GENERATING DEPENDENCIES =====";
		$(CC) $(CFLAGS) -MM $(CSRC) $(CCMDSRC) > $(DEP);
		@# prefix all files with the build-path (otherwise make wouldn't find them)
		sed --in-place -e "s/\([a-zA-Z_]*\).o:/$(subst /,\/,$(BUILD)\/user_$(NAME)_)\1.o:/g" $(DEP);
		@for i in $(patsubst tests/%.c,%,$(wildcard tests/*.c)) ; do \
			sed --in-place -e "s/$(subst /,\/,$(BUILD)/user_$(NAME)_)$$i.o:/$(subst /,\/,$(BUILD)/user_$(NAME)_tests_)$$i.o:/g" $(DEP); \
		done;

-include $(DEP)

clean:
		rm -f $(BIN) $(DEP) $(COBJ) $(CCMDOBJ)
