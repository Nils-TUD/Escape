#!/bin/bash
ROOT=`dirname $(readlink -f $0)`
BUILD=$ROOT/../build/$1
DIST=$ROOT/../build/dist
TARGET=i586-elf-escape

if [ ! $# -eq 1 -o ! -d $BUILD ]; then
	echo "Usage: $0 debug|release" 1>&2
	echo "	Switches from debug to release-mode or the other way around."
	exit 1
fi

# remove old links
rm -f $DIST/$TARGET/lib/*.{a,so}
rm -f $DIST/lib/gcc/$TARGET/4.4.3/{libg.a,crt0.o}

# copy basic libs
ln -sf $BUILD/libg.a $DIST/lib/gcc/$TARGET/4.4.3/libg.a
ln -sf $BUILD/libc_startup.o $DIST/lib/gcc/$TARGET/4.4.3/crt0.o

# copy additional libs
for i in $BUILD/*.{a,so}; do
	if [ "$i" != "$BUILD/libg.a" ]; then
		ln -sf $i $DIST/$TARGET/lib/`basename $i`
	fi
done

