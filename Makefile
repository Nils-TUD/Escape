# general
BUILD=build
DISK=$(BUILD)/disk.img
DISKMOUNT=disk
BINNAME=kernel.bin
BIN=$(BUILD)/$(BINNAME)
SYMBOLS=$(BUILD)/kernel.symbols

DIRS = tools libc user kernel kernel/test

# warning flags for gcc
export CWFLAGS=-Wall -ansi \
				 -Wextra -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-prototypes \
				 -Wmissing-declarations -Wredundant-decls -Wnested-externs -Winline -Wno-long-long \
				 -Wstrict-prototypes

.PHONY: all disk dis qemu bochs debug debugu debugm debugt test clean

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

qemu:	all prepareRun
		qemu -serial stdio -no-kqemu -fda $(DISK) > log.txt 2>&1

bochs: all prepareRun
		bochs -f bochs.cfg -q > log.txt 2>&1

debug: all prepareRun
		qemu -serial stdio -s -S -no-kqemu -fda $(DISK) > log.txt 2>&1 &
		@#bochs -f bochs.cfg -q > log.txt 2>&1 &
		sleep 1;
		gdb --command=gdb.start
		# --symbols $(BIN)

debugu: all prepareRun
		qemu -serial stdio -s -S -no-kqemu -fda $(DISK) > log.txt 2>&1 &
		sleep 1;
		gdb --command=gdb.start --symbols $(BUILD)/user_task1.bin

debugm: all prepareRun
		qemu -serial stdio -s -S -no-kqemu -fda $(DISK) > log.txt 2>&1 &
		@#bochs -f bochs.cfg -q > log.txt 2>&1 &

debugt: all prepareTest
		qemu -serial stdio -s -S -no-kqemu -fda $(DISK) > log.txt 2>&1 &

test: all prepareTest
		qemu -serial stdio -no-kqemu -fda $(DISK) > log.txt 2>&1

prepareTest:
		sudo mount -o loop $(DISK) $(DISKMOUNT) || true;
		sed --in-place -e "s/^kernel.*/kernel \/kernel_test.bin/g" $(DISKMOUNT)/grub/menu.lst;
		sudo umount $(DISKMOUNT);

prepareRun:
		sudo mount -o loop $(DISK) $(DISKMOUNT) || true;
		sed --in-place -e "s/^kernel.*/kernel \/kernel.bin/g" $(DISKMOUNT)/grub/menu.lst;
		sudo umount $(DISKMOUNT);

clean:
		@for i in $(DIRS); do \
			make -C $$i clean || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done
