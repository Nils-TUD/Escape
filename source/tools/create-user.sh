#!/bin/sh

if [ $# -lt 1 ]; then
	echo "Usage $0 <name> [<group>...]" >&2
	exit 1
fi

name=$1

if [ "$2" != "$name" ]; then
	if [ ! -d dist/etc/groups/$name ]; then
		./tools/create-group.sh $name
	fi
else
	shift
fi

max=0
for i in dist/etc/users/*/id; do
	read id < $i
	if [ $id -gt $max ]; then
		max=$id
	fi
done

id=$((max + 1))

mkdir dist/etc/users/$name
echo $id > dist/etc/users/$name/id
cp dist/etc/groups/$1/id dist/etc/users/$name/gid

echo -n > dist/etc/users/$name/groups
for g in $@; do
	echo -n "`cat dist/etc/groups/$g/id`," >> dist/etc/users/$name/groups
done
sed --in-place 's/.$/\n/' dist/etc/users/$name/groups
