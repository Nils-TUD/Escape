#!/bin/bash
# ---
# A script to change from debug to release mode and vice versa
# ---

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
	's/BUILDDIR = \$(abspath build\/'$OLD')/BUILDDIR = \$(abspath build\/'$NEW')/g' \
	Makefile

# now relink everything so that the symlinks in build/dist are changed
rm -f build/$NEW/*.{a,o,so,bin}
make

