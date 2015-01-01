#!/bin/sh
dest="$1"
version="$2"

if [ $# -lt 2 ]; then
	echo "Usage: $0 <targetDir> <version>" 1>&2
	exit 1
fi

if [ ! -d "$dest" ]; then
	echo "'$dest' is no directory" 1>&2
	exit 1
fi

export ESC_BUILD=release ESC_LINKTYPE=dynamic

create_cd_archive() {
	cp build/$2-$ESC_BUILD/cd-mini.iso ../LICENSE $1
	mv $1/cd-mini.iso $1/escape-$3-$2-cd.iso
	( cd $1 && tar -jcvf escape-$3-$2-cd.tar.bz2 escape-$3-$2-cd.iso LICENSE > /dev/null )
	rm -f $1/escape-$3-$2-cd.iso $1/LICENSE
}
create_hd_archive() {
	cp build/$2-$ESC_BUILD/hd.img ../LICENSE $1
	mv $1/hd.img $1/escape-$3-$2-hd.img
	( cd $1 && tar -jcvf escape-$3-$2-hd.tar.bz2 escape-$3-$2-hd.img LICENSE > /dev/null )
	rm -f $1/escape-$3-$2-hd.img $1/LICENSE
}

ESC_TARGET=i586 ./b run create-mini-iso
create_cd_archive $dest i586 $version
ESC_TARGET=i586 ./b run create-img -n
create_hd_archive $dest i586 $version

ESC_TARGET=x86_64 ./b run create-mini-iso
create_cd_archive $dest x86_64 $version
ESC_TARGET=x86_64 ./b run create-img -n
create_hd_archive $dest x86_64 $version

ESC_TARGET=eco32 ./b run create-img
create_hd_archive $dest eco32 $version

ESC_TARGET=mmix ./b run create-img
create_hd_archive $dest mmix $version
