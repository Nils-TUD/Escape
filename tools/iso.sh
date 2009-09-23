#!/bin/bash
ROOT=$(dirname $(dirname $(readlink -f $0)))
BUILD=$ROOT/build
ISO=$BUILD/cd.iso
OSTITLE="Escape v0.2"
BINNAME=kernel.bin
KERNELBIN=$BUILD/$BINNAME
TMPDIR=$ROOT/disk

mkdir -p $TMPDIR/boot/grub
mkdir $TMPDIR/apps
mkdir $TMPDIR/bin
mkdir $TMPDIR/sbin
mkdir $TMPDIR/etc

# copy / create boot-stuff
cp $ROOT/boot/* $TMPDIR/boot/grub/
echo 'default 0' > $TMPDIR/boot/grub/menu.lst;
echo 'timeout 0' >> $TMPDIR/boot/grub/menu.lst;
echo '' >> $TMPDIR/boot/grub/menu.lst;
echo "title $OSTITLE" >> $TMPDIR/boot/grub/menu.lst;
echo "kernel /boot/$BINNAME" >> $TMPDIR/boot/grub/menu.lst;
echo 'module /sbin/ata /services/ata' >> $TMPDIR/boot/grub/menu.lst;
echo 'module /sbin/fs /services/fs /drivers/cda1 iso9660' >> $TMPDIR/boot/grub/menu.lst;

# copy kernel
cp $KERNELBIN $TMPDIR/boot

# copy service-deps, apps, services and user-apps
cp $ROOT/services/services.txt $TMPDIR/etc/services
cp $BUILD/apps/* $TMPDIR/apps
for i in build/service_*.bin ; do
	cp $i $TMPDIR/sbin/`echo $i | sed "s/build\/service_\(.*\)\.bin/\1/g"`
done;
for i in build/user_*.bin ; do
	cp $i $TMPDIR/bin/`echo $i | sed "s/build\/user_\(.*\)\.bin/\1/g"`
done;

# finally create image and clean up
genisoimage -U -iso-level 3 -input-charset ascii -R -b boot/grub/stage2_eltorito -no-emul-boot -boot-load-size 4 \
	-boot-info-table -o $ISO $TMPDIR
rm -fR $TMPDIR/*
