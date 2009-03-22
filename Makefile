# general
BUILD=build
FLOPPYDISK=$(BUILD)/disk.img
FLOPPYDISKMOUNT=disk
HDD=$(BUILD)/hd.img
HDDBAK=$(BUILD)/hd.img.bak
# 5 MB disk (10 * 16 * 63 * 512 = 5,160,960 byte)
HDDCYL=10
HDDHEADS=16
HDDTRACKSECS=63
TMPFILE=$(BUILD)/disktmp
BINNAME=kernel.bin
BIN=$(BUILD)/$(BINNAME)
SYMBOLS=$(BUILD)/kernel.symbols
OSTITLE=hrniels-OS

QEMUARGS=-serial stdio -no-kqemu -fda $(FLOPPYDISK) -hda $(HDD) -boot a

DIRS = tools libc services user kernel kernel/test

# warning flags for gcc
export CWFLAGS=-Wall -ansi \
				 -Wextra -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-prototypes \
				 -Wmissing-declarations -Wredundant-decls -Wnested-externs -Winline -Wno-long-long \
				 -Wstrict-prototypes -fno-builtin

.PHONY: all floppy mounthdd debughdd umounthdd createhdd dis qemu bochs debug debugu debugm debugt test clean

all: $(BUILD)
		@[ -f $(FLOPPYDISK) ] || make floppy;
		@[ -f $(HDD) ] || make createhdd;
		@for i in $(DIRS); do \
			make -C $$i all || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done

$(BUILD):
		[ -d $(BUILD) ] || mkdir -p $(BUILD);

floppy: clean
		sudo umount $(FLOPPYDISKMOUNT) || true;
		dd if=/dev/zero of=$(FLOPPYDISK) bs=1024 count=1440;
		/sbin/mke2fs -F $(FLOPPYDISK);
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

mounthdd:
		sudo umount $(FLOPPYDISKMOUNT) || true;
		sudo mount -text2 -oloop=/dev/loop0,offset=`expr $(HDDTRACKSECS) \* 512` $(HDD) $(FLOPPYDISKMOUNT);

debughdd:
		make mounthdd;
		sudo debugfs /dev/loop0
		make umounthdd;

umounthdd:
		sudo umount /dev/loop0

createhdd: clean
		sudo umount /dev/loop0 || true
		sudo losetup -d /dev/loop0 || true
		dd if=/dev/zero of=$(HDD) bs=`expr $(HDDTRACKSECS) \* $(HDDHEADS) \* 512`c count=$(HDDCYL)
		sudo losetup /dev/loop0 $(HDD) || true
		echo "n" > $(TMPFILE) && \
			echo "p" >> $(TMPFILE) && \
			echo "1" >> $(TMPFILE) && \
			echo "" >> $(TMPFILE) && \
			echo "" >> $(TMPFILE) && \
			echo "w" >> $(TMPFILE);
		sudo fdisk -u -C$(HDDCYL) -S$(HDDTRACKSECS) -H$(HDDHEADS) /dev/loop0 < $(TMPFILE) || true
		sudo losetup -d /dev/loop0
		sudo losetup -o`expr $(HDDTRACKSECS) \* 512` /dev/loop0 $(HDD)
		@# WE HAVE TO CHANGE THE BLOCK-COUNT HERE IF THE DISK-GEOMETRY CHANGES!
		sudo mke2fs -r0 -Onone -b1024 /dev/loop0 5008
		sudo umount /dev/loop0 || true
		sudo losetup -d /dev/loop0 || true
		@# store some test-data on the disk
		make mounthdd
		sudo mkdir $(FLOPPYDISKMOUNT)/apps
		sudo mkdir $(FLOPPYDISKMOUNT)/etc
		sudo mkdir $(FLOPPYDISKMOUNT)/services
		sudo cp services/services.txt $(FLOPPYDISKMOUNT)/etc/services
		sudo mkdir $(FLOPPYDISKMOUNT)/test
		sudo touch $(FLOPPYDISKMOUNT)/file.txt
		sudo chmod 0666 $(FLOPPYDISKMOUNT)/file.txt
		echo "Das ist ein Test-String!!" > $(FLOPPYDISKMOUNT)/file.txt
		sudo cp $(FLOPPYDISKMOUNT)/file.txt $(FLOPPYDISKMOUNT)/test/file.txt
		sudo touch $(FLOPPYDISKMOUNT)/bigfile
		sudo chmod 0666 $(FLOPPYDISKMOUNT)/bigfile
		./tools/createStr.sh 'Das ist der %d Test\n' 200 > $(FLOPPYDISKMOUNT)/bigfile;
		make umounthdd
		rm -f $(TMPFILE)
		cp $(HDD) $(HDDBAK)
		make all

dis: all
		objdump -d -S $(BIN) | less

qemu:	all prepareRun
		qemu $(QEMUARGS) > log.txt 2>&1

bochs: all prepareRun
		bochs -f bochs.cfg -q > log.txt 2>&1

debug: all prepareRun
		qemu $(QEMUARGS) -S -s > log.txt 2>&1 &
		sleep 1;
		gdb --command=gdb.start --symbols $(BUILD)/user_shell.bin

debugu: all prepareRun
		qemu $(QEMUARGS) -S -s > log.txt 2>&1 &
		sleep 1;
		gdb --command=gdb.start --symbols $(BUILD)/service_env.bin

debugc: all prepareRun
		qemu $(QEMUARGS) -S -s > log.txt 2>&1 &
		sleep 1;
		gdb --command=gdb.start --symbols $(BUILD)/service_console.bin

debugm: all prepareRun
		qemu $(QEMUARGS) -S -s > log.txt 2>&1 &
		@#bochs -f bochs.cfg -q > log.txt 2>&1 &

debugt: all prepareTest
		qemu $(QEMUARGS) -S -s > log.txt 2>&1 &

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
