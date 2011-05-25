#!/bin/bash
ROOT=$(dirname $(dirname $(readlink -f $0)))
DIST=$ROOT/build/$ARCH-dist

if [ $# != 1 ]; then
	echo "Usage: $0 <file>" 1>&2
	exit 1
fi

if [[ `basename $1` =~ ^crt(0|1|n|begin|end)S?\.o$ ]]; then
	DIR=$DIST/lib/gcc/$TARGET/4.4.3
else
	DIR=$DIST/$TARGET/lib
fi

rm -f $DIR/`basename $1`
ln -s $1 $DIR/`basename $1`
