#!/bin/sh

if [ $# -ne 1 ]; then
	echo "Usage $0 <name>" >&2
	exit 1
fi

name=$1

max=0
for i in dist/etc/groups/*/id; do
	read id < $i
	if [ $id -gt $max ]; then
		max=$id
	fi
done

id=$((max + 1))

mkdir dist/etc/groups/$name
echo $id > dist/etc/groups/$name/id
