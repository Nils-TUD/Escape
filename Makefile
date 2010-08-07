# general
BUILDDIR = $(abspath build/debug)
DISKMOUNT = disk
HDD = $(BUILDDIR)/hd.img
ISO = $(BUILDDIR)/cd.iso
VBHDDTMP = $(BUILDDIR)/vbhd.bin
VBHDD = $(BUILDDIR)/vbhd.vdi
VMDISK = $(abspath vmware/vmwarehddimg.vmdk)
VBOXOSTITLE = "Escape v0.1"
BINNAME = kernel.bin
BIN = $(BUILDDIR)/$(BINNAME)
SYMBOLS = $(BUILDDIR)/kernel.symbols
BUILDAPPS = $(BUILDDIR)/apps

KVM = -enable-kvm
QEMU = /home/hrniels/Applications/qemu-0.12.2/bin/bin/qemu
QEMUARGS = -serial stdio -hda $(HDD) -cdrom $(BUILD)/cd.iso -boot order=c -vga std -m 20 \
	-localtime
BOCHSDBG = /home/hrniels/Applications/bochs/bochs-2.4.2-gdb/bochs

ifeq ($(BUILDDIR),$(abspath build/debug))
	DIRS = tools lib drivers user kernel/src kernel/test
else
	DIRS = tools lib drivers user kernel/src
endif

# wether to link drivers and user-apps statically or dynamically
export LINKTYPE = static

# number of jobs passing to make
export JOBS =

# flags for gcc
export BUILD = $(BUILDDIR)
export CC = $(abspath build/dist/bin/i586-elf-escape-gcc)
export CPPC = $(abspath build/dist/bin/i586-elf-escape-g++)
export LD = $(abspath build/dist/bin/i586-elf-escape-ld)
export AR = $(abspath build/dist/bin/i586-elf-escape-ar)
export AS = $(abspath build/dist/bin/i586-elf-escape-as)
export CWFLAGS=-Wall -ansi \
				 -Wextra -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-prototypes \
				 -Wmissing-declarations -Wnested-externs -Winline -Wno-long-long \
				 -Wstrict-prototypes -fms-extensions -fno-builtin
export CPPWFLAGS=-Wall -Wextra -Weffc++ -ansi \
				-Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-declarations \
				-Wno-long-long
export DWFLAGS=-w -wi
ifeq ($(BUILDDIR),$(abspath build/debug))
	export CPPDEFFLAGS=$(CPPWFLAGS) -g -D LOGSERIAL -fno-inline
	export CDEFFLAGS=$(CWFLAGS) -g -D LOGSERIAL
	export DDEFFLAGS=$(DWFLAGS) -gc -debug
else
	export CPPDEFFLAGS=$(CPPWFLAGS) -g0 -O3 -D NDEBUG
	export CDEFFLAGS=$(CWFLAGS) -g0 -O3 -D NDEBUG
	export DDEFFLAGS=$(DWFLAGS) -O -release -inline
endif
# flags for gas
export ASFLAGS = --warn
# other
export SUDO=sudo

.PHONY: all debughdd mountp1 mountp2 umountp debugp1 debugp2 checkp1 checkp2 createhdd \
	dis qemu bochs debug debugu debugm debugt test clean updatehdd

all: $(BUILD) $(BUILDAPPS)
		@[ -f $(HDD) ] || make createhdd;
		@for i in $(DIRS); do \
			make $(JOBS) -C $$i all || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done

$(BUILD):
		[ -d $(BUILD) ] || mkdir -p $(BUILD);

$(BUILDAPPS):
		[ -d $(BUILDAPPS) ] || mkdir -p $(BUILDAPPS);

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

updatehdd:
		tools/disk.sh update

createcd:	all
		tools/iso.sh

$(ISO):	all
		tools/iso.sh

$(VMDISK): $(HDD)
		qemu-img convert -f raw $(HDD) -O vmdk $(VMDISK)

swapbl:
		tools/disk.sh swapbl $(BLOCK)

dis:
ifeq ($(APP),)
		objdump -dSC -M intel $(BIN) | less
else
		objdump -dSC -M intel $(BUILD)/$(APP) | less
endif

qemu:	all prepareRun
		sudo /etc/init.d/qemu-kvm start || true
		$(QEMU) $(QEMUARGS) $(KVM) > log.txt 2>&1

bochs: all prepareRun
		bochs -f bochs.cfg -q

vmware: all prepareRun $(ISO) $(VMDISK)
		sudo /etc/init.d/qemu-kvm stop || true # vmware doesn't like kvm :/
		vmplayer vmware/escape.vmx

vbox: all prepareRun $(ISO) $(VMDISK)
		sudo /etc/init.d/qemu-kvm stop || true # vbox doesn't like kvm :/
		tools/vboxhddupd.sh $(VBOXOSTITLE) $(VMDISK)
		VBoxSDL --evdevkeymap -startvm $(VBOXOSTITLE)

debug: all prepareRun
		$(QEMU) $(QEMUARGS) -S -s > log.txt 2>&1 &
		sleep 1;
		/usr/local/bin/gdbtui --command=gdb.start

debugb:	all prepareRun
		$(BOCHSDBG) -f bochsgdb.cfg -q

debugbt:
		$(BOCHSDBG) -f bochsgdb.cfg -q &
		sleep 1;
		/usr/local/bin/gdbtui --command=gdb.start --symbols $(BUILD)/kernel.bin

debugm: all prepareRun
		$(QEMU) $(QEMUARGS) -S -s > log.txt 2>&1 &

debugt: all prepareTest
		$(QEMU) $(QEMUARGS) -S -s > log.txt 2>&1 &

test: all prepareTest
		$(QEMU) $(QEMUARGS) > log.txt 2>&1

testbochs: all prepareTest
		bochs -f bochs.cfg -q

testvbox: all prepareTest $(VMDISK)
		tools/vboxhddupd.sh $(VBOXOSTITLE) $(VMDISK)
		VBoxSDL --evdevkeymap -startvm $(VBOXOSTITLE)

testvmware:	all prepareTest $(VMDISK)
		vmplayer vmware/escape.vmx

prepareTest:
		tools/disk.sh mountp1
		@if [ "`cat $(DISKMOUNT)/boot/grub/menu.lst | grep kernel.bin`" != "" ]; then \
			$(SUDO) sed --in-place -e "s/kernel\.bin\(.*\)/kernel_test.bin\\1/g" \
				$(DISKMOUNT)/boot/grub/menu.lst; \
				touch $(HDD); \
		fi;
		tools/disk.sh unmount

prepareRun:
		tools/disk.sh mountp1
		@if [ "`cat $(DISKMOUNT)/boot/grub/menu.lst | grep kernel_test.bin`" != "" ]; then \
			$(SUDO) sed --in-place -e "s/kernel_test\.bin\(.*\)/kernel.bin\\1/g" \
				$(DISKMOUNT)/boot/grub/menu.lst; \
				touch $(HDD); \
		fi;
		tools/disk.sh unmount

clean:
		@for i in $(DIRS); do \
			make $(JOBS) -C $$i clean || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done
		rm -f $(APPSDB)
