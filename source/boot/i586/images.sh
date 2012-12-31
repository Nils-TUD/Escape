#!/bin/sh

create_cd() {
	src="$1"
	dst="$2"
	cat > $src/boot/grub/menu.lst <<EOF
default 0
timeout 3

title Escape - VESA-text
kernel /boot/escape videomode=vesa
module /sbin/pci /dev/pci
module /sbin/ata /system/devices/ata nodma
module /sbin/cmos /dev/cmos
module /sbin/fs /dev/fs cdrom iso9660

title Escape - VGA-text
kernel /boot/escape videomode=vga
module /sbin/pci /dev/pci
module /sbin/ata /system/devices/ata nodma
module /sbin/cmos /dev/cmos
module /sbin/fs /dev/fs cdrom iso9660

title Escape - Test
kernel /boot/escape_test videomode=vga
EOF
	
	genisoimage -U -iso-level 3 -input-charset ascii -R -b boot/grub/stage2_eltorito -no-emul-boot \
		-boot-load-size 4 -boot-info-table -o "$dst" "$src" 1>&2
}

create_disk() {
	src="$1"
	dst="$2"
	dir=`mktemp -d`
	cp -R $src/* $dir
	cat > $dir/boot/grub/menu.lst <<EOF
default 0
timeout 3

title Escape - VESA-text
kernel /boot/escape videomode=vesa swapdev=/dev/hda3
module /sbin/pci /dev/pci
module /sbin/ata /system/devices/ata nodma
module /sbin/cmos /dev/cmos
module /sbin/fs /dev/fs /dev/hda1 ext2

title Escape - VGA-text
kernel /boot/escape videomode=vga swapdev=/dev/hda3
module /sbin/pci /dev/pci
module /sbin/ata /system/devices/ata
module /sbin/cmos /dev/cmos
module /sbin/fs /dev/fs /dev/hda1 ext2

title Escape - Test
kernel /boot/escape_test videomode=vga
EOF

	sudo ./boot/perms.sh $dir
	./tools/disk.py create --part ext2r0 64 "$dir" --part ext2r0 4 - --part nofs 8 - "$dst" 1>&2
	sudo rm -Rf $dir
}
