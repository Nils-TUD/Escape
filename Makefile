# general
BUILDDIR = $(abspath build/debug)
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

#KVM = -enable-kvm
QEMU = qemu
QEMUARGS = -serial stdio -hda $(HDD) -cdrom $(ISO) -boot order=d -vga std -m 350 -localtime
BOCHSDBG = /home/hrniels/Applications/bochs/bochs-2.4.2-gdb/bochs

# wether to link drivers and user-apps statically or dynamically
export LINKTYPE = static
# if LINKTYPE = dynamic: wether to use the static or dynamic libgcc (and libgcc_eh)
export LIBGCC = dynamic

# flags for gcc
export BUILD = $(BUILDDIR)
export CC = $(abspath build/dist/bin/i586-elf-escape-gcc)
export CPPC = $(abspath build/dist/bin/i586-elf-escape-g++)
export LD = $(abspath build/dist/bin/i586-elf-escape-ld)
export AR = $(abspath build/dist/bin/i586-elf-escape-ar)
export AS = $(abspath build/dist/bin/i586-elf-escape-as)
export READELF = $(abspath build/dist/bin/i586-elf-escape-readelf)
export OBJDUMP = $(abspath build/dist/bin/i586-elf-escape-objdump)
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

ifeq ($(BUILDDIR),$(abspath build/debug))
	DIRS = tools lib drivers user kernel/src kernel/test
	export CPPDEFFLAGS=$(CPPWFLAGS) -fno-inline -g -D LOGSERIAL
	export CDEFFLAGS=$(CWFLAGS) -g -D LOGSERIAL
	export DDEFFLAGS=$(DWFLAGS) -gc -debug
	export BUILDTYPE=debug
else
	DIRS = tools lib drivers user kernel/src
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
		tools/bochshdd.sh bochs.cfg $(HDD)

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
