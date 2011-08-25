#!/bin/bash
if [ $# -ne 4 ]; then
	echo "Usage: $0 <cfg> <hdd> <cd> <map>" 1>&2
	exit 1
fi

# hdd
CFG=$1
HDD=$2
CD=$3
MAP=$4
sed --in-place -e \
	's/\(ata0-master:.*\?path\)=.*\?,/\1='`echo $HDD | sed -e 's/\//\\\\\//g'`',/' $CFG

# cdrom
sed --in-place -e \
	's/\(ata1-master:.*\?path\)=.*\?,/\1='`echo $CD | sed -e 's/\//\\\\\//g'`',/' $CFG

sed --in-place -e \
	's/^\(debug_symbols:.*\?file\)=.*\?/\1='`echo $MAP | sed -e 's/\//\\\\\//g'`'/' $CFG

