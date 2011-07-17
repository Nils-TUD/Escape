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
TSRC = $(wildcard *.c)
TSRC += $(wildcard arch/$(ARCH)/*.c)
KSRC = $(filter-out $(shell find ../src/arch -name "*.c"),\
	$(shell find ../src -name "*.c" | grep -v '\.svn'))
KSRC += $(filter-out ../src/arch/$(ARCH)/start.c,\
	$(shell find ../src/arch/$(ARCH) -name "*.c" | grep -v '\.svn'))
KASRC = ../src/arch/$(ARCH)/kernel.s
KASRC += $(filter-out $(shell find ../src/arch -name "*.s"),\
	$(shell find ../src -name "*.s" | grep -v '\.svn'))
KASRC += $(filter-out ../src/arch/$(ARCH)/end.s ../src/arch/$(ARCH)/kernel.s,\
	$(shell find ../src/arch/$(ARCH) -name "*.s" | grep -v '\.svn'))
KAEND = ../src/arch/$(ARCH)/end.s
LSRC = $(filter-out $(shell find $(LIB)/arch -name "*.c"),\
	$(shell find $(LIB) -name "*.c" | grep -v '\.svn'))
LSRC += $(shell find $(LIB)/arch/$(ARCH) -name "*.c" | grep -v '\.svn')
LASRC = $(shell find $(LIB)/arch/$(ARCH) -name "*.s" | grep -v '\.svn')

# objects
TOBJ = $(patsubst %.c,$(BUILDT)/%.o,$(TSRC))
KOBJ = $(patsubst %.c,$(BUILDK)/%.o,$(KSRC))
KAOBJ = $(patsubst %.s,$(BUILDK)/%.s.o,$(KASRC))
LOBJ = $(patsubst $(LIB)/%.c,$(BUILDT)/lib/%.o,$(LSRC))
LAOBJ = $(patsubst $(LIB)/%.s,$(BUILDT)/lib/%.s.o,$(LASRC))
KAENDOBJ = $(patsubst %.s,$(BUILDK)/%.s.o,$(KAEND))

SYMBOLS = $(BUILD)/kernelt_symbols.txt
SYMBOLSCRIPT = $(ROOT)/tools/gensymbols.sh
SYMBOLSRC = ../src/ksymbols.c
SYMBOLOBJ = $(patsubst ../src/%.c,$(BUILDT)/%.o,$(SYMBOLSRC))

.PHONY: all pre clean

all: $(BIN) $(MAP)

$(BIN):	$(BUILDDIRS) $(LDCONF) $(KAOBJ) $(KOBJ) $(TOBJ) $(LOBJ) $(LAOBJ) $(KAENDOBJ)
	@echo "	" LINKING $(BIN)
	@$(CC) $(CFLAGS) -Wl,-T,$(LDCONF) -o $(BIN) \
		$(KAOBJ) $(KOBJ) $(TOBJ) $(LOBJ) $(LAOBJ) $(KAENDOBJ) -lgcc;
	@# Note that we link 2 additional times because otherwise the symbol-table is incorrect
	@# since it's size is 0 at the first time. In the second pass the size is correct
	@# and therefore the symbol-table will be generated correctly in the third pass :)
	@echo "	" BUILDING SYMBOL-FILE
	@$(SYMBOLSCRIPT) $(BIN) > $(SYMBOLS)
	@echo "	" CC $(SYMBOLSRC)
	@$(CC) $(CFLAGS) -o $(SYMBOLOBJ) -c $(SYMBOLSRC)
	@echo "	" LINKING $(BIN)
	@$(CC) $(CFLAGS) -Wl,-T,$(LDCONF) -o $(BIN) \
		$(KAOBJ) $(KOBJ) $(TOBJ) $(LOBJ) $(LAOBJ) $(KAENDOBJ) -lgcc;
	@echo "	" BUILDING SYMBOL-FILE
	@$(SYMBOLSCRIPT) $(BIN) > $(SYMBOLS)
	@echo "	" CC $(SYMBOLSRC)
	@$(CC) $(CFLAGS) -o $(SYMBOLOBJ) -c $(SYMBOLSRC)
	@echo "	" LINKING $(BIN)
	@$(CC) $(CFLAGS) -Wl,-T,$(LDCONF) -o $(BIN) \
		$(KAOBJ) $(KOBJ) $(TOBJ) $(LOBJ) $(LAOBJ) $(KAENDOBJ) -lgcc;

$(MAP): $(BIN)
	@echo "	" GEN MAP $@
	@$(ROOT)/tools/createmap-$(ARCH) $(BIN) > $@

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
