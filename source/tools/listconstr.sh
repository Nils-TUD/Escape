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

show_symbols() {
	name=$1
	namepat="\\$name"

	rdelf=$crossdir/bin/$cross-readelf
	if [ "`$rdelf -S "$file" | grep $namepat`" != "" ]; then
		if $x86_64; then
			off=0x`$rdelf -S "$file" | grep $namepat | sed -e 's/\[.*\]//g' | xargs | cut -d ' ' -f 4`
			len=0x`$rdelf -S "$file" | grep $namepat -A1 | grep '^       ' | xargs | cut -d ' ' -f 1`
			bytes=8
		else
			section=`$rdelf -S "$file" | grep $namepat | sed -e 's/\[.*\]//g' | xargs`
			off=0x`echo "$section" | cut -d ' ' -f 4`
			len=0x`echo "$section" | cut -d ' ' -f 5`
			bytes=4
		fi

		echo "Constructors in $name ($off : $len):"
		od -t x$bytes "$file" -j $off -N $len -v -w$bytes | grep ' ' | while read line; do
			addr=${line#* }
			$crossdir/bin/$cross-nm -C -l "$file" | grep -m 1 $addr
		done
	fi
}

for name in ".init_array" ".ctors"; do
	show_symbols $name
done
