#!/bin/bash
# A script to change from debug to release mode and vice versa and to a specified architecture

ARCH="$1"
TYPE="$2"
DYN="$3"

if [ $# != 2 ] && [ $# != 3 ] || \
	[ "$TYPE" != "release" ] && [ "$TYPE" != "debug" ] || \
	[ "$ARCH" != "i586" ] && [ "$ARCH" != "eco32" ] && [ "$ARCH" != "mmix" ]; then
	echo "Usage: $0 i586|eco32|mmix release|debug [--dynamic]" 1>&2
	exit 1
fi

if [ "$TYPE" = "release" ]; then
	OLD="debug"
else
	OLD="release"
fi
NEW="$TYPE"

if [ "$ARCH" = "eco32" ]; then
	GCCVER="4.4.3"
elif [ "$ARCH" = "i586" ]; then
	GCCVER="4.4.3"
else
	GCCVER="4.6.0"
fi

if [ "$DYN" = "--dynamic" ]; then
	if [ "$ARCH" != "i586" ]; then
		echo "Dynamic linking is not supported for $ARCH" 1>&2
		exit 1
	fi
	DYN=true
else
	DYN=false
fi

# change build-dir in makefile
sed --in-place -e \
	's/BUILDDIR = \$(abspath build\/\$(ARCH)-'$OLD')/BUILDDIR = \$(abspath build\/\$(ARCH)-'$NEW')/g' \
	Makefile
# change arch and gcc-version
sed --in-place -e 's/ARCH = .*/ARCH = '$ARCH'/g' Makefile
sed --in-place -e 's/GCCVER = .*/GCCVER = '$GCCVER'/g' Makefile
# change linktype
if $DYN; then
	sed --in-place -e 's/export LINKTYPE = .*/export LINKTYPE = dynamic/g' Makefile
else
	sed --in-place -e 's/export LINKTYPE = .*/export LINKTYPE = static/g' Makefile
fi

# now relink everything so that the symlinks in build/dist are changed
rm -f build/$ARCH-$NEW/*.{a,o,so,bin}
make -j8

