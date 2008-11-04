# general
BUILD=build
DISK=$(BUILD)/disk.img
BINNAME=kernel.bin
BIN=$(BUILD)/$(BINNAME)

DIRS = tools user kernel

.PHONY: all qemu bochs clean

all: $(BUILD)
		@for i in $(DIRS); do \
			make -C $$i all || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done

$(BUILD):
		[ -d $(BUILD) ] || mkdir -p $(BUILD);

dis: all
		objdump -d -S $(BIN) | less

qemu:	all
		qemu -serial stdio -no-kqemu -fda $(DISK) > log.txt 2>&1

bochs: all
		bochs -f bochs.cfg > log.txt 2>&1;

debug: all
		qemu -serial stdio -s -S -no-kqemu -fda $(DISK) > log.txt 2>&1 &
		sleep 0.5;
		gdb --command=gdb.start $(BIN)

clean:
		@for i in $(DIRS); do \
			make -C $$i clean || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done
