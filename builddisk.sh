#!/bin/sh
BUILD=build
DISK=$BUILD/disk.img
DISKMOUNT=disk
BINNAME=kernel.bin
KERNELBIN=$BUILD/$BINNAME
OSTITLE=hrniels-OS
USERTITLE=Task1
USERNAME=user_task1.bin
USERBIN=$BUILD/$USERNAME

sudo umount $DISKMOUNT || true;
dd if=/dev/zero of=$DISK bs=1024 count=1440;
/sbin/mke2fs -F $DISK;
sudo mount -o loop $DISK $DISKMOUNT;
mkdir $DISKMOUNT/grub;
cp boot/stage1 $DISKMOUNT/grub;
cp boot/stage2 $DISKMOUNT/grub;
echo 'default 0' > $DISKMOUNT/grub/menu.lst;
echo 'timeout 0' >> $DISKMOUNT/grub/menu.lst;
echo '' >> $DISKMOUNT/grub/menu.lst;
echo "title $OSTITLE" >> $DISKMOUNT/grub/menu.lst;
echo "kernel /$BINNAME" >> $DISKMOUNT/grub/menu.lst;
echo 'root (fd0)' >> $DISKMOUNT/grub/menu.lst;
#echo '' >> $DISKMOUNT/grub/menu.lst;
#echo "title $USERTITLE" >> $DISKMOUNT/grub/menu.lst;
#echo "kernel /$USERNAME" >> $DISKMOUNT/grub/menu.lst;
#echo 'root (fd0)' >> $DISKMOUNT/grub/menu.lst;
echo -n "device (fd0) $DISK\nroot (fd0)\nsetup (fd0)\nquit\n" | grub --batch;
sudo umount $DISKMOUNT;
make all;
sudo mount -o loop $DISK $DISKMOUNT;
cp $KERNELBIN $DISKMOUNT;
#cp $USERBIN $DISKMOUNT;
sudo umount $DISKMOUNT;