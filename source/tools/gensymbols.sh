#!/bin/sh

echo "#include <sys/ksymbols.h>"
echo "#include <sys/video.h>"
echo ""
echo "sSymbol kernel_symbols[] = {"

if [ "$1" != "-" ]; then
	objdump -t $* | gawk '
	/^[a-f0-9]+.+?\.text.*$/ {
		address = gensub(/^([a-f0-9]+).*/, "\\1", 1)
		name = gensub(/^[a-f0-9]+.+?\.text[ \t]+[a-f0-9]+[ \t]+(.+)$/, "\\1", 1)
		print address "\t" name
	}
	' | sort | gawk '{ print "\t{0x" $1 ", \"" $2 "\"}," }'
fi

echo "};"
echo "size_t kernel_symbol_count = ARRAY_SIZE(kernel_symbols);"
echo ""

