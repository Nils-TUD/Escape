#!/bin/sh

if [ $# -eq 0 ]; then
	echo "Missing parameter"
	exit -1
fi

objdump -t $* | gawk '
/^[a-f0-9]+.+?\.text.*$/ {
	address = gensub(/^([a-f0-9]+).*/, "\\1", 1)
	name = gensub(/^[a-f0-9]+.+?\.text[ \t]+[a-f0-9]+[ \t]+(.+)$/, "\\1", 1)
	print address "\t" name
}
' | sort | gawk '{ print "\t{0x" $1 ", \"" $2 "\"}," }'
