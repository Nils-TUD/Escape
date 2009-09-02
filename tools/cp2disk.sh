#!/bin/bash
ROOT=$(dirname $(dirname $(readlink -f $0)))
HDD=$ROOT/build/hd.img
DISKMOUNT=$ROOT/disk
# 10 MB disk (20 * 16 * 63 * 512 = 10,321,920 byte)
HDDCYL=20
HDDHEADS=16
HDDTRACKSECS=63
SUDO=sudo

# check args
if [ $# -ne 2 ]; then
	echo "Usage: $0 <src> <dst>" 1>&2
	exit 1
fi;

# mount disk
$SUDO umount $DISKMOUNT > /dev/null 2>&1 || true;
$SUDO mount -text2 -oloop=/dev/loop0,offset=`expr $HDDTRACKSECS \* 512` $HDD $DISKMOUNT;

# copy file
$SUDO cp $1 $DISKMOUNT/$2

# unmount
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

# update modify-timestamp of disk-image
touch $HDD
