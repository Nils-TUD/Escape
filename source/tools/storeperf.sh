#!/bin/sh

if [ ! -f log.txt ]; then
	echo "Unable to find log.txt" 1>&2
	exit 1
fi

if [ "$ESC_BUILD" = "debug" ]; then
	echo "WARNING: you're in debug mode!"
fi

if [ -z "$ESC_TARGET" ]; then
	ESC_TARGET=i586
fi
if [ -z "$ESC_LINKTYPE" ] || [ "$ESC_TARGET" != "i586" ]; then
	ESC_LINKTYPE=static
fi

platform=""
if [ "$ESC_TARGET" = "i586" ]; then
	if [ "$1" = "" ]; then
		echo "Usage: $0 <platform>" 1>&2
		exit 1
	fi
	platform="$1-"
fi

start="=============== PERFORMANCE TEST START ==============="
end="=============== PERFORMANCE TEST END ==============="

if [ "`grep "$start" log.txt`" = "" ] || [ "`grep "$end" log.txt`" = "" ]; then
	echo "Unable to find start and end pattern of a performance test in log.txt" 1>&2
	exit 1
fi

gawk "
BEGIN {
	inpat = 0
}

/.*/ {
	switch(\$0) {
		case /^$start\$/:
			inpat = 1
			break
		case /^$end\$/:
			inpat = 0
			break
		default:
			if(inpat == 1)
				print \$0
			break
	}
}
" log.txt > perf/$ESC_TARGET-$ESC_LINKTYPE-$platform`date --iso-8601=seconds`.txt

