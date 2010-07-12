#!/bin/bash
ROOT=`dirname $(readlink -f $0)`
BUILD=$ROOT/../build/cross
DIST=$ROOT/../build/dist
SRC=$ROOT/src
HEADER=$ROOT/../include

GCCCORE_ARCH=gcc-core-4.4.3.tar.bz2
GCCGPP_ARCH=gcc-g++-4.4.3.tar.bz2
BINUTILS_ARCH=binutils-2.20.tar.bz2

if [ "$1" == "--rebuild" ]; then
	REBUILD=1
elif [ ! -d $DIST ]; then
	REBUILD=1
else
	REBUILD=0
fi

# setup
export PREFIX=$DIST
export TARGET=i586-elf-escape
mkdir -p $BUILD/gcc $BUILD/binutils
if [ $REBUILD -eq 1 ]; then
	rm -Rf $SRC $DIST
	mkdir -p $SRC
fi

# binutils
if [ $REBUILD -eq 1 ]; then
	cat $BINUTILS_ARCH | bunzip2 | tar -C $SRC -xf -
	mv $SRC/binutils-2.20 $SRC/binutils
	patch -p0 < binutils.diff
fi
cd $BUILD/binutils
if [ $REBUILD -eq 1 ]; then
	$SRC/binutils/configure --target=$TARGET --prefix=$PREFIX --disable-nls
fi
make all
make install

# gcc
cd $ROOT
if [ $REBUILD -eq 1 ]; then
	cat $GCCCORE_ARCH | bunzip2 | tar -C $SRC -xf -
	cat $GCCGPP_ARCH | bunzip2 | tar -C $SRC -xf -
	mv $SRC/gcc-4.4.3 $SRC/gcc
	patch -p0 < gcc.diff
fi
export PATH=$PATH:$PREFIX/bin
cd $BUILD/gcc
if [ $REBUILD -eq 1 ]; then
	$SRC/gcc/configure --target=$TARGET --prefix=$PREFIX --disable-nls \
		  --enable-languages=c,c++ --with-headers=$HEADER --enable-shared \
		  --disable-linker-build-id
fi
make all-gcc
make install-gcc

# libgcc
make all-target-libgcc
make install-target-libgcc

# create some symlinks
rm -Rf $DIST/$TARGET/sys-include
ln -sf $ROOT/../include $DIST/$TARGET/sys-include
ln -sf $ROOT/../build/debug/libc.so $DIST/$TARGET/lib/libc.so
ln -sf $ROOT/../build/debug/libc.a $DIST/$TARGET/lib/libc.a
ln -sf $ROOT/../build/debug/libg.a $DIST/lib/gcc/$TARGET/4.4.3/libg.a
ln -sf $ROOT/../build/debug/libc_startup.o $DIST/lib/gcc/$TARGET/4.4.3/crt0.o

