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
CSUBSRC=$(wildcard $(SUBDIR)/*.c)

LIBCA=$(BUILD)/libc.a
START=$(BUILD)/libc_startup.o
COBJ=$(patsubst %.c,$(BUILD)/user_$(NAME)_%.o,$(CSRC))
CSUBOBJ=$(patsubst $(SUBDIR)/%.c,$(BUILD)/user_$(NAME)_$(SUBDIR)_%.o,$(CSUBSRC))

.PHONY: all clean

all:	$(BIN)

$(BIN):	$(LDCONF) $(COBJ) $(CSUBOBJ) $(START) $(LIBCA)
		@echo "	" LINKING $(BIN)
		@$(CC) $(CFLAGS) -o $(BIN) $(START) $(COBJ) $(CSUBOBJ) $(LIBCA);
		@echo "	" COPYING ON DISK
		$(ROOT)/tools/cp2disk.sh $(BIN) /bin/$(NAME)

$(BUILD)/user_$(NAME)_%.o:		%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD)/user_$(NAME)_$(SUBDIR)_%.o:		$(SUBDIR)/%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $<

$(DEP):	$(CSRC) $(CSUBSRC)
		@echo "	" GENERATING DEPENDENCIES
		@$(CC) $(CFLAGS) -MM $(CSRC) $(CSUBSRC) > $(DEP);
		@# prefix all files with the build-path (otherwise make wouldn't find them)
		@sed --in-place -e "s/\([a-zA-Z_]*\).o:/$(subst /,\/,$(BUILD)\/user_$(NAME)_)\1.o:/g" $(DEP);
		@for i in $(patsubst $(SUBDIR)/%.c,%,$(wildcard $(SUBDIR)/*.c)) ; do \
			sed --in-place -e "s/$(subst /,\/,$(BUILD)/user_$(NAME)_)$$i.o:/$(subst /,\/,$(BUILD)/user_$(NAME)_$(SUBDIR)_)$$i.o:/g" $(DEP); \
		done;

-include $(DEP)

clean:
		rm -f $(BIN) $(COBJ) $(CSUBOBJ) $(DEP)
