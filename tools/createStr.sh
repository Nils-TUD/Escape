#!/bin/sh

if [ "$#" -lt "2" ]; then
	echo "Please provide the number of strings and the string itself"
	exit 1
fi

STR=$1
COUNT=$2

i=0
while test $i != $COUNT ; do
	printf "$STR" $i;
	i=`expr $i + 1`;
done;