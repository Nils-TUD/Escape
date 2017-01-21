#!/bin/sh
dir="$1"
root=0
uhrniels=12
ghrniels=11
ujon=11
gjon=12
gshared=10

# /boot
chown -R $root:$root $dir/boot
chmod -R 0600 $dir/boot
find $dir/boot -type d | xargs chmod 0755

# /bin
chown -R $root:$root $dir/bin
chmod -R 0755 $dir/bin

# /sbin
chown -R $root:$root $dir/sbin
chmod 0755 $dir/sbin
chmod 0744 $dir/sbin/*

# /lib
if [ -d $dir/lib ]; then
	chown -R $root:$root $dir/lib
	chmod 0755 $dir/lib
	chmod 0644 $dir/lib/*
fi

# /etc
chown -R $root:$root $dir/etc
chmod -R 0644 $dir/etc
chmod 0755 $dir/etc
chmod 0755 $dir/etc/keymaps
chmod 0755 $dir/etc/net
chmod 0755 $dir/etc/themes

# /etc/groups
chown -R $root:$root $dir/etc/groups
find $dir/etc/groups -type d | xargs chmod 0755
find $dir/etc/groups -type f | xargs chmod 0644

# /etc/users
chown -R $root:$root $dir/etc/users
find $dir/etc/users -type d | xargs chmod 0755
find $dir/etc/users -type f | xargs chmod 0644
# users can change their own password
chown $uhrniels:$ghrniels $dir/etc/users/hrniels/passwd
chmod 0600 $dir/etc/users/hrniels/passwd
chown $ujon:$gjon $dir/etc/users/jon/passwd
chmod 0600 $dir/etc/users/jon/passwd
chmod 0600 $dir/etc/users/root/passwd

# /root
chown -R $root:$root $dir/root
chmod -R 0600 $dir/root
find $dir/root -type d | xargs chmod +x

# /home
chown $root:$root $dir/home
chmod 0755 $dir/home

# /home/hrniels
chown -R $uhrniels:$ghrniels $dir/home/hrniels
chmod -R 0600 $dir/home/hrniels
find $dir/home/hrniels -type d | xargs chmod +rx
chown -R $uhrniels:$gshared $dir/home/hrniels/scripts
chmod -R 0750 $dir/home/hrniels/scripts

# /home/jon
chown -R $ujon:$gjon $dir/home/jon
chmod -R 0600 $dir/home/jon
find $dir/home/jon -type d | xargs chmod +rx
