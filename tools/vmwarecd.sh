#!/bin/bash
if [ $# -ne 2 ]; then
	echo "Usage: $0 <cfg> <hdd>" 1>&2
	exit 1
fi

CFG=$1
HDD=$2
sed --in-place -e \
	's/ide1:0.fileName = ".*\?"/ide1:0.fileName = "'`echo $HDD | sed -e 's/\//\\\\\//g'`'"/' $CFG

