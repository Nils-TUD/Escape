#!/bin/bash

SVNURL="http://hs1.whm-webhosting.de/usvn/svn/OS/trunk"
SVNOFF="/home/hrniels/escape"
LICENSE=../LICENSE

if [ $# -lt 2 ]; then
	echo "Usage: $0 <targetDir> <version> [--offline]" 1>&2
	exit
fi

DEST="$1"
VERSION="$2"
SRCDIR="escape-$VERSION"
DESTSRC="$DEST/$SRCDIR"

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
rm -Rf "$DESTSRC"/source/{build,.csettings,.settings,tools/release.sh}
for i in "$DESTSRC"/source/vmware/*; do
	if [ "$i" != "$DESTSRC/source/vmware/escape.vmx" ]; then
		rm -f "$i"
	fi
done
rm -f "$DESTSRC/source/"{license.php,log.txt,.project,.cproject,gstlfilt.pl}
rm -f "$DESTSRC/eco32/"{.project,.cproject} "$DESTSRC/gimmix/"{.project,.cproject}
rm -Rf "$DESTSRC/source/user/d" "$DESTSRC/source/lib/d"
echo "done"

echo -n "Creating source-zip..."
OLDDIR=`pwd`
cd $DEST
rm -f "escape-src-complete-$VERSION.tar.bz2"
tar -jcvf "escape-src-complete-$VERSION.tar.bz2" $SRCDIR > /dev/null
rm -Rf $SRCDIR/eco32 $SRCDIR/gimmix

rm -f "escape-src-$VERSION.tar.bz2"
tar -jcvf "escape-src-$VERSION.tar.bz2" $SRCDIR > /dev/null
rm -Rf $SRCDIR
cd $OLDDIR
echo "done"

# build directory to compress
mkdir $DEST/$SRCDIR
cp $LICENSE $DEST/$SRCDIR

for i in eco32 mmix i586; do
	echo -n "Switching to $i..."
	if [ "$i" = "i586" ]; then
		./switch.sh $i release --dynamic > /dev/null 2>&1
	else
		./switch.sh $i release > /dev/null 2>&1
	fi
	echo "done"
	
	echo -n "Building disk, cd and vmdk-disk..."
	make createhdd > /dev/null
	cp build/$i-release/hd.img "$DEST/$SRCDIR/escape-$i-hd.img"
	if [ "$i" = "i586" ]; then
		make createcd > /dev/null 2>&1
		cp build/$i-release/cd.iso "$DEST/$SRCDIR/escape-$i-cd.iso"
		qemu-img convert -f raw build/$i-release/hd.img -O vmdk \
			"$DEST/$SRCDIR/escape-$i-hd.vmdk" >/dev/null 2>&1
	fi
	echo "done"

	OLDDIR=`pwd`
	cd $DEST
	
	echo -n "Zipping disk and cd..."
	rm -f "escape-$i-hd-$VERSION.tar.bz2"
	tar -jcvf "escape-$i-hd-$VERSION.tar.bz2" $SRCDIR/{escape-$i-hd.img,LICENSE} > /dev/null
	rm -f "$SRCDIR/escape-$i-hd.img"
	if [ "$i" = "i586" ]; then
		rm -f "escape-$i-cd-$VERSION.tar.bz2"
		tar -jcvf "escape-$i-cd-$VERSION.tar.bz2" $SRCDIR/{escape-$i-cd.iso,LICENSE} > /dev/null
		rm -f "$SRCDIR/escape-$i-cd.iso"
		
		rm -f "escape-$i-vmdk-$VERSION.tar.bz2"
		tar -jcvf "escape-$i-vmdk-$VERSION.tar.bz2" $SRCDIR/{escape-$i-hd.vmdk,LICENSE} > /dev/null
		rm -f "$SRCDIR/escape-$i-hd.vmdk"
	fi
	echo "done"
	
	cd $OLDDIR
done

rm -Rf $DEST/$SRCDIR

