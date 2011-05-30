#!/bin/bash

if [ $# -ne 2 ]; then
	echo "Usage: $0 <cdrom> <vboxMachineName>" 1>&2
	exit 1
fi

CDROM=`readlink -f $1`
MACHINE=$2

if [ ! -f $CDROM ]; then
	echo "No such file: $CDROM" 1>&2
	exit 1
fi

MACHINECFG="$HOME/.VirtualBox/Machines/$MACHINE/$MACHINE.xml"
DVDSTART=`cat "$MACHINECFG" | grep -n 'AttachedDevice.*\?type="DVD"' | cut -f 1 -d ':'`
ENDS=`cat "$MACHINECFG" | grep -n '/AttachedDevice' | cut -f 1 -d ':'`

for i in $ENDS; do
	if [ $i -ge $DVDSTART ]; then
		DVDEND=$i
		break
	fi
done
if [ "$DVDEND" = "" ]; then
	echo "End-Tag of 'AttachedDevice' not found" 1>&2
	exit 1
fi

UUID=`cat $HOME/.VirtualBox/VirtualBox.xml | grep $CDROM | sed -e 's/.*uuid="{\(.*\)}".*/\1/'`
sed --in-place -e $DVDSTART','$DVDEND's/uuid="{.*\?}"/uuid="{'$UUID'}"/' "$MACHINECFG"

