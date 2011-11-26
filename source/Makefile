# general
ARCH = i586
GCCVER = 4.4.3
TARGET = $(ARCH)-elf-escape
BUILDDIR = $(abspath build/$(ARCH)-debug)
DIST = ../toolchain/$(ARCH)
DISKMOUNT = disk
HDD = $(BUILDDIR)/hd.img
ISO = $(BUILDDIR)/cd.iso
ISO_TEST = $(BUILDDIR)/cd_test.iso
VBHDDTMP = $(BUILDDIR)/vbhd.bin
VBHDD = $(BUILDDIR)/vbhd.vdi
VMDISK = $(abspath vmware/vmwarehddimg.vmdk)
VBOXOSTITLE = Escape v0.1
KERNELNAME = kernel.bin
KERNEL = $(BUILDDIR)/$(KERNELNAME)
SYMBOLS = $(BUILDDIR)/kernel.symbols

# simulators
KVM = -enable-kvm
QEMU = qemu
#-net nic,model=ne2k_pci -net user
QEMUARGS = -smp 4 -serial stdio -hda $(HDD) -boot order=d -vga std -m 32 -localtime
ECO32 = ../eco32
ECOSIM = $(ECO32)/build/bin/sim
ECOMON = $(ECO32)/build/monitor/monitor.bin
GIMMIX = ../gimmix
GIMSIM = $(GIMMIX)/build/gimmix
GIMMON = $(GIMMIX)/build/tests/manual/hexmon.rom

# whether to link drivers and user-apps statically or dynamically (static | dynamic)
export LINKTYPE = static
# if LINKTYPE = dynamic: whether to use the static or dynamic libgcc (static | dynamic)
export LIBGCC = dynamic

# various variables, that are used in many makefiles and shellscripts
export ARCH
export GCCVER
export TARGET
export BUILD = $(BUILDDIR)
export HCC = gcc
export CC = $(abspath $(DIST)/bin/$(TARGET)-gcc)
export CPP = $(abspath $(DIST)/bin/$(TARGET)-cpp)
export CPPC = $(abspath $(DIST)/bin/$(TARGET)-g++)
export LD = $(abspath $(DIST)/bin/$(TARGET)-ld)
export AR = $(abspath $(DIST)/bin/$(TARGET)-ar)
export AS = $(abspath $(DIST)/bin/$(TARGET)-as)
export READELF = $(abspath $(DIST)/bin/$(TARGET)-readelf)
export OBJDUMP = $(abspath $(DIST)/bin/$(TARGET)-objdump)
export OBJCOPY = $(abspath $(DIST)/bin/$(TARGET)-objcopy)
export NM = $(abspath $(DIST)/bin/$(TARGET)-nm)
export CWFLAGS=-Wall -ansi \
				 -Wextra -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-prototypes \
				 -Wmissing-declarations -Wnested-externs -Winline -Wno-long-long \
				 -Wstrict-prototypes -fms-extensions
export CPPWFLAGS=-Wall -Wextra -Weffc++ -ansi \
				-Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-declarations \
				-Wno-long-long
export DWFLAGS=-w -wi
export ASFLAGS = --warn -I $(abspath include)
ifeq ($(LIBGCC),static)
	CWFLAGS += -static-libgcc
	CPPWFLAGS += -static-libgcc
endif
export SUDO=sudo

# for profiling:
# ADDFLAGS = -finstrument-functions -DPROFILE\
#		-finstrument-functions-exclude-file-list=../../../lib/basic/profile.c
# ADDLIBS = ../../../lib/basic/profile.c

DIRS = tools dist lib drivers user kernel/src kernel/test

# build flags, depending on build-type
ifneq ($(BUILDDIR),$(abspath build/$(ARCH)-release))
	export CPPDEFFLAGS=$(CPPWFLAGS) -fno-inline -g
	export CDEFFLAGS=$(CWFLAGS) -fno-builtin -g -D LOGSERIAL
	export DDEFFLAGS=$(DWFLAGS) -gc -debug
	export BUILDTYPE=debug
else
	export CPPDEFFLAGS=$(CPPWFLAGS) -g0 -O3 -D NDEBUG
	export CDEFFLAGS=$(CWFLAGS) -fno-builtin -g0 -O3 -D NDEBUG
	export DDEFFLAGS=$(DWFLAGS) -O -release -inline
	export BUILDTYPE=release
endif

# atm we have to disable base-addresses, because the mmix-gcc-port seems to be unable to handle it
# correctly
ifeq ($(ARCH),mmix)
CDEFFLAGS += -mno-base-addresses
CPPDEFFLAGS += -mno-base-addresses
# do not warn if instructions are extended to setl,incml,...
ASFLAGS += -x
endif

.PHONY: all debughdd mountp1 mountp2 umountp debugp1 debugp2 checkp1 checkp2 createhdd \
	dis qemu bochs debug debugu debugm debugt test clean updatehdd

all: $(BUILD)
	@for i in $(DIRS); do \
		$(MAKE) -C $$i all || { echo "Make: Error (`pwd`)"; exit 1; } ; \
	done

$(BUILD):
	[ -d $(BUILD) ] || mkdir -p $(BUILD);

hdd:
	@[ -f $(HDD) ] || $(MAKE) createhdd;
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
	$(OBJDUMP) -dSC $(KERNEL) | less
else
	$(OBJDUMP) -dSC $(BUILD)/$(APP) | less
endif

elf:
ifeq ($(APP),)
	$(READELF) -a -W $(KERNEL) | less
else
	$(READELF) -a -W $(BUILD)/$(APP) | less
endif

mmix:	all hdd prepareRun
	$(GIMSIM) -r $(GIMMON) -o log.txt -c -t 1 -d $(HDD) -i --script=gimmix.start \
		-b -s $(BUILD)/kernel.map $(BUILD)/user_dd.map

mmixd:	all hdd prepareRun
	$(GIMSIM) -r $(GIMMON) -o log.txt -c -t 1 -d $(HDD) -p 1234 >/dev/null 2>&1 &
	sleep 1;
	/opt/mmix/bin/mmix-gdbtui --command=gdb.start --symbols $(BUILD)/kernel.bin

testm:	all hdd prepareTest
	$(GIMSIM) -r $(GIMMON) -m 24 -t 1 -d $(HDD) -o log.txt -c -i --script=gimmix.start \
		-b -s $(BUILD)/kernel_test.map

testmd:	all hdd prepareTest
	$(GIMSIM) -r $(GIMMON) -m 24 -t 1 -d $(HDD) -o log.txt -c -p 1234 >/dev/null 2>&1 &
	sleep 1;
	/opt/mmix/bin/mmix-gdbtui --command=gdb.start --symbols $(BUILD)/kernel_test.bin

eco:	all hdd prepareRun
	$(ECOSIM) -r $(ECOMON) -t 1 -d $(HDD) -o log.txt -c -i \
		-m $(BUILD)/kernel.map $(BUILD)/user_test.map -n

teste:	all hdd prepareTest
	$(ECOSIM) -r $(ECOMON) -t 1 -d $(HDD) -o log.txt -c -i \
		-m $(BUILD)/kernel_test.map -n

qemu:	all prepareQemu prepareRun
	$(QEMU) $(QEMUARGS) -cdrom $(ISO) $(KVM) > log.txt 2>&1

bochs: all prepareBochs prepareRun
	bochs -f bochs.cfg -q

vmware: all prepareVmware prepareRun
	vmplayer vmware/escape.vmx

vbox: all prepareVbox prepareRun
	VBoxSDL -startvm "$(VBOXOSTITLE)"

debug: all prepareQemu prepareRun
	$(QEMU) $(QEMUARGS) -cdrom $(ISO) -S -s > log.txt 2>&1 &
	sleep 1;
	/usr/bin/gdbtui --command=gdb.start

debugq: all prepareQemu prepareRun
	$(QEMU) $(QEMUARGS) -cdrom $(ISO) -S -s > log.txt 2>&1 &

debugqt: all prepareQemuTest prepareTest
	$(QEMU) $(QEMUARGS) -cdrom $(ISO_TEST) -S -s > log.txt 2>&1 &

testq: all prepareQemuTest
	$(QEMU) $(QEMUARGS) -cdrom $(ISO_TEST) > log.txt 2>&1

testbochs: all prepareBochsTest
	bochs -f bochs.cfg -q

testvbox: all prepareVbox prepareTest
	VBoxSDL -startvm "$(VBOXOSTITLE)"

testvmware:	all prepareVmware prepareTest
	vmplayer vmware/escape.vmx

prepareQemu:	hdd cd
	sudo service qemu-kvm start || true

prepareBochs:	hdd cd
	tools/bochshdd.sh bochs.cfg $(HDD) $(ISO) $(BUILDDIR)/kernel.map

prepareBochsTest:
	tools/bochshdd.sh bochs.cfg $(HDD) $(ISO_TEST) $(BUILDDIR)/kernel_test.map

prepareVbox: cd $(VMDISK)
	sudo service qemu-kvm stop || true # vbox doesn't like kvm :/
	tools/vboxcd.sh $(ISO) "$(VBOXOSTITLE)"
	tools/vboxhddupd.sh "$(VBOXOSTITLE)" $(VMDISK)

prepareVmware: cd $(VMDISK)
	sudo service qemu-kvm stop || true # vmware doesn't like kvm :/
	tools/vmwarecd.sh vmware/escape.vmx $(ISO)

prepareQemuTest:
	sudo service qemu-kvm start || true
	tools/iso.sh test

prepareTest:
	tools/disk.sh mountp1
	$(SUDO) cp $(BUILD)/kernel_test.bin $(DISKMOUNT)/boot/kernel.bin
	tools/disk.sh unmount

prepareRun:
	tools/disk.sh mountp1
	$(SUDO) cp $(BUILD)/kernel.bin $(DISKMOUNT)/boot/kernel.bin
	tools/disk.sh unmount

clean:
	@for i in $(DIRS); do \
		$(MAKE) -C $$i clean || { echo "Make: Error (`pwd`)"; exit 1; } ; \
	done
