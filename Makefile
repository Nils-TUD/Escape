# general
BUILD=build
FLOPPYDISK=$(BUILD)/disk.img
FLOPPYDISKMOUNT=disk
HDD=$(BUILD)/hd.img
BINNAME=kernel.bin
BIN=$(BUILD)/$(BINNAME)
SYMBOLS=$(BUILD)/kernel.symbols
OSTITLE=hrniels-OS

QEMUARGS=-serial stdio -no-kqemu -fda $(FLOPPYDISK) -hda $(HDD)

DIRS = tools libc services user kernel kernel/test

# warning flags for gcc
export CWFLAGS=-Wall -ansi \
				 -Wextra -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-prototypes \
				 -Wmissing-declarations -Wredundant-decls -Wnested-externs -Winline -Wno-long-long \
				 -Wstrict-prototypes -fno-builtin

.PHONY: all disk dis qemu bochs debug debugu debugm debugt test clean

all: $(BUILD)
		[ -f $(FLOPPYDISK) ] && [ -f $(HDD) ] || make disk;
		@for i in $(DIRS); do \
			make -C $$i all || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done

$(BUILD):
		[ -d $(BUILD) ] || mkdir -p $(BUILD);

disk: clean
		sudo umount $(FLOPPYDISKMOUNT) || true;
		dd if=/dev/zero of=$(FLOPPYDISK) bs=1024 count=1440;
		/sbin/mke2fs -F $(FLOPPYDISK);
		dd if=/dev/zero of=$(HDD) bs=512 count=2880;
		/sbin/mke2fs -F $(HDD)
		sudo mount -o loop $(FLOPPYDISK) $(FLOPPYDISKMOUNT);
		mkdir $(FLOPPYDISKMOUNT)/grub;
		cp boot/stage1 $(FLOPPYDISKMOUNT)/grub;
		cp boot/stage2 $(FLOPPYDISKMOUNT)/grub;
		echo 'default 0' > $(FLOPPYDISKMOUNT)/grub/menu.lst;
		echo 'timeout 0' >> $(FLOPPYDISKMOUNT)/grub/menu.lst;
		echo '' >> $(FLOPPYDISKMOUNT)/grub/menu.lst;
		echo "title $(OSTITLE)" >> $(FLOPPYDISKMOUNT)/grub/menu.lst;
		echo "kernel /$(BINNAME)" >> $(FLOPPYDISKMOUNT)/grub/menu.lst;
		echo 'root (fd0)' >> $(FLOPPYDISKMOUNT)/grub/menu.lst;
		echo -n "device (fd0) $(FLOPPYDISK)\nroot (fd0)\nsetup (fd0)\nquit\n" | grub --batch;
		sudo umount $(FLOPPYDISKMOUNT);

dis: all
		objdump -d -S $(BIN) | less

qemu:	all prepareRun
		qemu $(QEMUARGS) > log.txt 2>&1

bochs: all prepareRun
		bochs -f bochs.cfg -q > log.txt 2>&1

debug: all prepareRun
		qemu $(QEMUARGS) > log.txt 2>&1 &
		@#bochs -f bochs.cfg -q > log.txt 2>&1 &
		sleep 1;
		gdb --command=gdb.start
		@# --symbols $(BIN)

debugu: all prepareRun
		qemu $(QEMUARGS) > log.txt 2>&1 &
		sleep 1;
		gdb --command=gdb.start --symbols $(BUILD)/user_task1.bin

debugc: all prepareRun
		qemu $(QEMUARGS) > log.txt 2>&1 &
		sleep 1;
		gdb --command=gdb.start --symbols $(BUILD)/service_console.bin

debugm: all prepareRun
		qemu $(QEMUARGS) > log.txt 2>&1 &
		@#bochs -f bochs.cfg -q > log.txt 2>&1 &

debugt: all prepareTest
		qemu $(QEMUARGS) > log.txt 2>&1 &

test: all prepareTest
		@#bochs -f bochs.cfg -q > log.txt 2>&1 &
		qemu $(QEMUARGS) > log.txt 2>&1

prepareTest:
		sudo mount -o loop $(FLOPPYDISK) $(FLOPPYDISKMOUNT) || true;
		sed --in-place -e "s/^kernel.*/kernel \/kernel_test.bin/g" $(FLOPPYDISKMOUNT)/grub/menu.lst;
		sudo umount $(FLOPPYDISKMOUNT);

prepareRun:
		sudo mount -o loop $(FLOPPYDISK) $(FLOPPYDISKMOUNT) || true;
		sed --in-place -e "s/^kernel.*/kernel \/kernel.bin/g" $(FLOPPYDISKMOUNT)/grub/menu.lst;
		sudo umount $(FLOPPYDISKMOUNT);

clean:
		@for i in $(DIRS); do \
			make -C $$i clean || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done
