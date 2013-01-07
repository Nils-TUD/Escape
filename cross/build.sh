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
	echo "Usage: $1 (i586|eco32|mmix) [--rebuild]" >&2
	exit
}

if [ $# -ne 1 ] && [ $# -ne 2 ]; then
	usage $0
fi

ARCH="$1"
if [ "$ARCH" != "i586" ] && [ "$ARCH" != "eco32" ] && [ "$ARCH" != "mmix" ]; then
	usage $0
fi

ROOT=`dirname $(readlink -f $0)`
BUILD=$ROOT/$ARCH/build
DIST=/opt/escape-cross-$ARCH
SRC=$ROOT/$ARCH/src
HEADER=$ROOT/include

if [ "$2" = "--rebuild" ] || [ ! -d $DIST ] || [ ! -d $SRC ]; then
	REBUILD=1
else
	REBUILD=0
fi

echo "Downloading binutils, gcc and newlib..."
if [ "$ARCH" = "eco32" ]; then
	wget -c http://ftp.gnu.org/gnu/binutils/binutils-2.20.1a.tar.bz2
	wget -c http://ftp.gnu.org/gnu/gcc/gcc-4.4.3/gcc-core-4.4.3.tar.bz2
	wget -c http://ftp.gnu.org/gnu/gcc/gcc-4.4.3/gcc-g++-4.4.3.tar.bz2
	wget -c ftp://sources.redhat.com/pub/newlib/newlib-1.15.0.tar.gz
	BINVER=2.20.1
	GCCVER=4.4.3
	NEWLVER=1.15.0
	CUSTOM_FLAGS="-g -O2"
	ENABLE_THREADS=""
else
	wget -c http://ftp.gnu.org/gnu/binutils/binutils-2.21.1a.tar.bz2
	wget -c http://ftp.gnu.org/gnu/gcc/gcc-4.6.3/gcc-core-4.6.3.tar.bz2
	wget -c http://ftp.gnu.org/gnu/gcc/gcc-4.6.3/gcc-g++-4.6.3.tar.bz2
	wget -c ftp://sources.redhat.com/pub/newlib/newlib-1.20.0.tar.gz
	BINVER=2.21.1
	GCCVER=4.6.3
	NEWLVER=1.20.0
	CUSTOM_FLAGS="-g -O2 -D_POSIX_THREADS -D_UNIX98_THREAD_MUTEX_ATTRIBUTES -DPTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP"
	ENABLE_THREADS=" --enable-threads=posix"
fi

GCCCORE_ARCH=gcc-core-$GCCVER.tar.bz2
GCCGPP_ARCH=gcc-g++-$GCCVER.tar.bz2
BINUTILS_ARCH=binutils-"$BINVER"a.tar.bz2
NEWLIB_ARCH=newlib-$NEWLVER.tar.gz

# setup

export PREFIX=$DIST
export TARGET=$ARCH-elf-escape
mkdir -p $BUILD/gcc $BUILD/binutils $BUILD/newlib

# create dist-dir and chown it to the current user. without this we would need sudo to change anything
# which becomes a problem when "make install" wants to use the just built cross-compiler which does
# not by default reside in $PATH. and we can't always change $PATH for sudo (might be forbidden).
$SUDO mkdir -p $DIST
$SUDO chown -R $USER $DIST

# cleanup
if [ $REBUILD -eq 1 ]; then
	if [ -d $SRC ]; then
		if $BUILD_BINUTILS && [ -d $BUILD/binutils ]; then
			cd $BUILD/binutils
			make distclean
			rm -Rf $SRC/binutils
		fi
		if $BUILD_GCC && [ -d $BUILD/gcc ]; then
			cd $BUILD/gcc
			make distclean
			rm -Rf $SRC/gcc
		fi
		if $BUILD_CPP && [ -d $BUILD/newlib ]; then
			cd $BUILD/newlib
			make distclean
			rm -Rf $SRC/newlib
		fi
		cd $ROOT
	fi
	mkdir -p $SRC
fi

# binutils
if $BUILD_BINUTILS; then
	if [ $REBUILD -eq 1 ]; then
		cat $BINUTILS_ARCH | bunzip2 | tar -C $SRC -xf -
		mv $SRC/binutils-$BINVER $SRC/binutils
		cd $ARCH && patch -p0 < binutils.diff
	fi
	cd $BUILD/binutils
	if [ $REBUILD -eq 1 ] || [ ! -f $BUILD/binutils/Makefile ]; then
		$SRC/binutils/configure --target=$TARGET --prefix=$PREFIX --disable-nls --disable-werror
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
	else
		sed --in-place -e 's/#define PTHREAD_MUTEX_INITIALIZER  ((pthread_mutex_t) 0xFFFFFFFF)/#define PTHREAD_MUTEX_INITIALIZER  ((pthread_mutex_t) 0)/g' $DIST/$TARGET/include/pthread.h
	fi
fi

# gcc
export PATH=$PATH:$PREFIX/bin
if $BUILD_GCC; then
	if [ $REBUILD -eq 1 ]; then
		cat $GCCCORE_ARCH | bunzip2 | tar -C $SRC -xf -
		cat $GCCGPP_ARCH | bunzip2 | tar -C $SRC -xf -
		mv $SRC/gcc-$GCCVER $SRC/gcc
		cd $ARCH && patch -p0 < gcc.diff
	fi
	cd $BUILD/gcc
	if [ $REBUILD -eq 1 ] || [ ! -f $BUILD/gcc/Makefile ]; then
		CFLAGS="$CUSTOM_FLAGS" \
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
	if [ "$ARCH" = "i586" ]; then
		# first, generate crt*S.o and libc for libgcc_s. Its not necessary to have a full libc (we'll have
		# one later). But at least its necessary to provide the correct startup-files
		TMPCRT0=`mktemp`
		TMPCRT1=`mktemp`
		TMPCRTN=`mktemp`
		# crt0 can be empty
		echo ".section .init" >> $TMPCRT1
		echo ".global _init" >> $TMPCRT1
		echo "_init:" >> $TMPCRT1
		echo "	push	%ebp" >> $TMPCRT1
		echo "	mov		%esp,%ebp" >> $TMPCRT1
		echo ".section .fini" >> $TMPCRT1
		echo ".global _fini" >> $TMPCRT1
		echo "_fini:" >> $TMPCRT1
		echo "	push	%ebp" >> $TMPCRT1
		echo "	mov		%esp,%ebp" >> $TMPCRT1

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
		CPP=$TARGET-cpp CXXFLAGS=$CUSTOM_FLAGS \
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
