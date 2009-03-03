# general
BUILD=build
DISK=$(BUILD)/disk.img
DISKMOUNT=disk
BINNAME=kernel.bin
BIN=$(BUILD)/$(BINNAME)
SYMBOLS=$(BUILD)/kernel.symbols
OSTITLE=hrniels-OS

DIRS = tools libc services user kernel kernel/test

# warning flags for gcc
export CWFLAGS=-Wall -ansi \
				 -Wextra -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-prototypes \
				 -Wmissing-declarations -Wredundant-decls -Wnested-externs -Winline -Wno-long-long \
				 -Wstrict-prototypes

.PHONY: all disk dis qemu bochs debug debugu debugm debugt test clean

all: $(BUILD)
		[ -f $(DISK) ] || make disk;
		@for i in $(DIRS); do \
			make -C $$i all || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done

$(BUILD):
		[ -d $(BUILD) ] || mkdir -p $(BUILD);

disk: clean
		sudo umount $(DISKMOUNT) || true;
		dd if=/dev/zero of=$(DISK) bs=1024 count=1440;
		/sbin/mke2fs -F $(DISK);
		sudo mount -o loop $(DISK) $(DISKMOUNT);
		mkdir $(DISKMOUNT)/grub;
		cp boot/stage1 $(DISKMOUNT)/grub;
		cp boot/stage2 $(DISKMOUNT)/grub;
		echo 'default 0' > $(DISKMOUNT)/grub/menu.lst;
		echo 'timeout 0' >> $(DISKMOUNT)/grub/menu.lst;
		echo '' >> $(DISKMOUNT)/grub/menu.lst;
		echo "title $(OSTITLE)" >> $(DISKMOUNT)/grub/menu.lst;
		echo "kernel /$(BINNAME)" >> $(DISKMOUNT)/grub/menu.lst;
		echo 'root (fd0)' >> $(DISKMOUNT)/grub/menu.lst;
		echo -n "device (fd0) $(DISK)\nroot (fd0)\nsetup (fd0)\nquit\n" | grub --batch;
		sudo umount $(DISKMOUNT);
		make all;
		sudo mount -o loop $(DISK) $(DISKMOUNT);
		cp $(BIN) $(DISKMOUNT);
		sudo umount $(DISKMOUNT);

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
		#sleep 1;
		#gdb --command=gdb.start --symbols $(BUILD)/user_task1.bin

debugc: all prepareRun
		qemu -serial stdio -s -S -no-kqemu -fda $(DISK) > log.txt 2>&1 &
		sleep 1;
		gdb --command=gdb.start --symbols $(BUILD)/service_console.bin

debugm: all prepareRun
		qemu -serial stdio -s -S -no-kqemu -fda $(DISK) > log.txt 2>&1 &
		@#bochs -f bochs.cfg -q > log.txt 2>&1 &

debugt: all prepareTest
		qemu -serial stdio -s -S -no-kqemu -fda $(DISK) > log.txt 2>&1 &

test: all prepareTest
		#bochs -f bochs.cfg -q > log.txt 2>&1 &
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
