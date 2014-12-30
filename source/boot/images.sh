#!/bin/sh
create_ecommix_disk() {
	build="$1"
	hd="$2"
	dir=`mktemp -d`
	cp -R $build/dist/* $dir
	sudo ./boot/perms.sh $dir
	if [ "$3" = "test" ]; then
		# use escape_test as kernel
		sudo mv $dir/boot/escape $dir/boot/escape_def
		sudo mv $dir/boot/escape_test $dir/boot/escape
	fi
	./tools/disk.py create --part ext2r0 128 $dir $hd --flat --nogrub 1>&2
	sudo rm -Rf $dir

	dd if=$build/bootloader/$ESC_TARGET/stage1/stage1.bin of=$hd bs=512 count=1 conv=notrunc 1>&2
	if [ "$ESC_TARGET" = "eco32" ]; then
		dd if=$build/bootloader/eco32/stage2/stage2.bin of=$hd bs=512 seek=1 conv=notrunc 1>&2
	else
		dd if=$build/bootloader/mmix/stage2/stage2.elf of=$hd bs=512 seek=1 conv=notrunc 1>&2
	fi
}
