#!/bin/bash

SVNURL="http://hs1.whm-webhosting.de/usvn/svn/OS/trunk"
SVNOFF="/home/hrniels/escape/source"
HDD="build/release/hd.img"
CD="build/release/cd.iso"
LICENSE=LICENSE

if [ $# -lt 2 ]; then
	echo "Usage: $0 <targetDir> <version> [--offline]" 1>&2
	exit
fi

DEST="$1"
DESTSRC="$DEST/src"
VERSION="$2"

if [ ! -d "$DEST" ]; then
	echo "'$DEST' is no directory" 1>&2
	exit
fi

if [ "$3" = "--offline" ]; then
	SRC="$SVNOFF"
else
	SRC="$SVNURL"
fi

echo -n "Checking out Escape..."
svn export "$SRC" "$DESTSRC" --force >/dev/null
echo "done"

echo -n "Removing unnecessary files and folders..."
rm -Rf "$DESTSRC/build" "$DESTSRC/.settings" "$DESTSRC/disk" "$DESTSRC/doc" "$DESTSRC/tools/release.sh"
for i in "$DESTSRC"/vmware/*; do
	if [ "$i" != "$DESTSRC/vmware/escape.vmx" ]; then
		rm -f "$i"
	fi
done
rm -f "$DESTSRC/errors.txt" "$DESTSRC/float" "$DESTSRC/float.c" "$DESTSRC/license.php"
rm -f "$DESTSRC/log.txt" "$DESTSRC/.cproject" "$DESTSRC/.project"
echo "done"

echo -n "Creating source-zip..."
OLDDIR=`pwd`
cd $DESTSRC
zip --quiet -r "../escape-src-$VERSION.zip" .
cd $OLDDIR
rm -Rf "$DESTSRC"
echo "done"

echo -n "Creating disk, cd and vmdk-disk..."
make createhdd createcd >/dev/null 2>&1
qemu-img convert -f raw "$HDD" -O vmdk "$DEST/escape-hdd.vmdk" >/dev/null 2>&1
echo "done"

echo -n "Creating other zips..."
cp "$CD" "$DEST/escape-cd.iso"
zip --quiet -j "$DEST/escape-cd-img-$VERSION.zip" "$DEST/escape-cd.iso" "$LICENSE"
rm -f "$DEST/escape-cd.iso"

cp "$HDD" "$DEST/escape-hdd.img"
zip --quiet -j "$DEST/escape-hdd-img-$VERSION.zip" "$DEST/escape-hdd.img" "$LICENSE"
rm -f "$DEST/escape-hdd.img"

zip --quiet -j "$DEST/escape-vmdk-img-$VERSION.zip" "$DEST/escape-hdd.vmdk" "$LICENSE"
rm -f "$DEST/escape-hdd.vmdk"
echo "done"

