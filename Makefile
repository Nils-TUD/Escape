# general
BUILD=build
DISK=$(BUILD)/disk.img
BINNAME=kernel.bin
BIN=$(BUILD)/$(BINNAME)

.PHONY: all qemu bochs clean

all: $(BUILD)
		make -C kernel all;

$(BUILD):
		[ -d $(BUILD) ] || mkdir -p $(BUILD);

qemu:	all
		qemu -serial stdio -no-kqemu -fda $(DISK) -d int;

bochs: all
		bochs -f bochs.cfg;

debug: all
		qemu -s -S -no-kqemu -fda $(DISK) &
		sleep 0.3;
		gdb --command=gdb.start --symbols $(BIN);

clean:
		make -C kernel clean;
