#!/bin/sh

usage() {
	echo "Usage: $1 <logfile> [platform]" 1>&2
	echo "  Note that the platform is mandatory for i586!" 1>&2
	exit 1
}

if [ $# -lt 1 ]; then
	usage $0
fi

if [ "$ESC_BUILD" = "debug" ]; then
	echo "WARNING: you're in debug mode!"
fi
if [ -z "$ESC_TARGET" ]; then
	ESC_TARGET=i586
fi
if [ "$ESC_TARGET" != "i586" ]; then
	ESC_LINKTYPE=static
elif [ -z "$ESC_LINKTYPE" ]; then
	ESC_LINKTYPE=dynamic
fi

logfile="$1"
platform=""
if [ "$ESC_TARGET" = "i586" ]; then
	if [ "$2" = "" ]; then
		usage $0
	fi
	platform="$2-"
fi

start="=============== PERFORMANCE TEST START ==============="
end="=============== PERFORMANCE TEST END ==============="

if [ "`grep "$start" "$logfile"`" = "" ] || [ "`grep "$end" "$logfile"`" = "" ]; then
	echo "Unable to find start and end pattern of a performance test in $logfile" 1>&2
	exit 1
fi

tmp=$(mktemp)
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
" "$logfile" > $tmp

scp $tmp os:escape-perf/$ESC_TARGET-$ESC_LINKTYPE-$platform`date --iso-8601=seconds`.txt
rm $tmp

