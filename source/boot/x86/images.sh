#!/bin/sh

get_suffix() {
	if [ "$ESC_TARGET" = "x86_64" ]; then
		echo -n ".elf32"
	fi
}

create_cd() {
	src="$1"
	dst="$2"
	suffix=`get_suffix`

	cat > $src/boot/grub/grub.cfg <<EOF
set timeout=3
set default=0

menuentry "Escape" {
	multiboot /boot/escape$suffix escape root=/dev/iso9660-cdrom
	module /sbin/initloader initloader
	module /sbin/pci pci /dev/pci
	module /sbin/ata ata /sys/dev/ata nodma
	module /sbin/iso9660 iso9660 /dev/iso9660-cdrom cdrom
	boot
}

menuentry "Escape - Test" {
	multiboot /boot/escape_test$suffix
	module /sbin/initloader initloader
	boot
}
EOF

	grub-mkrescue -d "/usr/lib/grub/i386-pc" \
		--locales="" --fonts="" --themes="" \
		--install-modules="normal part_msdos iso9660 multiboot" \
		-o "$dst" "$src" \
		-- -as mkisofs -U # options for xorriso
}

create_mini_cd() {
	src="$1"
	dst="$2"
	suffix=`get_suffix`

	dir=`mktemp -d`
	cp -Rf $src/* $dir
	if [ "$3" = "1" ]; then
		# remove test-files
		rm -Rf $dir/zeros $dir/home/hrniels/* $dir/home/jon/* $dir/root/*
		# remove test kernel
		rm $dir/boot/escape_test
	fi
	# strip all binaries. just ignore the error for others
	find $dir -type f | xargs strip -s 2>/dev/null
	create_cd $dir $dst $suffix
	rm -Rf $dir
}

create_disk() {
	src="$1"
	dst="$2"
	suffix=`get_suffix`

	dir=`mktemp -d`
	cp -R $src/* $dir
	cat > $dir/boot/grub/grub.cfg <<EOF
set timeout=3
set default=0

menuentry "Escape" {
	multiboot /boot/escape$suffix escape root=/dev/ext2-hda1 swapdev=/dev/hda3
	module /sbin/initloader initloader
	module /sbin/pci pci /dev/pci
	module /sbin/ata ata /sys/dev/ata
	module /sbin/ext2 ext2 /dev/ext2-hda1 /dev/hda1
	boot
}

menuentry "Escape - Test" {
	multiboot /boot/escape_test$suffix
	module /sbin/initloader initloader
	boot
}
EOF

	sudo ./boot/perms.sh $dir
	./tools/disk.py create --offset 2048 --part ext2r0 128 "$dir" --part ext2r0 4 - --part nofs 8 - "$dst" 1>&2
	sudo rm -Rf $dir
}

create_fsimg() {
	src="$1"
	dst="$2"

	dir=`mktemp -d`
	cp -R $src/* $dir

	sudo ./boot/perms.sh $dir

	# determine size and add a bit of space
	size=`sudo du -sm $dir | cut -f 1`
	size=$((size + $size / 5))

	./tools/disk.py create --offset 0 --part ext2r0 $size "$dir" "$dst" --flat --nogrub 1>&2

	# gzip compress the image
	gzip -fk $dst

	sudo rm -Rf $dir
}

create_usbimg() {
	create_fsimg $1/dist $1/fs.img

	suffix=`get_suffix`

	tmp=`mktemp -d`

	# copy files necessary to boot, including the iso image containing everything
	cp -R $1/dist/boot $tmp
	mkdir $tmp/sbin $tmp/bin
	for d in ramdisk ext2 pci initloader; do
		cp $1/dist/sbin/$d $tmp/sbin
	done
	cp $1/fs.img.gz $tmp/boot

	# create grub.cfg
	cat > $dir/boot/grub/grub.cfg <<EOF
set timeout=3
set default=0

menuentry "Escape" {
	multiboot /boot/escape$suffix escape root=/dev/ext2-ramdisk-fs nolog
	module /sbin/initloader initloader
	module /sbin/ramdisk ramdisk /dev/ramdisk-fs -f /sys/boot/fs.img.gz
	module /sbin/pci pci /dev/pci
	module /sbin/ext2 ext2 /dev/ext2-ramdisk-fs /dev/ramdisk-fs
	module /boot/fs.img.gz fs.img.gz
	boot
}

menuentry "Escape - Test" {
	multiboot /boot/escape_test$suffix
	module /sbin/initloader initloader
	boot
}
EOF

	# determine size and add a bit of space
	size=`du -sm $tmp | cut -f 1`
	size=$((size + $size / 2))

	# create disk
	./tools/disk.py create --offset 2048 --part ext3 $size $tmp $1/usb.img
}
