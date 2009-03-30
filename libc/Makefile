BUILD = ../build
LIBDIR=../lib

CC = gcc
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs $(CDEFFLAGS) -I$(LIBDIR)/h

CESCSRCS = $(wildcard esc/src/*.c)
CSTDSRCS = $(wildcard std/src/*.c)
AESCSRCS = $(wildcard esc/src/*.s)
ASTDSRCS = $(wildcard std/src/*.s)
LSRC=$(wildcard $(LIBDIR)/src/*.c)
OBJS = $(patsubst esc/src/%.c,$(BUILD)/libc_esc_%.o,$(CESCSRCS))
OBJS += $(patsubst esc/src/%.s,$(BUILD)/libc_esc_a%.o,$(AESCSRCS))
OBJS += $(patsubst std/src/%.c,$(BUILD)/libc_std_%.o,$(CSTDSRCS))
OBJS += $(patsubst std/src/%.s,$(BUILD)/libc_std_a%.o,$(ASTDSRCS))
LOBJ=$(patsubst $(LIBDIR)/src/%.c,$(BUILD)/libc_lib_%.o,$(LSRC))
LIB = $(BUILD)/libc.a
DEP = $(BUILD)/libc.dep
START = startup.s
STARTO = $(BUILD)/libc_startup.o

.PHONY:		all clean

all: $(LIB)

$(LIB): $(OBJS) $(LOBJ) $(STARTO)
		@echo "===== BUILDING ARCHIVE =====";
		ar rcs $(LIB) $(OBJS) $(LOBJ)

$(BUILD)/libc_esc_%.o: esc/src/%.c
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD)/libc_esc_a%.o: esc/src/%.s
		@echo "===== ASSEMBLING $< =====";
		nasm $(ASMFLAGS) -o $@ $<

$(BUILD)/libc_std_%.o: std/src/%.c
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD)/libc_std_a%.o: std/src/%.s
		@echo "===== ASSEMBLING $< =====";
		nasm $(ASMFLAGS) -o $@ $<

$(BUILD)/libc_lib_%.o:	$(LIBDIR)/src/%.c
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(STARTO): $(START)
		@echo "===== ASSEMBLING $(START) =====";
		nasm $(ASMFLAGS) -o $(STARTO) $(START)

$(DEP):	$(CESCSRCS) $(CSTDSRCS)
		@echo "===== GENERATING DEPENDENCIES =====";
		$(CC) $(CFLAGS) -MM $(CESCSRCS) $(CSTDSRCS) > $(DEP)
		@# prefix all files with the build-path (otherwise make wouldn't find them)
		@for i in $(patsubst esc/src/%.c,%,$(wildcard esc/src/*.c)) ; do \
			sed --in-place -e "s/$$i.o:/$(subst /,\/,$(BUILD)/libc_esc_)$$i.o:/g" $(DEP); \
		done;
		@for i in $(patsubst std/src/%.c,%,$(wildcard std/src/*.c)) ; do \
			sed --in-place -e "s/$$i.o:/$(subst /,\/,$(BUILD)/libc_std_)$$i.o:/g" $(DEP); \
		done;

-include $(DEP)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(OBJS) $(LOBJ) $(LIB) $(DEP) $(STARTO)
