#!/bin/bash
BUILD=build
AS=./mmixal
MMIX=./mmix

ARGS=
START=2
PROG=$1
if [ $# -lt 1 ]; then
	echo "Usage: $0 [-a argsToMMIX] <prog> ..." 1>&2
	exit 1
fi
if [ "$1" = "-a" ]; then
	START=4
	PROG=$3
	ARGS=$2
fi

$AS -x -b 250 -l "$BUILD/$PROG.mml" -o "$BUILD/$PROG.mmo" "apps/$PROG.mms" \
 && $MMIX $ARGS "$BUILD/$PROG.mmo" ${@:$START:$#}
