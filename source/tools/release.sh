#!/bin/bash

SVNURL="http://hs1.whm-webhosting.de/usvn/svn/OS/trunk"
SVNOFF="/home/hrniels/escape"
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
rm -Rf "$DESTSRC/doc" "$DESTSRC/cross/eco32/"{binutils-2.19.diff,gcc-4.4.0.diff}
rm -Rf "$DESTSRC"/source/{build,.csettings,.settings,disk,diskmnt,tools/release.sh}
for i in "$DESTSRC"/source/vmware/*; do
	if [ "$i" != "$DESTSRC/source/vmware/escape.vmx" ]; then
		rm -f "$i"
	fi
done
rm -f "$DESTSRC/source/"{license.php,log.txt,.project,.cproject,gstlfilt.pl}
rm -Rf "$DESTSRC/source/user/d" "$DESTSRC/source/lib/d"
echo "done"

echo -n "Creating source-zip..."
OLDDIR=`pwd`
cd $DESTSRC
rm -f "../escape-src-complete-$VERSION.zip"
zip --quiet -r "../escape-src-complete-$VERSION.zip" .
rm -Rf eco32 gimmix
rm -f "../escape-src-$VERSION.zip"
zip --quiet -r "../escape-src-$VERSION.zip" .
cd $OLDDIR
rm -Rf "$DESTSRC"
echo "done"

exit 0

for i in i586 eco32 mmix; do
	echo -n "Switching to $i..."
	if [ "$i" = "i586" ]; then
		./switch.sh $i release --dynamic > /dev/null 2>&1
	else
		./switch.sh $i release > /dev/null 2>&1
	fi
	echo "done"
	
	echo -n "Building disk, cd and vmdk-disk..."
	make createhdd > /dev/null
	cp build/$i-release/hd.img "$DEST/escape-$i-hdd.img"
	if [ "$i" = "i586" ]; then
		make createcd > /dev/null 2>&1
		cp build/$i-release/cd.iso "$DEST/escape-$i-cd.iso"
		qemu-img convert -f raw build/$i-release/hd.img -O vmdk "$DEST/escape-$i-hdd.vmdk" >/dev/null 2>&1
	fi
	echo "done"
	
	echo -n "Zipping disk and cd..."
	zip --quiet -j "$DEST/escape-$i-hdd-$VERSION.zip" "$DEST/escape-$i-hdd.img" "$LICENSE"
	rm -f "$DEST/escape-$i-hdd.img"
	if [ "$i" = "i586" ]; then
		zip --quiet -j "$DEST/escape-$i-cd-$VERSION.zip" "$DEST/escape-$i-cd.iso" "$LICENSE"
		rm -f "$DEST/escape-$i-cd.iso"
		zip --quiet -j "$DEST/escape-$i-vmdk-$VERSION.zip" "$DEST/escape-$i-hdd.vmdk" "$LICENSE"
		rm -f "$DEST/escape-$i-hdd.vmdk"
	fi
	echo "done"
done

