# general
BUILD=build
DISKMOUNT=disk
HDD=$(BUILD)/hd.img
VBHDDTMP=$(BUILD)/vbhd.bin
VBHDD=$(BUILD)/vbhd.vdi
HDDBAK=$(BUILD)/hd.img.bak
# 10 MB disk (20 * 16 * 63 * 512 = 10,321,920 byte)
HDDCYL=20
HDDHEADS=16
HDDTRACKSECS=63
TMPFILE=$(BUILD)/disktmp
BINNAME=kernel.bin
BIN=$(BUILD)/$(BINNAME)
SYMBOLS=$(BUILD)/kernel.symbols
OSTITLE=hrniels-OS

QEMUARGS=-serial stdio -hda $(HDD) -boot c -vga std

DIRS = tools libc libcpp services user kernel kernel/test

# flags for gcc
export CWFLAGS=-Wall -ansi \
				 -Wextra -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-prototypes \
				 -Wmissing-declarations -Wnested-externs -Winline -Wno-long-long \
				 -Wstrict-prototypes -fno-builtin
export CPPWFLAGS=-Wall -Wextra -ansi \
				-Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-declarations \
				-Wno-long-long -fno-builtin
export CPPDEFFLAGS=$(CPPWFLAGS) -g -D DEBUGGING=1
export CDEFFLAGS=$(CWFLAGS) -g -D DEBUGGING=1
# flags for nasm
export ASMFLAGS=-f elf
# other
export SUDO=sudo

.PHONY: all mounthdd debughdd umounthdd createhdd dis qemu bochs debug debugu debugm debugt test clean

all: $(BUILD) $(DISKMOUNT)
		@[ -f $(HDD) ] || make createhdd;
		@for i in $(DIRS); do \
			make -C $$i all || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done
		@# just temporary
		qemu-img convert -f raw $(HDD) -O vmdk vmware/vmwarehddimg.vmdk

$(BUILD):
		[ -d $(BUILD) ] || mkdir -p $(BUILD);

$(DISKMOUNT):
		[ -d $(DISKMOUNT) ] || mkdir -p $(DISKMOUNT);

mounthdd: $(DISKMOUNT)
		$(SUDO) umount $(DISKMOUNT) > /dev/null 2>&1 || true;
		$(SUDO) mount -text2 -oloop=/dev/loop0,offset=`expr $(HDDTRACKSECS) \* 512` $(HDD) $(DISKMOUNT);

debughdd:
		make mounthdd;
		$(SUDO) debugfs /dev/loop0
		make umounthdd;

umounthdd:
		tools/umounthdd.sh

# virtual box disk
createvbhdd:
		qemu-img convert $(HDD) $(VBHDDTMP)
		rm -f $(VBHDD)
		VBoxManage convertdd $(VBHDDTMP) $(VBHDD)

createhdd: $(DISKMOUNT) clean
		$(SUDO) umount /dev/loop0 || true
		$(SUDO) losetup -d /dev/loop0 || true
		dd if=/dev/zero of=$(HDD) bs=`expr $(HDDTRACKSECS) \* $(HDDHEADS) \* 512`c count=$(HDDCYL)
		$(SUDO) losetup /dev/loop0 $(HDD) || true
		echo "n" > $(TMPFILE) && \
			echo "p" >> $(TMPFILE) && \
			echo "1" >> $(TMPFILE) && \
			echo "" >> $(TMPFILE) && \
			echo "" >> $(TMPFILE) && \
			echo "w" >> $(TMPFILE);
		$(SUDO) fdisk -u -C$(HDDCYL) -S$(HDDTRACKSECS) -H$(HDDHEADS) /dev/loop0 < $(TMPFILE) || true
		$(SUDO) losetup -d /dev/loop0
		$(SUDO) losetup -o`expr $(HDDTRACKSECS) \* 512` /dev/loop0 $(HDD)
		@# WE HAVE TO CHANGE THE BLOCK-COUNT HERE IF THE DISK-GEOMETRY CHANGES!
		$(SUDO) mke2fs -r0 -Onone -b1024 /dev/loop0 10016
		$(SUDO) umount /dev/loop0 || true
		$(SUDO) losetup -d /dev/loop0 || true
		@# add boot stuff
		make mounthdd
		$(SUDO) mkdir $(DISKMOUNT)/boot
		$(SUDO) mkdir $(DISKMOUNT)/boot/grub
		$(SUDO) cp boot/stage1 $(DISKMOUNT)/boot/grub;
		$(SUDO) cp boot/stage2 $(DISKMOUNT)/boot/grub;
		$(SUDO) touch $(DISKMOUNT)/boot/grub/menu.lst;
		$(SUDO) chmod 0666 $(DISKMOUNT)/boot/grub/menu.lst;
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
		$(SUDO) mkdir $(DISKMOUNT)/bin
		$(SUDO) mkdir $(DISKMOUNT)/etc
		$(SUDO) mkdir $(DISKMOUNT)/services
		$(SUDO) cp services/services.txt $(DISKMOUNT)/etc/services
		$(SUDO) mkdir $(DISKMOUNT)/testdir
		$(SUDO) touch $(DISKMOUNT)/file.txt
		$(SUDO) chmod 0666 $(DISKMOUNT)/file.txt
		echo "Das ist ein Test-String!!" > $(DISKMOUNT)/file.txt
		$(SUDO) cp user/test.bmp $(DISKMOUNT)
		$(SUDO) cp user/bbc.bmp $(DISKMOUNT)
		$(SUDO) cp $(DISKMOUNT)/file.txt $(DISKMOUNT)/testdir/file.txt
		$(SUDO) dd if=/dev/zero of=$(DISKMOUNT)/zeros bs=1024 count=1024
		$(SUDO) touch $(DISKMOUNT)/bigfile
		$(SUDO) chmod 0666 $(DISKMOUNT)/bigfile
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
		gdb --command=gdb.start --symbols $(BUILD)/kernel.bin

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

prepareTest: $(DISKMOUNT)
		make mounthdd
		$(SUDO) sed --in-place -e "s/^kernel.*/kernel \/boot\/kernel_test.bin/g" $(DISKMOUNT)/boot/grub/menu.lst;
		make umounthdd

prepareRun: $(DISKMOUNT)
		make mounthdd
		$(SUDO) sed --in-place -e "s/^kernel.*/kernel \/boot\/kernel.bin/g" $(DISKMOUNT)/boot/grub/menu.lst;
		make umounthdd

clean:
		@for i in $(DIRS); do \
			make -C $$i clean || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done
