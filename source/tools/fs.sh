#!/bin/bash
ROOT=$(dirname $(dirname $(readlink -f $0)))

if [ $# != 1 ]; then
	echo "Usage: $0 <diskMnt>" 1>&2
	exit 1
fi
if [ "$ARCH" = "" ]; then
	echo '$ARCH is not defined. Please use the root-Makefile to call this shellscript' 1>&2
	exit 1
fi

DISK="$1"

$SUDO mkdir $DISK/bin
$SUDO mkdir $DISK/sbin
$SUDO mkdir $DISK/lib
$SUDO mkdir -p $DISK/etc/keymaps
$SUDO cp -R $ROOT/dist/etc/* $DISK/etc
$SUDO cp -R $ROOT/dist/arch/$ARCH/etc/* $DISK/etc
$SUDO mkdir $DISK/root
$SUDO mkdir -p $DISK/home/hrniels
$SUDO mkdir -p $DISK/home/jon
$SUDO mkdir $DISK/home/hrniels/testdir
$SUDO mkdir $DISK/home/hrniels/scripts
$SUDO cp $ROOT/dist/scripts/* $DISK/home/hrniels/scripts
$SUDO cp $ROOT/dist/testdir/* $DISK/home/hrniels/testdir
$SUDO cp -R $ROOT/kernel/src $DISK
find $DISK/src -type d | grep '.svn' | $SUDO xargs rm -Rf
$SUDO touch $DISK/etc/keymap
$SUDO chmod 0666 $DISK/etc/keymap
echo "/etc/keymaps/ger" > $DISK/etc/keymap
$SUDO touch $DISK/file.txt
$SUDO chmod 0666 $DISK/file.txt
echo "This is a test-string!!!" > $DISK/file.txt
$SUDO dd if=/dev/zero of=$DISK/zeros bs=1024 count=1024

