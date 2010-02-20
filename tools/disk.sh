#!/bin/bash
ROOT=$(dirname $(dirname $(readlink -f $0)))
HDD=$BUILD/hd.img
DISKMOUNT=$ROOT/disk
TMPFILE=$BUILD/disktmp
BINNAME=kernel.bin
OSTITLE="Escape v0.2"
SUDO=sudo

# 20 MB disk (40 * 16 * 63 * 512 = 20,643,840 byte)
HDDCYL=40
HDDHEADS=16
HDDTRACKSECS=63
# partitions
PART1OFFSET=$HDDTRACKSECS
PART1BLOCKS=16000
PART2OFFSET=32063
PART2BLOCKS=3000

# 50 MB disk (100 * 16 * 63 * 512 = 51,609,600 byte)
#HDDCYL=100
#HDDHEADS=16
#HDDTRACKSECS=63
# partitions
#PART1OFFSET=$HDDTRACKSECS
#PART1BLOCKS=20032
#PART2OFFSET=40320
#PART2BLOCKS=10016

# check args
if [ $# -lt 1 ]; then
	echo "Usage: $0 build" 1>&2
	echo "Usage: $0 update" 1>&2
	echo "Usage: $0 mkdiskdev" 1>&2
	echo "Usage: $0 rmdiskdev" 1>&2
	echo "Usage: $0 mountp1" 1>&2
	echo "Usage: $0 mountp2" 1>&2
	echo "Usage: $0 unmount" 1>&2
	echo "Usage: $0 copy <src> <dst>" 1>&2
	echo "Usage: $0 appsdb <src> <dst>" 1>&2
	exit 1
fi;

# create mount-point if not yet present
if [ ! -d $DISKMOUNT ]; then
	mkdir -p $DISKMOUNT;
fi

setupPart() {
	$SUDO losetup -o`expr $1 \* 512` /dev/loop0 $HDD
	$SUDO mke2fs -r0 -Onone -b1024 /dev/loop0 $2
	rmDiskDev
}

buildMenuLst() {
	echo "default 0" > $DISKMOUNT/boot/grub/menu.lst;
	echo "timeout 3" >> $DISKMOUNT/boot/grub/menu.lst;
	echo "" >> $DISKMOUNT/boot/grub/menu.lst;
	echo "title \"$OSTITLE - VESA-text\"" >> $DISKMOUNT/boot/grub/menu.lst;
	echo "kernel /boot/$BINNAME videomode=vesa" >> $DISKMOUNT/boot/grub/menu.lst;
	echo "module /sbin/ata /services/ata" >> $DISKMOUNT/boot/grub/menu.lst;
	echo "module /sbin/fs /services/fs /drivers/hda1 ext2" >> $DISKMOUNT/boot/grub/menu.lst;
	echo "boot" >> $DISKMOUNT/boot/grub/menu.lst;
	echo "" >> $DISKMOUNT/boot/grub/menu.lst;
	echo "title \"$OSTITLE - VGA-text\"" >> $DISKMOUNT/boot/grub/menu.lst;
	echo "kernel /boot/$BINNAME videomode=vga" >> $DISKMOUNT/boot/grub/menu.lst;
	echo "module /sbin/ata /services/ata" >> $DISKMOUNT/boot/grub/menu.lst;
	echo "module /sbin/fs /services/fs /drivers/hda1 ext2" >> $DISKMOUNT/boot/grub/menu.lst;
	echo "boot" >> $DISKMOUNT/boot/grub/menu.lst;
}

addBootData() {
	$SUDO mkdir $DISKMOUNT/boot
	$SUDO mkdir $DISKMOUNT/boot/grub
	$SUDO cp dist/boot/stage1 $DISKMOUNT/boot/grub;
	$SUDO cp dist/boot/stage2 $DISKMOUNT/boot/grub;
	$SUDO touch $DISKMOUNT/boot/grub/menu.lst;
	$SUDO chmod 0666 $DISKMOUNT/boot/grub/menu.lst;
	buildMenuLst;
	echo -n -e "device (hd0) $HDD\nroot (hd0,0)\nsetup (hd0)\nquit\n" | grub --no-floppy '--batch';
}

addTestData() {
	$SUDO mkdir $DISKMOUNT/apps
	$SUDO mkdir $DISKMOUNT/bin
	$SUDO mkdir $DISKMOUNT/sbin
	$SUDO mkdir $DISKMOUNT/etc
	$SUDO mkdir $DISKMOUNT/etc/keymaps
	$SUDO cp dist/services.txt $DISKMOUNT/etc/services
	$SUDO cp dist/keymap-ger.map $DISKMOUNT/etc/keymaps/ger
	$SUDO cp dist/keymap-us.map $DISKMOUNT/etc/keymaps/us
	$SUDO touch $DISKMOUNT/etc/keymap
	$SUDO chmod 0666 $DISKMOUNT/etc/keymap
	echo "/etc/keymaps/ger" > $DISKMOUNT/etc/keymap
	$SUDO cp dist/test.bmp $DISKMOUNT
	$SUDO cp dist/bbc.bmp $DISKMOUNT
	$SUDO cp dist/test.bmp $DISKMOUNT/bla.bmp
	$SUDO cp dist/test.sh $DISKMOUNT/test.sh
	$SUDO cp dist/cursor.bmp $DISKMOUNT/etc
	$SUDO mkdir $DISKMOUNT/testdir
	$SUDO touch $DISKMOUNT/file.txt
	$SUDO chmod 0666 $DISKMOUNT/file.txt
	echo "This is a test-string!!!" > $DISKMOUNT/file.txt
	$SUDO cp $DISKMOUNT/file.txt $DISKMOUNT/testdir/file.txt
	$SUDO dd if=/dev/zero of=$DISKMOUNT/zeros bs=1024 count=1024
	$SUDO touch $DISKMOUNT/bigfile
	$SUDO chmod 0666 $DISKMOUNT/bigfile
	echo -n "" > $DISKMOUNT/bigfile
	i=0
	while [ $i != 200 ]; do
		printf 'Thats the %d test\n' $i >> $DISKMOUNT/bigfile;
		i=`expr $i + 1`;
	done;
	$SUDO cp kernel/src/mem/paging.c $DISKMOUNT/paging.c
}

mkDiskDev() {
	$SUDO umount /dev/loop0 > /dev/null 2>&1 || true
	$SUDO losetup /dev/loop0 $HDD || true
}

rmDiskDev() {
	$SUDO umount /dev/loop0 > /dev/null 2>&1 || true
	$SUDO losetup -d /dev/loop0
}

mountDisk() {
	$SUDO umount $DISKMOUNT > /dev/null 2>&1 || true;
	$SUDO mount -text2 -oloop=/dev/loop0,offset=`expr $1 \* 512` $HDD $DISKMOUNT;
}

unmountDisk() {
	i=0
	$SUDO umount /dev/loop0 > /dev/null 2>&1
	while [ $? != 0 ]; \
	do \
		i=`expr $i + 1`
		if [ $i -ge 10 ]; then
			echo "Unmount failed after $i tries" 1>&2
			exit 1
		fi
		$SUDO umount /dev/loop0 > /dev/null 2>&1
	done
}

# build disk?
if [ "$1" == "build" ]; then
	# create disk
	rmDiskDev
	dd if=/dev/zero of=$HDD bs=`expr $HDDTRACKSECS \* $HDDHEADS \* 512`c count=$HDDCYL
	mkDiskDev
	# create partitions
	echo "n" > $TMPFILE && \
		echo "p" >> $TMPFILE && \
		echo "1" >> $TMPFILE && \
		echo "" >> $TMPFILE && \
		echo `expr $PART2OFFSET - 1` >> $TMPFILE && \
		echo "n" >> $TMPFILE && \
		echo "p" >> $TMPFILE && \
		echo "2" >> $TMPFILE && \
		echo "" >> $TMPFILE && \
		echo "" >> $TMPFILE && \
		echo "w" >> $TMPFILE;
	$SUDO fdisk -u -C$HDDCYL -S$HDDTRACKSECS -H$HDDHEADS /dev/loop0 < $TMPFILE || true
	rmDiskDev
	rm -f $TMPFILE
	
	# build ext2-filesystem
	setupPart $PART1OFFSET $PART1BLOCKS;
	setupPart $PART2OFFSET $PART2BLOCKS;
	
	# add data to disk
	mountDisk $PART1OFFSET
	addBootData
	addTestData
	unmountDisk
	
	# ensure that we'll copy all stuff to the disk with 'make all'
	rm -f $BUILD/*.bin $BUILD/apps/*
	# now rebuild and copy it
	make all
fi

# copy something to disk?
if [ "$1" == "copy" ]; then
	if [ $# -ne 3 ]; then
		echo "Usage: $0 copy <src> <dst>" 1>&2
		exit 1
	fi;
	
	SRC=$2
	DST=$3
	
	# mount partition1
	mountDisk $PART1OFFSET
	
	# copy file
	$SUDO cp $SRC $DISKMOUNT/$DST
	
	# unmount
	unmountDisk
	
	# update modify-timestamp of disk-image
	touch $HDD
fi

# copy all files (except programs etc.) to disk
if [ "$1" == "update" ]; then
	mountDisk $PART1OFFSET
	addTestData
	unmountDisk
fi

# make our disk available under /dev/loop0
if [ "$1" == "mkdiskdev" ]; then
	mkDiskDev
fi

# remove disk from /dev/loop0
if [ "$1" == "rmdiskdev" ]; then
	rmDiskDev
fi

# mount partition 1
if [ "$1" == "mountp1" ]; then
	mountDisk $PART1OFFSET
fi

# mount partition 2
if [ "$1" == "mountp2" ]; then
	mountDisk $PART2OFFSET
fi

# unmount
if [ "$1" == "unmount" ]; then
	unmountDisk
fi
