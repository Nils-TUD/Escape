#!/bin/sh
ROOT=$(dirname $(dirname $(readlink -f $0)))
BUILD=$ROOT/build
DISK=$BUILD/fd.img
DISKMOUNT=$ROOT/disk
OSTITLE="Escape v0.2"
KERNELBIN=$BUILD/kernel.bin

sudo umount $DISKMOUNT || true;
dd if=/dev/zero of=$DISK bs=1024 count=1440;
/sbin/mke2fs -F $DISK;
sudo mount -o loop $DISK $DISKMOUNT;
mkdir $DISKMOUNT/grub;
cp $ROOT/boot/stage1 $DISKMOUNT/grub;
cp $ROOT/boot/stage2 $DISKMOUNT/grub;
echo 'default 0' > $DISKMOUNT/grub/menu.lst;
echo 'timeout 0' >> $DISKMOUNT/grub/menu.lst;
echo '' >> $DISKMOUNT/grub/menu.lst;
echo "title $OSTITLE" >> $DISKMOUNT/grub/menu.lst;
echo "kernel /$BINNAME" >> $DISKMOUNT/grub/menu.lst;
echo 'root (fd0)' >> $DISKMOUNT/grub/menu.lst;
echo -n -e "device (fd0) $DISK\nroot (fd0)\nsetup (fd0)\nquit\n" | /usr/sbin/grub;
cp $KERNELBIN $DISKMOUNT;
sudo umount $DISKMOUNT;
