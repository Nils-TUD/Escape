#!/bin/sh

SUDO=sudo
BUILD_BINUTILS=true
BUILD_GCC=true
BUILD_CPP=true

if [ -f /proc/cpuinfo ]; then
	cpus=`cat /proc/cpuinfo | grep '^processor[[:space:]]*:' | wc -l`
else
	cpus=1
fi
MAKE_ARGS="-j$cpus"

usage() {
	echo "Usage: $1 (i586|x86_64|eco32|mmix) [--rebuild]" >&2
	exit
}

if [ $# -ne 1 ] && [ $# -ne 2 ]; then
	usage $0
fi

ARCH="$1"
if [ "$ARCH" != "i586" ] && [ "$ARCH" != "x86_64" ] &&
	[ "$ARCH" != "eco32" ] && [ "$ARCH" != "mmix" ]; then
	usage $0
fi

ROOT=`dirname $(readlink -f $0)`
BUILD=$ROOT/$ARCH/build
DIST=/opt/escape-cross-$ARCH
SRC=$ROOT/$ARCH/src
HEADER=$ROOT/include
BUILD_CC=gcc

if [ "$2" = "--rebuild" ] || [ ! -d $DIST ] || [ ! -d $SRC ]; then
	REBUILD=1
else
	REBUILD=0
fi

echo "Downloading binutils, gcc and newlib..."
if [ $ARCH = "i586" ] || [ $ARCH = "x86_64" ]; then
	BINVER=2.28
	GCCVER=7.1.0
else
	BINVER=2.23.2
	GCCVER=4.8.2
fi
NEWLVER=1.20.0

wget -c http://ftp.gnu.org/gnu/binutils/binutils-$BINVER.tar.bz2
wget -c http://ftp.gnu.org/gnu/gcc/gcc-$GCCVER/gcc-$GCCVER.tar.bz2
wget -c ftp://sources.redhat.com/pub/newlib/newlib-$NEWLVER.tar.gz
REGPARMS=""
if [ "$ARCH" = "eco32" ] || [ "$ARCH" = "mmix" ]; then
	CUSTOM_FLAGS="-g -O2"
	ENABLE_THREADS=""
else
	CUSTOM_FLAGS="-g -O2 -D_POSIX_THREADS -D_GTHREAD_USE_MUTEX_INIT_FUNC -DPTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP"
	CUSTOM_FLAGS="$CUSTOM_FLAGS -D_UNIX98_THREAD_MUTEX_ATTRIBUTES"
	ENABLE_THREADS=" --enable-threads=posix"
	if [ "$ARCH" = "i586" ]; then
		REGPARMS="-mregparm=3"
		REG_SP="%esp"
		REG_BP="%ebp"
	else
		REG_SP="%rsp"
		REG_BP="%rbp"
	fi
fi

GCC_ARCH=gcc-$GCCVER.tar.bz2
BINUTILS_ARCH=binutils-$BINVER.tar.bz2
NEWLIB_ARCH=newlib-$NEWLVER.tar.gz

# setup

export PREFIX=$DIST
export TARGET=$ARCH-elf-escape

# create dist-dir and chown it to the current user. without this we would need sudo to change anything
# which becomes a problem when "make install" wants to use the just built cross-compiler which does
# not by default reside in $PATH. and we can't always change $PATH for sudo (might be forbidden).
$SUDO mkdir -p $DIST
$SUDO chown -R $USER $DIST

# cleanup
if [ $REBUILD -eq 1 ]; then
	if $BUILD_BINUTILS; then
		rm -Rf $BUILD/binutils $SRC/binutils
	fi
	if $BUILD_GCC; then
		rm -Rf $BUILD/gcc $SRC/gcc
	fi
	if $BUILD_CPP; then
		rm -Rf $BUILD/newlib $SRC/newlib
	fi
	mkdir -p $SRC
fi
mkdir -p $BUILD/gcc $BUILD/binutils $BUILD/newlib

# binutils
if $BUILD_BINUTILS; then
	if [ $REBUILD -eq 1 ]; then
		cat $BINUTILS_ARCH | bunzip2 | tar -C $SRC -xf -
		mv $SRC/binutils-$BINVER $SRC/binutils
		cd $ARCH && patch -p0 < binutils.diff
	fi
	cd $BUILD/binutils
	if [ $REBUILD -eq 1 ] || [ ! -f $BUILD/binutils/Makefile ]; then
		CC=$BUILD_CC $SRC/binutils/configure --target=$TARGET --prefix=$PREFIX --disable-nls --disable-werror
		if [ $? -ne 0 ]; then
			exit 1
		fi
	fi
	make $MAKE_ARGS all && make install
	if [ $? -ne 0 ]; then
		exit 1
	fi
	cd $ROOT
fi

if $BUILD_GCC || $BUILD_CPP; then
	# put the include-files of newlib in the system-include-dir to pretend that we have a full libc
	# this is necessary for libgcc and libsupc++. we'll provide our own version of the few required
	# libc-functions later
	rm -Rf $DIST/$TARGET/include $DIST/$TARGET/sys-include
	mkdir -p tmp
	cat $ROOT/$NEWLIB_ARCH | gunzip | tar -C tmp -xf - newlib-$NEWLVER/newlib/libc/include
	mv tmp/newlib-$NEWLVER/newlib/libc/include $DIST/$TARGET
	rm -Rf tmp

	# the mutexes should be initialized with 0
	if [ "$ARCH" = "eco32" ]; then
		( cd $DIST/$TARGET && patch -p0 < $SRC/../newlib.diff )
	fi
fi

# gcc
export PATH=$PATH:$PREFIX/bin
if $BUILD_GCC; then
	if [ $REBUILD -eq 1 ]; then
		cat $GCC_ARCH | bunzip2 | tar -C $SRC -xf -
		mv $SRC/gcc-$GCCVER $SRC/gcc
		cd $ARCH && patch -p0 < gcc.diff
	fi
	cd $BUILD/gcc
	if [ $REBUILD -eq 1 ] || [ ! -f $BUILD/gcc/Makefile ]; then
		CC=$BUILD_CC CFLAGS="$CUSTOM_FLAGS" CXXFLAGS="$CUSTOM_FLAGS" CFLAGS_FOR_TARGET="$CUSTOM_FLAGS" \
			$SRC/gcc/configure --target=$TARGET --prefix=$PREFIX --disable-nls \
			  --enable-languages=c,c++ --with-headers=$HEADER \
			  --disable-linker-build-id --with-gxx-include-dir=$HEADER/cpp $ENABLE_THREADS
		if [ $? -ne 0 ]; then
			exit 1
		fi
	fi
	make $MAKE_ARGS all-gcc && make install-gcc
	if [ $? -ne 0 ]; then
		exit 1
	fi
	ln -sf $DIST/bin/$TARGET-gcc $DIST/bin/$TARGET-cc

	# libgcc (only i586 supports dynamic linking)
	if [ "$ARCH" = "i586" ] || [ "$ARCH" = "x86_64" ]; then
		# for libgcc, we need crt*S.o and a libc. crt1S.o and crtnS.o need to be working. the others
		# don't need to do something useful, but at least they have to be present.
		TMPCRT0=`mktemp`
		TMPCRT1=`mktemp`
		TMPCRTN=`mktemp`

		# we need the function prologs and epilogs. otherwise the INIT entry in the dynamic section
		# won't be created (and the init- and fini-sections don't work).
		echo ".section .init" >> $TMPCRT1
		echo ".global _init" >> $TMPCRT1
		echo "_init:" >> $TMPCRT1
		echo "	push	$REG_BP" >> $TMPCRT1
		echo "	mov		$REG_SP,$REG_BP" >> $TMPCRT1
		echo ".section .fini" >> $TMPCRT1
		echo ".global _fini" >> $TMPCRT1
		echo "_fini:" >> $TMPCRT1
		echo "	push	$REG_BP" >> $TMPCRT1
		echo "	mov		$REG_SP,$REG_BP" >> $TMPCRT1

		echo ".section .init" >> $TMPCRTN
		echo "	leave" >> $TMPCRTN
		echo "	ret" >> $TMPCRTN
		echo ".section .fini" >> $TMPCRTN
		echo "	leave" >> $TMPCRTN
		echo "	ret" >> $TMPCRTN

		# assemble them
		$TARGET-as -o $DIST/$TARGET/lib/crt0S.o $TMPCRT0 || exit 1
		$TARGET-as -o $DIST/$TARGET/lib/crt1S.o $TMPCRT1 || exit 1
		$TARGET-as -o $DIST/$TARGET/lib/crtnS.o $TMPCRTN || exit 1
		# build empty libc
		$TARGET-gcc -nodefaultlibs -nostartfiles -shared -Wl,-shared -Wl,-soname,libc.so \
		  -o $DIST/$TARGET/lib/libc.so $DIST/$TARGET/lib/crt0S.o || exit 1
		# cleanup
		rm -f $TMPCRT0 $TMPCRT1 $TMPCRTN
	fi

	# now build libgcc
	make $MAKE_ARGS all-target-libgcc && make install-target-libgcc
	if [ $? -ne 0 ]; then
		exit 1
	fi
	cd $ROOT
fi

# libsupc++
if $BUILD_CPP; then
	# libstdc++
	mkdir -p $BUILD/gcc/libstdc++-v3
	cd $BUILD/gcc/libstdc++-v3
	if [ $REBUILD -eq 1 ] || [ ! -f Makefile ]; then
		# pretend that we're using newlib
		CPP=$TARGET-cpp CXXFLAGS="$CUSTOM_FLAGS $REGPARMS" \
			$SRC/gcc/libstdc++-v3/configure --host=$TARGET --prefix=$PREFIX \
			--disable-hosted-libstdcxx --disable-nls --with-newlib
		if [ $? -ne 0 ]; then
		    exit 1
		fi
	fi
	cd include
	make $MAKE_ARGS && make install
	if [ $? -ne 0 ]; then
		exit 1
	fi
	cd ../libsupc++
	make $MAKE_ARGS && make install
	if [ $? -ne 0 ]; then
		exit 1
	fi
fi

# create basic symlinks
rm -Rf $DIST/$TARGET/sys-include $DIST/$TARGET/include
ln -sf $ROOT/../source/include $DIST/$TARGET/sys-include
ln -sf $ROOT/../source/include $DIST/$TARGET/include

# copy crt* to basic gcc-stuff
cp -f $BUILD/gcc/$TARGET/libgcc/crt*.o $DIST/lib/gcc/$TARGET/$GCCVER
