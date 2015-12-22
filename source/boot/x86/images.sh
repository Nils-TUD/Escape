#!/bin/sh

get_suffix() {
	if [ "$ESC_TARGET" = "x86_64" ]; then
		echo -n ".elf32"
	fi
	case "$ESC_SIM_FLAGS" in
		*-enable-kvm*)
			echo -n " forcepit"
			;;
	esac
}

create_cd() {
	src="$1"
	dst="$2"
	suffix=`get_suffix`

	cat > $src/boot/grub/menu.lst <<EOF
default 0
timeout 3

title Escape
kernel /boot/escape$suffix root=/dev/iso9660-cdrom
module /bin/initloader
module /sbin/pci /dev/pci
module /sbin/ata /sys/devices/ata nodma
module /sbin/iso9660 /dev/iso9660-cdrom cdrom

title Escape - Test
kernel /boot/escape_test$suffix
module /bin/initloader
EOF

	genisoimage -U -iso-level 3 -input-charset ascii -R -b boot/grub/stage2_eltorito -no-emul-boot \
		-boot-load-size 4 -boot-info-table -o "$dst" "$src" 1>&2
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
	cat > $dir/boot/grub/menu.lst <<EOF
default 0
timeout 3

title Escape
kernel /boot/escape$suffix root=/dev/ext2-hda1 swapdev=/dev/hda3
module /bin/initloader
module /sbin/pci /dev/pci
module /sbin/ata /sys/devices/ata
module /sbin/ext2 /dev/ext2-hda1 /dev/hda1

title Escape - Test
kernel /boot/escape_test$suffix
module /bin/initloader
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

	sudo rm -Rf $dir
}

create_usbimg() {
	create_fsimg $1/dist $1/fs.img

	suffix=""
	if [ "$ESC_TARGET" = "x86_64" ]; then
		suffix=".elf32"
	fi

	tmp=`mktemp -d`

	# copy files necessary to boot, including the iso image containing everything
	cp -R $1/dist/boot $tmp
	mkdir $tmp/sbin $tmp/bin
	for d in ramdisk ext2 pci; do
		cp $1/dist/sbin/$d $tmp/sbin
	done
	cp $1/dist/bin/initloader $tmp/bin
	cp $1/fs.img $tmp/boot

	# create menu.lst
	cat > $tmp/boot/grub/menu.lst <<EOF
default 0
timeout 3

title Escape
kernel /boot/escape$suffix root=/dev/ext2-ramdisk-fs nosmp nolog
module /bin/initloader
module /sbin/ramdisk /dev/ramdisk-fs -f /sys/boot/fs.img
module /sbin/pci /dev/pci
module /sbin/ext2 /dev/ext2-ramdisk-fs /dev/ramdisk-fs
module /boot/fs.img

title Escape - Test
kernel /boot/escape_test$suffix
module /bin/initloader
EOF

	# determine size and add a bit of space
	size=`du -sm $tmp | cut -f 1`
	size=$((size + $size / 5))

	# create disk
	./tools/disk.py create --offset 2048 --part ext3 $size $tmp $1/usb.img
}
