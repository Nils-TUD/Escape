#
# Makefile to build the master boot block (with an empty partition table)
#

BUILD = ../../../build

.PHONY:		all install clean

all:		mbootblk

install:	mbootblk
		mkdir -p $(BUILD)/boot
		cp mbootblk $(BUILD)/boot

mbootblk:	stage1/mbr.bin stage2/boot.bin
		dd if=/dev/zero of=./zeroes bs=512 count=1
		cat stage1/mbr.bin zeroes stage2/boot.bin > mbootblk

stage1/mbr.bin:
		$(MAKE) -C stage1

stage2/boot.bin:
		$(MAKE) -C stage2

clean:
		$(MAKE) -C stage1 clean
		$(MAKE) -C stage2 clean
		rm -f *~ zeroes mbootblk
