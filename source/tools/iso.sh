#!/bin/bash
ROOT=$(dirname $(dirname $(readlink -f $0)))
TOOLCHAIN=$ROOT/../toolchain/$ARCH
OSTITLE="Escape v0.3"
if [ "$1" = "test" ]; then
	ISO=$BUILD/cd_test.iso
	BINNAME=kernel_test.bin
else
	ISO=$BUILD/cd.iso
	BINNAME=kernel.bin
fi
KERNELBIN=$BUILD/$BINNAME
TMPDIR=$ROOT/disk
NOVA=0
NOVABINS=/home/hrniels/nova/nul/build/bin/apps
NOVACFGS=/home/hrniels/nova/nul/build

rm -Rf $TMPDIR/*

# establish fs (without sudo)
export SUDO=""
$ROOT/tools/fs.sh $TMPDIR

# copy / create boot-stuff
mkdir -p $TMPDIR/boot/grub 2>/dev/null
cp $ROOT/dist/arch/$ARCH/boot/* $TMPDIR/boot/grub/

if [ $NOVA -eq 1 ]; then
	cp $NOVABINS/{hypervisor,sigma0.nul,vancouver.nul} $TMPDIR/boot
	echo "sigma0::mem:64 sigma0::dma  name::/s0/log name::/s0/timer name::/s0/fs/rom name::/s0/admission name::/s0/disk ||
	rom:///boot/vancouver.nul PC_PS2 ||
	rom:///boot/escape.bin videomode=vga swapdev=/dev/hda3 ||
	rom:///sbin/pci /dev/pci ||
	rom:///sbin/ata /system/devices/ata ||
	rom:///sbin/cmos /dev/cmos ||
	rom:///sbin/fs /dev/fs cdrom iso9660" > $TMPDIR/boot/escape.nulconfig

	echo 'default 0' > $TMPDIR/boot/grub/menu.lst;
	echo 'timeout 3' >> $TMPDIR/boot/grub/menu.lst;
	echo '' >> $TMPDIR/boot/grub/menu.lst;
	echo "title $OSTITLE - VESA-text" >> $TMPDIR/boot/grub/menu.lst;
	echo "kernel /boot/hypervisor serial" >> $TMPDIR/boot/grub/menu.lst;
	echo "module /boot/sigma0.nul tracebuffer_verbose S0_DEFAULT hostserial hostvga hostkeyb:0,0x60,1,12 script_start:1,1 service_config service_disk" >> $TMPDIR/boot/grub/menu.lst;
	echo "module /boot/vancouver.nul" >> $TMPDIR/boot/grub/menu.lst;
	echo "module /boot/escape.nulconfig" >> $TMPDIR/boot/grub/menu.lst;
	if [ "$1" = "test" ]; then
		echo "module /boot/escape_test.bin" >> $TMPDIR/boot/grub/menu.lst;
	else
		if [ "`echo $BUILD | grep $ARCH'-release'`" != "" ]; then
			echo "module /boot/escape.bin" >> $TMPDIR/boot/grub/menu.lst;
		else
			echo "module /boot/escape.bin" >> $TMPDIR/boot/grub/menu.lst;
		fi
	fi
	echo 'module /sbin/pci' >> $TMPDIR/boot/grub/menu.lst;
	echo 'module /sbin/ata' >> $TMPDIR/boot/grub/menu.lst;
	echo 'module /sbin/cmos' >> $TMPDIR/boot/grub/menu.lst;
	echo 'module /sbin/fs' >> $TMPDIR/boot/grub/menu.lst;
	echo '' >> $TMPDIR/boot/grub/menu.lst;
else
	if [ "$ARCH" = "i586" ]; then
		echo 'default 0' > $TMPDIR/boot/grub/menu.lst;
		echo 'timeout 3' >> $TMPDIR/boot/grub/menu.lst;
		echo '' >> $TMPDIR/boot/grub/menu.lst;
		echo "title $OSTITLE - VESA-text" >> $TMPDIR/boot/grub/menu.lst;
		if [ "$1" = "test" ]; then
			echo "kernel /boot/escape_test.bin videomode=vga swapdev=/dev/hda3" >> $TMPDIR/boot/grub/menu.lst;
		else
			if [ "`echo $BUILD | grep $ARCH'-release'`" != "" ]; then
				echo "kernel /boot/escape.bin videomode=vesa swapdev=/dev/hda3" >> $TMPDIR/boot/grub/menu.lst;
			else
				echo "kernel /boot/escape.bin videomode=vesa swapdev=/dev/hda3" >> $TMPDIR/boot/grub/menu.lst;
			fi
		fi
		echo 'module /sbin/pci /dev/pci' >> $TMPDIR/boot/grub/menu.lst;
		echo 'module /sbin/ata /system/devices/ata' >> $TMPDIR/boot/grub/menu.lst;
		echo 'module /sbin/cmos /dev/cmos' >> $TMPDIR/boot/grub/menu.lst;
		echo 'module /sbin/fs /dev/fs cdrom iso9660' >> $TMPDIR/boot/grub/menu.lst;
		echo '' >> $TMPDIR/boot/grub/menu.lst;
	
		echo "title $OSTITLE - VGA-text" >> $TMPDIR/boot/grub/menu.lst;
		if [ "$1" = "test" ]; then
			echo "kernel /boot/escape_test.bin videomode=vga swapdev=/dev/hda3" >> $TMPDIR/boot/grub/menu.lst;
		else
			if [ "`echo $BUILD | grep $ARCH'-release'`" != "" ]; then
				echo "kernel /boot/escape.bin videomode=vga swapdev=/dev/hda3" >> $TMPDIR/boot/grub/menu.lst;
			else
				echo "kernel /boot/escape.bin videomode=vga swapdev=/dev/hda3" >> $TMPDIR/boot/grub/menu.lst;
			fi
		fi
		echo 'module /sbin/pci /dev/pci' >> $TMPDIR/boot/grub/menu.lst;
		echo 'module /sbin/ata /system/devices/ata' >> $TMPDIR/boot/grub/menu.lst;
		echo 'module /sbin/cmos /dev/cmos' >> $TMPDIR/boot/grub/menu.lst;
		echo 'module /sbin/fs /dev/fs cdrom iso9660' >> $TMPDIR/boot/grub/menu.lst;
	fi
fi

# copy kernel
if [ "$1" = "test" ]; then
	cp $KERNELBIN $TMPDIR/boot/escape_test.bin
else
	cp $KERNELBIN $TMPDIR/boot/escape.bin
fi

# copy driver-deps, apps, drivers and user-apps
for i in $BUILD/lib*.so; do
	BASE=`basename $i`
	cp $i $TMPDIR/lib/$BASE
done;
# copy libgcc; its in a different dir
cp $TOOLCHAIN/$TARGET/lib/libgcc_s.so.1 $TMPDIR/lib/libgcc_s.so.1
for i in $BUILD/driver_*.bin ; do
	BASE=`basename $i .bin`
	cp $i $TMPDIR/sbin/`echo $BASE | sed "s/driver_\(.*\)/\1/g"`
done;
for i in $BUILD/user_*.bin ; do
	BASE=`basename $i .bin`
	cp $i $TMPDIR/bin/`echo $BASE | sed "s/user_\(.*\)/\1/g"`
done;

# enable setuid-bit
chmod +s $TMPDIR/bin/{users,groups}

# finally create image and clean up
genisoimage -U -iso-level 3 -input-charset ascii -R -b boot/grub/stage2_eltorito \
	-no-emul-boot -boot-load-size 4 -boot-info-table -o $ISO $TMPDIR
rm -fR $TMPDIR/*

