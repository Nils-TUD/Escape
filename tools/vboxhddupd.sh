#!/bin/bash

# check arguments
if [ $# -ne 2 ]; then
	echo "Usage: $0 <vmName> <pathToVDI>" 1>&2
	exit 1
fi

# check if the UUID has changed
if [ "`VBoxManage showhdinfo \"$2\" | grep -i 'Error'`" != "" ]; then
	# first detach disk from vm
	VBoxManage modifyvm "$1" --hda none
	# remove disk
	VBoxManage closemedium disk "$2"
	# add disk again
	VBoxManage openmedium disk "$2"
	# attach to vm
	VBoxManage modifyvm "$1" --hda "$2"
fi;

