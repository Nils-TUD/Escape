# general
#ARCH = mmix
#GCCVER = 4.6.0
#ARCH = i586
#GCCVER = 4.4.3
ARCH = eco32
GCCVER = 4.4.3
TARGET = $(ARCH)-elf-escape
BUILDDIR = $(abspath build/$(ARCH)-debug)
DIST = ../toolchain/$(ARCH)
DISKMOUNT = diskmnt
HDD = $(BUILDDIR)/hd.img
ISO = $(BUILDDIR)/cd.iso
VBHDDTMP = $(BUILDDIR)/vbhd.bin
VBHDD = $(BUILDDIR)/vbhd.vdi
VMDISK = $(abspath vmware/vmwarehddimg.vmdk)
VBOXOSTITLE = Escape v0.1
BINNAME = kernel.bin
BIN = $(BUILDDIR)/$(BINNAME)
SYMBOLS = $(BUILDDIR)/kernel.symbols

# simulators
KVM = -enable-kvm
QEMU = qemu
QEMUARGS = -serial stdio -hda $(HDD) -cdrom $(ISO) -boot order=d -vga std -m 60 -localtime
BOCHSDBG = /home/hrniels/Applications/bochs/bochs-2.4.2-gdb/bochs
ECO32 = /home/hrniels/Applications/eco32-0.20
ECOSIM = $(ECO32)/build/bin/sim
ECOMON = $(ECO32)/build/monitor/monitor.bin
GIMMIX = /home/hrniels/mmix/gimmix
GIMSIM = $(GIMMIX)/build/gimmix
GIMMON = $(GIMMIX)/build/tests/manual/hexmon.rom

# wether to link drivers and user-apps statically or dynamically
export LINKTYPE = static
# if LINKTYPE = dynamic: wether to use the static or dynamic libgcc (and libgcc_eh)
export LIBGCC = dynamic

# various variables, that are used in many makefiles and shellscripts
export ARCH
export GCCVER
export TARGET
export BUILD = $(BUILDDIR)
export HCC = gcc
export CC = $(abspath $(DIST)/bin/$(TARGET)-gcc)
export CPPC = $(abspath $(DIST)/bin/$(TARGET)-g++)
export LD = $(abspath $(DIST)/bin/$(TARGET)-ld)
export AR = $(abspath $(DIST)/bin/$(TARGET)-ar)
export AS = $(abspath $(DIST)/bin/$(TARGET)-as)
export READELF = $(abspath $(DIST)/bin/$(TARGET)-readelf)
ifeq ($(ARCH),eco32)
export OBJDUMP = /home/hrniels/Applications/gcc/bin/eco32-objdump
else
export OBJDUMP = $(abspath $(DIST)/bin/$(TARGET)-objdump)
endif
export OBJCOPY = $(abspath $(DIST)/bin/$(TARGET)-objcopy)
export NM = $(abspath $(DIST)/bin/$(TARGET)-nm)
export CWFLAGS=-Wall -ansi \
				 -Wextra -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-prototypes \
				 -Wmissing-declarations -Wnested-externs -Winline -Wno-long-long \
				 -Wstrict-prototypes -fms-extensions -fno-builtin
export CPPWFLAGS=-Wall -Wextra -Weffc++ -ansi \
				-Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-declarations \
				-Wno-long-long
export DWFLAGS=-w -wi
export ASFLAGS = --warn
ifeq ($(LIBGCC),static)
	CWFLAGS += -static-libgcc
	CPPWFLAGS += -static-libgcc
endif
export SUDO=sudo

# for profiling:
# ADDFLAGS = -finstrument-functions -DPROFILE\
#		-finstrument-functions-exclude-file-list=../../../lib/basic/profile.c
# ADDLIBS = ../../../lib/basic/profile.c

# build flags, depending on build-type
ifneq ($(BUILDDIR),$(abspath build/$(ARCH)-release))
	#DIRS = tools dist lib drivers user kernel/src kernel/test
	DIRS = tools dist lib drivers user kernel/src
	export CPPDEFFLAGS=$(CPPWFLAGS) -fno-inline -g
	export CDEFFLAGS=$(CWFLAGS) -g -D LOGSERIAL
	export DDEFFLAGS=$(DWFLAGS) -gc -debug
	export BUILDTYPE=debug
else
	DIRS = tools dist lib drivers user kernel/src
	export CPPDEFFLAGS=$(CPPWFLAGS) -g0 -O3 -D NDEBUG
	export CDEFFLAGS=$(CWFLAGS) -g0 -O3 -D NDEBUG
	export DDEFFLAGS=$(DWFLAGS) -O -release -inline
	export BUILDTYPE=release
endif

.PHONY: all debughdd mountp1 mountp2 umountp debugp1 debugp2 checkp1 checkp2 createhdd \
	dis qemu bochs debug debugu debugm debugt test clean updatehdd

all: $(BUILD)
		@[ -f $(HDD) ] || $(MAKE) createhdd;
		@for i in $(DIRS); do \
			$(MAKE) -C $$i all || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done

$(BUILD):
		[ -d $(BUILD) ] || mkdir -p $(BUILD);

hdd:
		$(MAKE) -C dist hdd

cd:
		$(MAKE) -C dist cd

debughdd:
		tools/disk.sh mkdiskdev
		$(SUDO) fdisk /dev/loop0 -C 180 -S 63 -H 16
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
		$(MAKE) -C dist hdd

updatehdd:
		tools/disk.sh update
		$(MAKE) -C dist hdd

createcd:	all
		tools/iso.sh
		$(MAKE) -C dist cd

$(VMDISK): hdd
		qemu-img convert -f raw $(HDD) -O vmdk $(VMDISK)

swapbl:
		tools/disk.sh swapbl $(BLOCK)

dis:
ifeq ($(APP),)
		$(OBJDUMP) -dSC $(BIN) | less
else
		$(OBJDUMP) -dSC $(BUILD)/$(APP) | less
endif

elf:
ifeq ($(APP),)
		$(READELF) -a $(BIN) | less
else
		$(READELF) -a $(BUILD)/$(APP) | less
endif

mmix:	all hdd
		$(GIMSIM) -r $(GIMMON) -t 1 -d $(HDD) -i --script=gimmix.start -s $(BUILD)/stage2.map

mmixd:	all hdd
		$(GIMSIM) -r $(GIMMON) -t 1 -d $(HDD) -p 1235

eco:	all hdd
		$(ECOSIM) -r $(ECOMON) -t 1 -d $(HDD) -o log.txt -c -i \
			-m $(BUILD)/kernel.map $(BUILD)/driver_vterm.map -n

qemu:	all prepareQemu prepareRun
		$(QEMU) $(QEMUARGS) $(KVM) > log.txt 2>&1

bochs: all prepareBochs prepareRun
		bochs -f bochs.cfg -q

vmware: all prepareVmware prepareRun
		vmplayer vmware/escape.vmx

vbox: all prepareVbox prepareRun
		VBoxSDL -startvm "$(VBOXOSTITLE)"

debug: all prepareQemu prepareRun
		$(QEMU) $(QEMUARGS) -S -s > log.txt 2>&1 &
		sleep 1;
		/usr/local/bin/gdbtui --command=gdb.start

debugb:	all prepareBochs prepareRun
		$(BOCHSDBG) -f bochsgdb.cfg -q

debugbt:	all prepareBochs prepareRun
		$(BOCHSDBG) -f bochsgdb.cfg -q &
		sleep 1;
		/usr/local/bin/gdbtui --command=gdb.start --symbols $(BUILD)/kernel.bin

debugm: all prepareQemu prepareRun
		$(QEMU) $(QEMUARGS) -S -s > log.txt 2>&1 &

debugt: all prepareQemu prepareTest
		$(QEMU) $(QEMUARGS) -S -s > log.txt 2>&1 &

test: all prepareQemu prepareTest
		$(QEMU) $(QEMUARGS) > log.txt 2>&1

testbochs: all prepareBochs prepareTest
		bochs -f bochs.cfg -q

testvbox: all prepareVbox prepareTest
		VBoxSDL -startvm "$(VBOXOSTITLE)"

testvmware:	all prepareVmware prepareTest
		vmplayer vmware/escape.vmx

prepareQemu:	hdd cd
		sudo service qemu-kvm start || true

prepareBochs:	hdd cd
		tools/bochshdd.sh bochs.cfg $(HDD) $(ISO)

prepareVbox: cd $(VMDISK)
		sudo service qemu-kvm stop || true # vbox doesn't like kvm :/
		tools/vboxcd.sh $(ISO) "$(VBOXOSTITLE)"
		tools/vboxhddupd.sh "$(VBOXOSTITLE)" $(VMDISK)

prepareVmware: cd $(VMDISK)
		sudo service qemu-kvm stop || true # vmware doesn't like kvm :/
		tools/vmwarecd.sh vmware/escape.vmx $(ISO)

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
			$(MAKE) -C $$i clean || { echo "Make: Error (`pwd`)"; exit 1; } ; \
		done
