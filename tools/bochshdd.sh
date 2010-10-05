#!/bin/bash
if [ $# -ne 3 ]; then
	echo "Usage: $0 <cfg> <hdd> <cd>" 1>&2
	exit 1
fi

# hdd
CFG=$1
HDD=$2
CD=$3
sed --in-place -e \
	's/\(ata0-master:.*\?path\)=.*\?,/\1='`echo $HDD | sed -e 's/\//\\\\\//g'`',/' $CFG

# cdrom
sed --in-place -e \
	's/\(ata1-master:.*\?path\)=.*\?,/\1='`echo $CD | sed -e 's/\//\\\\\//g'`',/' $CFG

