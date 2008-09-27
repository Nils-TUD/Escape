#!/bin/sh
BUILD=build
DISK=$BUILD/disk.img
DISKMOUNT=disk
BINNAME=kernel.bin
BIN=$BUILD/$BINNAME
OSTITLE=hrniels-OS

sudo umount $DISKMOUNT || true;
dd if=/dev/zero of=$DISK bs=1024 count=1440;
/sbin/mke2fs -F $DISK;
sudo mount -o loop $DISK $DISKMOUNT;
mkdir $DISKMOUNT/grub;
cp /boot/grub/stage1 $DISKMOUNT/grub;
cp /boot/grub/stage2 $DISKMOUNT/grub;
echo 'default 0' > $DISKMOUNT/grub/menu.lst;
echo 'timeout 1' >> $DISKMOUNT/grub/menu.lst;
echo '' >> $DISKMOUNT/grub/menu.lst;
echo "title $OSTITLE" >> $DISKMOUNT/grub/menu.lst;
echo "kernel /$BINNAME" >> $DISKMOUNT/grub/menu.lst;
echo 'root (fd0)' >> $DISKMOUNT/grub/menu.lst;
echo -n -e "device (fd0) $DISK\nroot (fd0)\nsetup (fd0)\nquit\n" | /usr/sbin/grub;
make all;
cp $BIN $DISKMOUNT;
sudo umount $DISKMOUNT;