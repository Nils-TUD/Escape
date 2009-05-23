#!/bin/bash

i=0
sudo umount /dev/loop0 > /dev/null 2>&1
while [ $? != 0 ]; \
do \
	i=`expr $i + 1`
	if [ $i -ge 10 ]; then
		echo "Unmount failed after $i tries"
		exit 1
	fi
	sudo umount /dev/loop0 > /dev/null 2>&1
done