# general
BUILD=build
DISKMOUNT=disk
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

QEMUARGS=-serial stdio -no-kqemu -hda $(HDD) -cdrom cdrom.iso -boot c

DIRS = tools libc services user kernel kernel/test

# flags for gcc
export CWFLAGS=-Wall -ansi \
				 -Wextra -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-prototypes \
				 -Wmissing-declarations -Wnested-externs -Winline -Wno-long-long \
				 -Wstrict-prototypes -fno-builtin
export CDEFFLAGS=$(CWFLAGS) -g -D DEBUGGING=1
# flags for nasm
export ASMFLAGS=-g -f elf

.PHONY: all mounthdd debughdd umounthdd createhdd dis qemu bochs debug debugu debugm debugt test clean

all: $(BUILD)
		@[ -f $(HDD) ] || make createhdd;
		@for i in $(DIRS); do \
			make -C $$i all || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done

$(BUILD):
		[ -d $(BUILD) ] || mkdir -p $(BUILD);

mounthdd:
		sudo umount $(DISKMOUNT) > /dev/null 2>&1 || true;
		sudo mount -text2 -oloop=/dev/loop0,offset=`expr $(HDDTRACKSECS) \* 512` $(HDD) $(DISKMOUNT);

debughdd:
		make mounthdd;
		sudo debugfs /dev/loop0
		make umounthdd;

umounthdd:
		sudo umount /dev/loop0 > /dev/null 2>&1

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
		@# add boot stuff
		make mounthdd
		sudo mkdir $(DISKMOUNT)/boot
		sudo mkdir $(DISKMOUNT)/boot/grub
		sudo cp boot/stage1 $(DISKMOUNT)/boot/grub;
		sudo cp boot/stage2 $(DISKMOUNT)/boot/grub;
		sudo touch $(DISKMOUNT)/boot/grub/menu.lst;
		sudo chmod 0666 $(DISKMOUNT)/boot/grub/menu.lst;
		echo 'default 0' > $(DISKMOUNT)/boot/grub/menu.lst;
		echo 'timeout 0' >> $(DISKMOUNT)/boot/grub/menu.lst;
		echo '' >> $(DISKMOUNT)/boot/grub/menu.lst;
		echo "title $(OSTITLE)" >> $(DISKMOUNT)/boot/grub/menu.lst;
		echo "kernel /boot/$(BINNAME)" >> $(DISKMOUNT)/boot/grub/menu.lst;
		echo "module /services/ata services:/ata" >> $(DISKMOUNT)/boot/grub/menu.lst;
		echo "module /services/fs" services:/fs>> $(DISKMOUNT)/boot/grub/menu.lst;
		echo "boot" >> $(DISKMOUNT)/boot/grub/menu.lst;
		echo -n "device (hd0) $(HDD)\nroot (hd0,0)\nsetup (hd0)\nquit\n" | grub --no-floppy --batch;
		@# store some test-data on the disk
		sudo mkdir $(DISKMOUNT)/bin
		sudo mkdir $(DISKMOUNT)/etc
		sudo mkdir $(DISKMOUNT)/services
		sudo cp services/services.txt $(DISKMOUNT)/etc/services
		sudo mkdir $(DISKMOUNT)/testdir
		sudo touch $(DISKMOUNT)/file.txt
		sudo chmod 0666 $(DISKMOUNT)/file.txt
		echo "Das ist ein Test-String!!" > $(DISKMOUNT)/file.txt
		sudo cp $(DISKMOUNT)/file.txt $(DISKMOUNT)/testdir/file.txt
		sudo dd if=/dev/zero of=$(DISKMOUNT)/zeros bs=1024 count=1024
		sudo touch $(DISKMOUNT)/bigfile
		sudo chmod 0666 $(DISKMOUNT)/bigfile
		./tools/createStr.sh 'Das ist der %d Test\n' 200 > $(DISKMOUNT)/bigfile;
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
		gdb --command=gdb.start --symbols $(BUILD)/service_keyboard.bin

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
		make mounthdd
		sudo sed --in-place -e "s/^kernel.*/kernel \/boot\/kernel_test.bin/g" $(DISKMOUNT)/boot/grub/menu.lst;
		make umounthdd

prepareRun:
		make mounthdd
		sudo sed --in-place -e "s/^kernel.*/kernel \/boot\/kernel.bin/g" $(DISKMOUNT)/boot/grub/menu.lst;
		make umounthdd

clean:
		@for i in $(DIRS); do \
			make -C $$i clean || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done
