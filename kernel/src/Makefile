# general
ROOT = ../..
BUILDK = $(BUILD)/kernel/src
BIN = $(BUILD)/kernel.bin
MAP = $(BUILD)/kernel.map
LDCONF = arch/$(ARCH)/ld.conf
LIB = $(ROOT)/lib/basic
SUBDIRS = $(shell find . -type d | grep -v '\.svn')
LSUBDIRS = $(shell find $(LIB) -type d | grep -v '\.svn')
BUILDDIRS = $(addprefix $(BUILDK)/,$(SUBDIRS)) $(patsubst $(LIB)/%,$(BUILDK)/lib/%,$(LSUBDIRS))
DEPS = $(shell find $(BUILDDIRS) -name "*.d")

CFLAGS = -ffreestanding -nostdlib -nostartfiles -nodefaultlibs $(CDEFFLAGS) -D IN_KERNEL=1 #\
 -D PROFILE=1 -finstrument-functions -finstrument-functions-exclude-file-list=ksymbols.c,$(LIB)/profile.c

# sources
ASRC = $(filter-out $(shell find ./arch -name "*.s"),\
	$(shell find . -name "*.s" | grep -v '\.svn'))
ASRC += $(filter-out ./arch/$(ARCH)/end.s,$(shell find ./arch/$(ARCH) -name "*.s" | grep -v '\.svn'))
ASRC += ./arch/$(ARCH)/end.s
ifeq ($(BUILDTYPE),release)
# remove dbg-files in release-mode
CSRC = $(filter-out $(shell find ./arch -name "*.c") $(shell find ./dbg -name "*.c"),\
	$(shell find . -name "*.c" | grep -v '\.svn'))
CSRC += $(shell find ./arch/$(ARCH) -name "*.c" | grep -v '\.svn')
else
CSRC = $(filter-out $(shell find ./arch -name "*.c"),$(shell find . -name "*.c" | grep -v '\.svn'))
CSRC += $(shell find ./arch/$(ARCH) -name "*.c" | grep -v '\.svn')
endif
ifeq ($(ARCH),eco32)
	LSRC = $(filter-out $(LIB)/test.c \
		$(shell find $(LIB)/arch -name "*.c"),$(shell find $(LIB) -name "*.c" | grep -v '\.svn'))
else
	LSRC = $(filter-out $(shell find $(LIB)/arch -name "*.c"),\
		$(shell find $(LIB) -name "*.c" | grep -v '\.svn'))
endif
LSRC += $(shell find $(LIB)/arch/$(ARCH) -name "*.c" | grep -v '\.svn')

# objects
AOBJ = $(patsubst %.s,$(BUILDK)/%.s.o,$(ASRC))
COBJ = $(patsubst %.c,$(BUILDK)/%.o,$(CSRC))
LOBJ = $(patsubst $(LIB)/%.c,$(BUILDK)/lib/%.o,$(LSRC))

SYMBOLS = $(BUILD)/kernel_symbols.txt
SYMBOLSCRIPT = $(ROOT)/tools/gensymbols.sh
SYMBOLSRC = ksymbols.c
SYMBOLOBJ = $(patsubst %.c,$(BUILDK)/%.o,$(SYMBOLSRC))

.PHONY: all pre clean

all:	$(BIN) $(MAP)

$(BIN):	$(BUILDDIRS) $(SYMBOLS) $(DRIVERS) $(LDCONF) $(AOBJ) $(COBJ) $(LOBJ)
		@echo "	" LINKING $(BIN)
		@$(CC) $(CFLAGS) -Wl,-T,$(LDCONF) -o $(BIN) $(AOBJ) $(COBJ) $(LOBJ) -lgcc;
		@# Note that we link 2 additional times because otherwise the symbol-table is incorrect
		@# since it's size is 0 at the first time. In the second pass the size is correct
		@# and therefore the symbol-table will be generated correctly in the third pass :)
		@echo "	" BUILDING SYMBOL-FILE
		@$(SYMBOLSCRIPT) $(BIN) > $(SYMBOLS)
		@echo "	" CC $(SYMBOLSRC)
		@$(CC) $(CFLAGS) -o $(SYMBOLOBJ) -c $(SYMBOLSRC)
		@echo "	" LINKING $(BIN)
		@$(CC) $(CFLAGS) -Wl,-T,$(LDCONF) -o $(BIN) $(AOBJ) $(COBJ) $(LOBJ) -lgcc;
		@echo "	" BUILDING SYMBOL-FILE
		@$(SYMBOLSCRIPT) $(BIN) > $(SYMBOLS)
		@echo "	" CC $(SYMBOLSRC)
		@$(CC) $(CFLAGS) -o $(SYMBOLOBJ) -c $(SYMBOLSRC)
		@echo "	" LINKING $(BIN)
		@$(CC) $(CFLAGS) -Wl,-T,$(LDCONF) -o $(BIN) $(AOBJ) $(COBJ) $(LOBJ) -lgcc;

$(MAP): $(BIN)
		@echo "	" GEN MAP $@
		@$(NM) -S $(BIN) | $(ROOT)/tools/createmap-mmix.php > $@

$(SYMBOLS):
		@if [ ! -f $(SYMBOLS) ]; then echo "" > $(SYMBOLS); fi
		
$(BUILDDIRS):
		@for i in $(BUILDDIRS); do \
			if [ ! -d $$i ]; then mkdir -p $$i; fi \
		done;

$(BUILDK)/%.s.o: %.s
		@echo "	" AS $<
		@$(AS) $(ASFLAGS) -o $@ $<;

$(BUILDK)/%.o:		%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MD

$(BUILDK)/lib/%.o:	$(LIB)/%.c
		@echo "	" CC $<
		@$(CC) $(CFLAGS) -o $@ -c $< -MD

-include $(DEPS)

clean:
		@echo "===== REMOVING FILES =====";
		rm -f $(DEPS) $(BIN) $(COBJ) $(AOBJ) $(LOBJ) $(SYMBOLS) $(SYMBOLOBJ) $(MAP)
