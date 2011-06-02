#!/bin/bash
# ---
# A script to change from debug to release mode and vice versa
# ---

ARCH=`cat Makefile | grep '^ARCH =' | cut -f 3 -d ' '`
echo $ARCH

if [ $# != 1 ] || [ "$1" != "release" ] && [ "$1" != "debug" ]; then
	echo "Usage: $0 release|debug" 1>&2
	exit 1
fi

if [ "$1" = "release" ]; then
	OLD="debug"
else
	OLD="release"
fi
NEW="$1"

# change build-dir in makefile
sed --in-place -e \
	's/BUILDDIR = \$(abspath build\/\$(ARCH)-'$OLD')/BUILDDIR = \$(abspath build\/\$(ARCH)-'$NEW')/g' \
	Makefile

# now relink everything so that the symlinks in build/dist are changed
rm -f build/$ARCH-$NEW/*.{a,o,so,bin}
make -j8

