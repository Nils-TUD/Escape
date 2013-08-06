#!/bin/sh

if [ $# -ne 3 ]; then
	echo "Usage: $0 <crossdir> <cross> <file>" >&2
	exit 1
fi

crossdir=$1
cross=$2
file=$3

x86_64=true
if [ "`$crossdir/bin/$cross-readelf -h \"$file\" | grep ELF32`" != "" ]; then
	x86_64=false
fi

name="\.ctors"
if [ "`$crossdir/bin/$cross-readelf -S "$file" | grep $name`" = "" ]; then
	name="\.init_array"
fi
if [ "`$crossdir/bin/$cross-readelf -S "$file" | grep $name`" = "" ]; then
	echo "Unable to find section init_array/ctors" >&2
	exit 1
fi

if $x86_64; then
	off=0x`$crossdir/bin/$cross-readelf -S "$file" | grep $name | xargs | cut -d ' ' -f 6`
	len=0x`$crossdir/bin/$cross-readelf -S "$file" | grep $name -A1 | grep '^       ' | \
		xargs | cut -d ' ' -f 1`
	bytes=8
else
	section=`$crossdir/bin/$cross-readelf -S "$file" | grep $name | xargs`
	off=0x`echo "$section" | cut -d ' ' -f 6`
	len=0x`echo "$section" | cut -d ' ' -f 7`
	bytes=4
fi

od -t x$bytes "$file" -j $off -N $len -v -w$bytes | grep ' ' | while read line; do
	addr=${line#* }
	$crossdir/bin/$cross-nm -C -l "$file" | grep -m 1 $addr
done
