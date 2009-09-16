# general
BUILD = build
DISKMOUNT = disk
HDD = $(BUILD)/hd.img
VBHDDTMP = $(BUILD)/vbhd.bin
VBHDD = $(BUILD)/vbhd.vdi
VMDISK = $(abspath vmware/vmwarehddimg.vmdk)
VBOXOSTITLE = "Escape v0.1"
BINNAME = kernel.bin
BIN = $(BUILD)/$(BINNAME)
SYMBOLS = $(BUILD)/kernel.symbols
BUILDAPPS = $(BUILD)/apps

QEMUARGS = -serial stdio -hda $(HDD) -boot c -vga std

DIRS = tools libc libcpp services user kernel/src kernel/test

# flags for gcc
export CC = gcc
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

.PHONY: all debughdd mountp1 mountp2 umountp debugp1 debugp2 checkp1 checkp2 createhdd \
	dis qemu bochs debug debugu debugm debugt test clean

all: $(BUILD) $(BUILDAPPS)
		@[ -f $(HDD) ] || make createhdd;
		@for i in $(DIRS); do \
			make -C $$i all || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done

$(BUILD):
		[ -d $(BUILD) ] || mkdir $(BUILD);

$(BUILDAPPS):
		[ -d $(BUILDAPPS) ] || mkdir $(BUILDAPPS);

debughdd:
		tools/disk.sh mkdiskdev
		$(SUDO) fdisk /dev/loop0
		tools/disk.sh rmdiskdev

mountp1:
		tools/disk.sh mountp1

mountp2:
		tools/disk.sh mountp2

debugp1:
		tools/disk.sh mountp1
		$(SUDO) debugfs -w /dev/loop0
		tools/disk.sh unmount

debugp2:
		tools/disk.sh mountp2
		$(SUDO) debugfs /dev/loop0
		tools/disk.sh unmount

checkp1:
		tools/disk.sh mountp1
		$(SUDO) fsck /dev/loop0 || true
		tools/disk.sh unmount

checkp2:
		tools/disk.sh mountp2
		$(SUDO) fsck /dev/loop0 || true
		tools/disk.sh unmount

umountp:
		tools/disk.sh unmount

createhdd:
		tools/disk.sh build

$(VMDISK): $(HDD)
		qemu-img convert -f raw $(HDD) -O vmdk $(VMDISK)

dis: all
		objdump -d -S $(BIN) | less

qemu:	all prepareRun
		qemu $(QEMUARGS) > log.txt 2>&1

bochs: all prepareRun
		bochs -f bochs.cfg -q | tee log.txt

vmware: all prepareRun $(VMDISK)
		vmplayer vmware/escape.vmx

vbox: all prepareRun $(VMDISK)
		tools/vboxhddupd.sh $(VBOXOSTITLE) $(VMDISK)
		VBoxSDL --evdevkeymap -startvm $(VBOXOSTITLE)

debug: all prepareRun
		qemu $(QEMUARGS) -S -s > log.txt 2>&1 &
		sleep 1;
		gdbtui --command=gdb.start --symbols $(BUILD)/kernel.bin

debugm: all prepareRun
		qemu $(QEMUARGS) -S -s > log.txt 2>&1 &

debugbochs: all prepareRun
		bochs -f bochs.cfg | tee log.txt

debugt: all prepareTest
		qemu $(QEMUARGS) -S -s > log.txt 2>&1 &

test: all prepareTest
		qemu $(QEMUARGS) > log.txt 2>&1

prepareTest:
		tools/disk.sh mountp1
		@if [ "`cat $(DISKMOUNT)/boot/grub/menu.lst | grep kernel.bin`" != "" ]; then \
			$(SUDO) sed --in-place -e "s/^kernel.*/kernel \/boot\/kernel_test.bin \/appsdb/g" \
				$(DISKMOUNT)/boot/grub/menu.lst; \
				touch $(HDD); \
		fi;
		tools/disk.sh unmount

prepareRun:
		tools/disk.sh mountp1
		@if [ "`cat $(DISKMOUNT)/boot/grub/menu.lst | grep kernel_test.bin`" != "" ]; then \
			$(SUDO) sed --in-place -e "s/^kernel.*/kernel \/boot\/kernel.bin \/appsdb/g" \
				$(DISKMOUNT)/boot/grub/menu.lst; \
				touch $(HDD); \
		fi;
		tools/disk.sh unmount

clean:
		@for i in $(DIRS); do \
			make -C $$i clean || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done
		rm -f $(APPSDB)
