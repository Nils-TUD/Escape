BUILD = ../build

CC = gcc
CFLAGS = -g -nostdlib -nostartfiles -nodefaultlibs $(CWFLAGS)

CSRCS = $(wildcard src/*.c)
ASRCS = $(wildcard src/*.s)
OBJS = $(patsubst src/%.c,$(BUILD)/libc_%.o,$(CSRCS))
OBJS += $(patsubst src/%.s,$(BUILD)/libc_a%.o,$(ASRCS))
LIB = $(BUILD)/libc.a
DEP = $(BUILD)/libc.dep
START = startup.s
STARTO = $(BUILD)/libc_startup.o

.PHONY:		all clean

all: $(LIB)

$(LIB): $(OBJS) $(STARTO)
		@echo "===== BUILDING ARCHIVE =====";
		ar rcs $(LIB) $(OBJS)

$(BUILD)/libc_%.o: src/%.c
		@echo "===== COMPILING $< =====";
		$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD)/libc_a%.o: src/%.s
		@echo "===== ASSEMBLING $< =====";
		nasm -g -f elf -o $@ $<

$(STARTO): $(START)
		@echo "===== ASSEMBLING $(START) =====";
		nasm -g -f elf -o $(STARTO) $(START)

$(DEP):	$(CSRCS)
		@echo "===== GENERATING DEPENDENCIES =====";
		$(CC) $(CFLAGS) -MM $(CSRCS) > $(DEP)

-include $(DEP)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(OBJS) $(LIB) $(DEP)
