#!/bin/bash

if [ $# -lt 1 ]; then
	echo "Usage: $0 <dir>" 1>&2
	exit 1
fi

listsvn() {
	for i in `ls -a -1 "$1"`; do
		if [ "$i" != "." -a "$i" != ".." ]; then
			if [ -d "$1/$i" ]; then
				if [ $i = ".svn" ]; then
					echo $1/$i
				else
					listsvn "$1/$i";
				fi
			fi
		fi
	done
}

listsvn "$1"

