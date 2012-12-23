#!/bin/sh
dir="$1"
root=0
hrniels=1
jondoe=2
shared=3

# /boot
chown -R $root:$root $dir/boot
chmod -R 0600 $dir/boot
find $dir/boot -type d | xargs chmod 0755
# /bin
chown -R $root:$root $dir/bin
chmod -R 0755 $dir/bin
chmod u+s $dir/bin/users $dir/bin/groups
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
chmod 0600 $dir/etc/users
# /root
chown -R $root:$root $dir/root
chmod -R 0600 $dir/root
find $dir/root -type d | xargs chmod +x
# /home
chown $root:$root $dir/home
chmod 0755 $dir/home
# /home/hrniels
chown -R $hrniels:$hrniels $dir/home/hrniels
chmod -R 0600 $dir/home/hrniels
find $dir/home/hrniels -type d | xargs chmod +rx
chown -R $hrniels:$shared $dir/home/hrniels/scripts
chmod -R 0750 $dir/home/hrniels/scripts
# /home/jon
chown -R $jondoe:$jondoe $dir/home/jon
chmod -R 0600 $dir/home/jon
find $dir/home/jon -type d | xargs chmod +rx

