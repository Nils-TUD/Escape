BUILD = ../build
LIBDIR=../lib

CC = gcc
CFLAGS = -nostdlib -nostartfiles -nodefaultlibs $(CDEFFLAGS) -Iinclude -I$(LIBDIR)/h

CESCSRCS = $(wildcard src/esc/*.c)
CSTDSRCS = $(wildcard src/*.c)
AESCSRCS = $(wildcard src/esc/*.s)
ASTDSRCS = $(wildcard src/*.s)
LSRC=$(wildcard $(LIBDIR)/src/*.c)
OBJS = $(patsubst src/esc/%.c,$(BUILD)/libc_esc_%.o,$(CESCSRCS))
OBJS += $(patsubst src/esc/%.s,$(BUILD)/libc_esc_a%.o,$(AESCSRCS))
OBJS += $(patsubst src/%.c,$(BUILD)/libc_std_%.o,$(CSTDSRCS))
OBJS += $(patsubst src/%.s,$(BUILD)/libc_std_a%.o,$(ASTDSRCS))
LOBJ=$(patsubst $(LIBDIR)/src/%.c,$(BUILD)/libc_lib_%.o,$(LSRC))
LIB = $(BUILD)/libc.a
DEP = $(BUILD)/libc.dep
START = startup.s
STARTO = $(BUILD)/libc_startup.o
# for long long arithmetic
GCCLIBDIR = ../libgcc
GCCLIB = $(GCCLIBDIR)/_divdi3.object \
	$(GCCLIBDIR)/_moddi3.object \
	$(GCCLIBDIR)/_udivdi3.object \
	$(GCCLIBDIR)/_umoddi3.object

.PHONY:		all clean

all: $(LIB)

$(LIB): $(OBJS) $(LOBJ) $(STARTO)
		@echo "	" AR $(LIB)
		@ar rcs $(LIB) $(OBJS) $(LOBJ) $(GCCLIB)

$(BUILD)/libc_esc_%.o: src/esc/%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD)/libc_esc_a%.o: src/esc/%.s
		@echo "	" AS $<
		@nasm $(ASMFLAGS) -o $@ $<

$(BUILD)/libc_std_%.o: src/%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD)/libc_std_a%.o: src/%.s
		@echo "	" AS $<
		@nasm $(ASMFLAGS) -o $@ $<

$(BUILD)/libc_lib_%.o:	$(LIBDIR)/src/%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $<

$(STARTO): $(START)
		@echo "	" AS $(START)
		@nasm $(ASMFLAGS) -o $(STARTO) $(START)

$(DEP):	$(CESCSRCS) $(CSTDSRCS)
		@echo "	" GENERATING DEPENDENCIES
		@$(CC) $(CFLAGS) -MM $(CESCSRCS) $(CSTDSRCS) > $(DEP)
		@# prefix all files with the build-path (otherwise make wouldn't find them)
		@for i in $(patsubst src/esc/%.c,%,$(wildcard src/esc/*.c)) ; do \
			sed --in-place -e "s/$$i.o:/$(subst /,\/,$(BUILD)/libc_esc_)$$i.o:/g" $(DEP); \
		done;
		@for i in $(patsubst src/%.c,%,$(wildcard src/*.c)) ; do \
			sed --in-place -e "s/$$i.o:/$(subst /,\/,$(BUILD)/libc_std_)$$i.o:/g" $(DEP); \
		done;

-include $(DEP)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(OBJS) $(LOBJ) $(LIB) $(DEP) $(STARTO)
