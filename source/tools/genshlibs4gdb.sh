#!/bin/sh

if [ $# -lt 2 ]; then
	echo "Usage: $0 <prog> <libpath>..." 1>&2
	exit 1
fi

pagesize=4096
# dynlink allocates 64K at the beginning. there is a bit meta-information around, therefore one page
# more and one page for the areas.
dynlinkheap=$((1024 * 64 + 2 * 4096))
prog=$1
libdirs=""
shift
while [ $# -gt 0 ]; do
	libdirs="$libdirs $1"
	shift
done

getlibs() {
	readelf -d $1 | grep '(NEEDED)' | sed -e 's/^.*Shared library: \[\(.*\)\]$/\1/g'
}

findbinary() {
	for dir in $libdirs; do
		if [ -f $dir/$1 ]; then
			echo $dir/$1
			return
		fi
	done
	echo "Unable to find binary $1" 1>&2
	exit 1
}

getProgEnd() {
	line=`readelf -l $1 | grep LOAD | tail -n 1`
	start=`echo $line | xargs | cut -d ' ' -f 3`
	len=`echo $line | xargs | cut -d ' ' -f 6`
	end=$((((start + len + $pagesize - 1) / 4096) * 4096))
	echo $end
}

# find all dependencies; in the same way our dynlink does it
progdeps=`getlibs $prog`
alllibs=""

findDeps() {
	for l in $1; do
		if [ "`echo $alllibs | grep $l`" = "" ]; then
			alllibs="$alllibs $l"
		fi

		lib=`findbinary $l`
		findDeps "`getlibs $lib`"
	done
}

findDeps "$progdeps"

# determine start address
dynlink=`findbinary dynlink`
dynlinkEnd=$((`getProgEnd $dynlink` + $dynlinkheap))
alllibs="$alllibs"

# add dynlink as well
#textaddr=`readelf -S $dynlink | grep .text | xargs | cut -d ' ' -f 5`
#printf "add-symbol-file %s %#x\n" $dynlink 0x$textaddr

# generate gdb commands
addr=$dynlinkEnd
for l in $alllibs; do
	lib=`findbinary $l`
	textaddr=`readelf -S $lib | grep .text | xargs | cut -d ' ' -f 5`
	end=`getProgEnd $lib`

	start=$((0x$textaddr + addr))
	printf "add-symbol-file %s %#x\n" $lib $start
	addr=$((addr + end))
done
