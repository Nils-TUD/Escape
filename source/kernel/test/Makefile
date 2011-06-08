# general
ROOT = ../..
BUILDK = $(BUILD)/kernel/src
BUILDT = $(BUILD)/kernel/test
BIN = $(BUILD)/kernel_test.bin
MAP = $(BUILD)/kernel_test.map
LDCONF = ../src/arch/$(ARCH)/ld.conf
LIB = $(ROOT)/lib/basic
DEPS = $(wildcard $(BUILDT)/*.d)
SUBDIRS = $(shell find . -type d | grep -v '\.svn')
LSUBDIRS = $(shell find $(LIB) -type d | grep -v '\.svn')
BUILDDIRS = $(BUILDT) $(addprefix $(BUILDT)/,$(SUBDIRS))
BUILDDIRS += $(patsubst $(LIB)/%,$(BUILDT)/lib/%,$(LSUBDIRS))

CFLAGS = -ffreestanding -nostdlib -nostartfiles -nodefaultlibs $(CDEFFLAGS) \
	-D TESTING=1 -D IN_KERNEL=1

# sources
ifeq ($(ARCH),mmix)
TSRC = tmain.c $(wildcard arch/$(ARCH)/*.c)
KASRC = arch/mmix/kernel.s arch/mmix/end.s
KSRC = $(filter-out ../src/arch/$(ARCH)/start.c,\
	$(shell find ../src/arch/$(ARCH) -name "*.c" | grep -v '\.svn'))
KSRC += printf.c mem/pmem.c
LSRC = $(filter-out $(LIB)/hashmap.c $(LIB)/sllist.c $(LIB)/ringbuffer.c \
	$(LIB)/string/strdup.c $(LIB)/string/strndup.c $(shell find $(LIB)/arch -name "*.c"),\
	$(shell find $(LIB) -name "*.c" | grep -v '\.svn'))
LSRC += $(shell find $(LIB)/arch/$(ARCH) -name "*.c" | grep -v '\.svn')
LASRC = $(shell find $(LIB)/arch/$(ARCH) -name "*.s" | grep -v '\.svn')
else
TSRC = $(wildcard *.c)
TSRC += $(wildcard arch/$(ARCH)/*.c)
KSRC = $(filter-out $(shell find ../src/arch -name "*.c"),\
	$(shell find ../src -name "*.c" | grep -v '\.svn'))
KSRC += $(filter-out ../src/arch/$(ARCH)/start.c,\
	$(shell find ../src/arch/$(ARCH) -name "*.c" | grep -v '\.svn'))
KASRC = $(filter-out $(shell find ../src/arch -name "*.s"),\
	$(shell find ../src -name "*.s" | grep -v '\.svn'))
KASRC += $(shell find ../src/arch/$(ARCH) -name "*.s" | grep -v '\.svn')
LSRC = $(filter-out $(shell find $(LIB)/arch -name "*.c"),\
	$(shell find $(LIB) -name "*.c" | grep -v '\.svn'))
LSRC += $(shell find $(LIB)/arch/$(ARCH) -name "*.c" | grep -v '\.svn')
LASRC = $(shell find $(LIB)/arch/$(ARCH) -name "*.s" | grep -v '\.svn')
endif

# objects
TOBJ = $(patsubst %.c,$(BUILDT)/%.o,$(TSRC))
KOBJ = $(patsubst %.c,$(BUILDK)/%.o,$(KSRC))
KAOBJ = $(patsubst %.s,$(BUILDK)/%.s.o,$(KASRC))
LOBJ = $(patsubst $(LIB)/%.c,$(BUILDT)/lib/%.o,$(LSRC))
LAOBJ = $(patsubst $(LIB)/%.s,$(BUILDT)/lib/%.s.o,$(LASRC))

SYMBOLS = $(BUILD)/kernelt_symbols.txt
SYMBOLSCRIPT = $(ROOT)/tools/gensymbols.sh
SYMBOLSRC = ../src/ksymbols.c
SYMBOLOBJ = $(patsubst ../src/%.c,$(BUILDT)/%.o,$(SYMBOLSRC))

.PHONY: all pre clean

all: $(BIN) $(MAP)

$(BIN):	$(BUILDDIRS) $(LDCONF) $(KAOBJ) $(KOBJ) $(TOBJ) $(LOBJ) $(LAOBJ)
		@echo "	" LINKING $(BIN)
		@$(CC) $(CFLAGS) -Wl,-T,$(LDCONF) -o $(BIN) $(KAOBJ) $(KOBJ) $(TOBJ) $(LOBJ) $(LAOBJ) -lgcc;
		@# Note that we link 2 additional times because otherwise the symbol-table is incorrect
		@# since it's size is 0 at the first time. In the second pass the size is correct
		@# and therefore the symbol-table will be generated correctly in the third pass :)
		@echo "	" BUILDING SYMBOL-FILE
		@$(SYMBOLSCRIPT) $(BIN) > $(SYMBOLS)
		@echo "	" CC $(SYMBOLSRC)
		@$(CC) $(CFLAGS) -o $(SYMBOLOBJ) -c $(SYMBOLSRC)
		@echo "	" LINKING $(BIN)
		@$(CC) $(CFLAGS) -Wl,-T,$(LDCONF) -o $(BIN) $(KAOBJ) $(KOBJ) $(TOBJ) $(LOBJ) $(LAOBJ) -lgcc;
		@echo "	" BUILDING SYMBOL-FILE
		@$(SYMBOLSCRIPT) $(BIN) > $(SYMBOLS)
		@echo "	" CC $(SYMBOLSRC)
		@$(CC) $(CFLAGS) -o $(SYMBOLOBJ) -c $(SYMBOLSRC)
		@echo "	" LINKING $(BIN)
		@$(CC) $(CFLAGS) -Wl,-T,$(LDCONF) -o $(BIN) $(KAOBJ) $(KOBJ) $(TOBJ) $(LOBJ) $(LAOBJ) -lgcc;

$(MAP): $(BIN)
		@echo "	" GEN MAP $@
		@$(NM) -S $(BIN) | $(ROOT)/tools/createmap-mmix.php > $@

$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDT)/%.o:		%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MD

$(BUILDT)/lib/%.o:	$(LIB)/%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MD

$(BUILDT)/lib/%.s.o:	$(LIB)/%.s
		@echo "	" AS $<
		@$(AS) $(ASFLAGS) -o $@ $<

-include $(DEPS)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(DEPS) $(BIN) $(MAP) $(TOBJ) $(LOBJ) $(LAOBJ) $(SYMBOLS) $(SYMBOLOBJ);
