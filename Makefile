# general
BUILD=build
DISK=$(BUILD)/disk.img
BINNAME=kernel.bin
BIN=$(BUILD)/$(BINNAME)
SYMBOLS=$(BUILD)/kernel.symbols

DIRS = tools user kernel

.PHONY: all qemu disk dis debug debugm bochs clean

all: $(BUILD)
		@for i in $(DIRS); do \
			make -C $$i all || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done

$(BUILD):
		[ -d $(BUILD) ] || mkdir -p $(BUILD);

disk:
		./builddisk.sh

dis: all
		objdump -d -S $(BIN) | less

qemu:	all
		qemu -serial stdio -no-kqemu -fda $(DISK) > log.txt 2>&1

bochs: all
		bochs -f bochs.cfg -q;
		# > log.txt 2>&1

debugm: all
		qemu -serial stdio -s -S -no-kqemu -fda $(DISK) > log.txt 2>&1 &
		@#bochs -f bochs.cfg -q > log.txt 2>&1 &

debug: all
		qemu -serial stdio -s -S -no-kqemu -fda $(DISK) > log.txt 2>&1 &
		@#bochs -f bochs.cfg -q > log.txt 2>&1 &
		sleep 1;
		gdb --command=gdb.start --symbols $(BIN)

clean:
		@for i in $(DIRS); do \
			make -C $$i clean || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done
