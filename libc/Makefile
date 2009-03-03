BUILD = ../build
LIBDIR=../lib

CC = gcc
CFLAGS = -g -nostdlib -nostartfiles -nodefaultlibs $(CWFLAGS) -I$(LIBDIR)/h

CSRCS = $(wildcard src/*.c)
TSRCS = $(wildcard test/*.c)
ASRCS = $(wildcard src/*.s)
LSRC=$(filter-out $(LIBDIR)/src/video.c,$(wildcard $(LIBDIR)/src/*.c))
OBJS = $(patsubst src/%.c,$(BUILD)/libc_%.o,$(CSRCS))
OBJS += $(patsubst src/%.s,$(BUILD)/libc_a%.o,$(ASRCS))
OBJS += $(patsubst test/%.c,$(BUILD)/libc_t%.o,$(TSRCS))
LOBJ=$(patsubst $(LIBDIR)/src/%.c,$(BUILD)/libc_lib_%.o,$(LSRC))
LIB = $(BUILD)/libc.a
DEP = $(BUILD)/libc.dep
TDEP = $(BUILD)/libctest.dep
START = startup.s
STARTO = $(BUILD)/libc_startup.o

.PHONY:		all clean

all: $(LIB)

$(LIB): $(OBJS) $(LOBJ) $(STARTO)
		@echo "===== BUILDING ARCHIVE =====";
		ar rcs $(LIB) $(OBJS) $(LOBJ)

$(BUILD)/libc_%.o: src/%.c
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD)/libc_t%.o: test/%.c
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD)/libc_a%.o: src/%.s
		@echo "===== ASSEMBLING $< =====";
		nasm -g -f elf -o $@ $<

$(BUILD)/libc_lib_%.o:	$(LIBDIR)/src/%.c
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(STARTO): $(START)
		@echo "===== ASSEMBLING $(START) =====";
		nasm -g -f elf -o $(STARTO) $(START)

$(DEP):	$(CSRCS)
		@echo "===== GENERATING DEPENDENCIES =====";
		$(CC) $(CFLAGS) -MM $(CSRCS) > $(DEP)
		@# prefix all files with the build-path (otherwise make wouldn't find them)
		sed --in-place -e "s/\([a-zA-Z_]*\).o:/$(subst /,\/,$(BUILD))\/libc_\1.o:/g" $(DEP);

$(TDEP): $(TSRCS)
		@echo "===== GENERATING TEST-DEPENDENCIES =====";
		$(CC) $(CFLAGS) -MM $(TSRCS) > $(TDEP)
		@# prefix all files with the build-path (otherwise make wouldn't find them)
		sed --in-place -e "s/\([a-zA-Z_]*\).o:/$(subst /,\/,$(BUILD))\/libc_t\1.o:/g" $(TDEP);

-include $(DEP)
-include $(TDEP)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(OBJS) $(LOBJ) $(LIB) $(DEP) $(TDEP)
