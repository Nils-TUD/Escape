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
TMPDIR=$ROOT/diskmnt

sudo rm -Rf $TMPDIR/*
mkdir -p $TMPDIR/boot/grub
mkdir $TMPDIR/apps
mkdir $TMPDIR/bin
mkdir $TMPDIR/sbin
mkdir $TMPDIR/lib
mkdir $TMPDIR/etc
mkdir $TMPDIR/etc/keymaps
mkdir $TMPDIR/testdir

# copy / create boot-stuff
cp $ROOT/dist/arch/$ARCH/boot/* $TMPDIR/boot/grub/
echo 'default 0' > $TMPDIR/boot/grub/menu.lst;
echo 'timeout 3' >> $TMPDIR/boot/grub/menu.lst;
echo '' >> $TMPDIR/boot/grub/menu.lst;
echo "title $OSTITLE - VESA-text" >> $TMPDIR/boot/grub/menu.lst;
if [ "$1" = "test" ]; then
	echo "kernel /boot/escape_test.bin videomode=vga swapdev=/dev/hda3" >> $TMPDIR/boot/grub/menu.lst;
else
	if [ "`echo $BUILD | grep '/release'`" != "" ]; then
		echo "kernel /boot/escape.bin videomode=vesa swapdev=/dev/hda3 nolog" >> $TMPDIR/boot/grub/menu.lst;
	else
		echo "kernel /boot/escape.bin videomode=vesa swapdev=/dev/hda3 " >> $TMPDIR/boot/grub/menu.lst;
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
	if [ "`echo $BUILD | grep '/release'`" != "" ]; then
		echo "kernel /boot/escape.bin videomode=vga swapdev=/dev/hda3 nolog" >> $TMPDIR/boot/grub/menu.lst;
	else
		echo "kernel /boot/escape.bin videomode=vga swapdev=/dev/hda3 " >> $TMPDIR/boot/grub/menu.lst;
	fi
fi
echo 'module /sbin/pci /dev/pci' >> $TMPDIR/boot/grub/menu.lst;
echo 'module /sbin/ata /system/devices/ata' >> $TMPDIR/boot/grub/menu.lst;
echo 'module /sbin/cmos /dev/cmos' >> $TMPDIR/boot/grub/menu.lst;
echo 'module /sbin/fs /dev/fs cdrom iso9660' >> $TMPDIR/boot/grub/menu.lst;

# copy kernel
if [ "$1" = "test" ]; then
	cp $KERNELBIN $TMPDIR/boot/escape_test.bin
else
	cp $KERNELBIN $TMPDIR/boot/escape.bin
fi

# copy driver-deps, apps, drivers and user-apps
cp -R $ROOT/kernel/src $TMPDIR
cp $ROOT/dist/arch/$ARCH/boot/* $TMPDIR/boot/grub
cp $ROOT/dist/etc/* $TMPDIR/etc
cp $ROOT/dist/arch/$ARCH/etc/* $TMPDIR/etc
cp $ROOT/dist/etc/keymaps/* $TMPDIR/etc/keymaps
cp $ROOT/dist/scripts/* $TMPDIR/scripts
cp $ROOT/dist/testdir/* $TMPDIR/testdir
echo "/etc/keymaps/ger" > $TMPDIR/etc/keymap
cp $BUILD/apps/* $TMPDIR/apps
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
cp $ROOT/kernel/src/mem/paging.c $TMPDIR/paging.c

# add some test-data
echo "Das ist ein Test-String!!" > $TMPDIR/file.txt
cp -R $ROOT/dist/scripts $TMPDIR
dd if=/dev/zero of=$TMPDIR/zeros bs=1024 count=1024
echo -n "" > $TMPDIR/bigfile
i=0
while [ $i != 200 ]; do
	printf 'Das ist der %d Test\n' $i >> $TMPDIR/bigfile;
	i=`expr $i + 1`;
done;

# finally create image and clean up
genisoimage -U -iso-level 3 -input-charset ascii -R -b boot/grub/stage2_eltorito \
	-no-emul-boot -boot-load-size 4 -boot-info-table -o $ISO $TMPDIR
rm -fR $TMPDIR/*
