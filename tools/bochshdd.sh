#!/bin/bash
if [ $# -ne 2 ]; then
	echo "Usage: $0 <cfg> <hdd>" 1>&2
	exit 1
fi

CFG=$1
HDD=$2
sed --in-place -e \
	's/\(ata0-master:.*\?path\)=.*\?,/\1='`echo $HDD | sed -e 's/\//\\\\\//g'`',/' $CFG

