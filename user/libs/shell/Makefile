NAME=shell
ROOT=../../..
BUILD=$(ROOT)/build
DISK=$(BUILD)/disk.img
DISKMOUNT=$(ROOT)/disk
BIN=$(BUILD)/user_libs_$(NAME).a
DEP=$(BUILD)/user_libs_$(NAME).dep
LIB=$(ROOT)/lib
LIBC=$(ROOT)/libc

CC = gcc
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs -I$(LIBC)/include -I$(LIB)/h \
	-Wl,-T,$(LDCONF) $(CDEFFLAGS) 
CSRC=$(wildcard *.c)
CCMDSRC=$(wildcard cmd/*.c)

# put all libc-object-files into the archive
LIBCOBJ=$(wildcard $(BUILD)/libc_*.o)
COBJ=$(patsubst %.c,$(BUILD)/user_libs_$(NAME)_%.o,$(CSRC))
CCMDOBJ=$(patsubst cmd/%.c,$(BUILD)/user_libs_$(NAME)_cmd_%.o,$(CCMDSRC))

.PHONY: all clean

all:	$(BIN)

$(BIN): $(COBJ) $(CCMDOBJ) $(LIBCOBJ)
		@echo "	" AR $(BIN)
		@ar rcs $(BIN) $(COBJ) $(CCMDOBJ) $(LIBCOBJ)

$(BUILD)/user_libs_$(NAME)_%.o:		%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD)/user_libs_$(NAME)_cmd_%.o:		cmd/%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $<

$(DEP):	$(CSRC) $(CCMDSRC)
		@echo "	" GENERATING DEPENDENCIES
		@$(CC) $(CFLAGS) -MM $(CSRC) $(CCMDSRC) > $(DEP);
		@# prefix all files with the build-path (otherwise make wouldn't find them)
		@sed --in-place -e "s/\([a-zA-Z_]*\).o:/$(subst /,\/,$(BUILD)\/user_libs_$(NAME)_)\1.o:/g" $(DEP);
		@for i in $(patsubst cmd/%.c,%,$(wildcard cmd/*.c)) ; do \
			sed --in-place -e "s/$(subst /,\/,$(BUILD)/user_$(NAME)_)$$i.o:/$(subst /,\/,$(BUILD)/user_libs_$(NAME)_cmd_)$$i.o:/g" $(DEP); \
		done;

-include $(DEP)

clean:
		rm -f $(BIN) $(DEP) $(COBJ) $(CCMDOBJ)
