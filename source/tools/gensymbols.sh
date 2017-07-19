#!/bin/sh

echo "#include <ksymbols.h>"
echo "#include <video.h>"
echo ""
echo "KSymbols::Symbol kernel_symbols[] = {"

if [ "$1" != "-" ]; then
	objdump -tC $* | gawk '
	/^[a-f0-9]+.+?\.text.*$/ {
		address = gensub(/^([a-f0-9]+).*/, "\\1", 1)
		name = gensub(/^[a-f0-9]+.+?\.text[ \t]+[a-f0-9]+[ \t]+(.+)$/, "\\1", 1)
		print address "\t" name
	}
	' | sort | gawk '
	function ltrim(s) {
		sub(/^[ \t\r\n]+/, "", s);
		return s
	}
	{
		printf("\t{0x%s, ", $1)
		$1 = ""
		print "\"" ltrim($0) "\"},"
	}'
fi

echo "};"
echo "size_t kernel_symbol_count = ARRAY_SIZE(kernel_symbols);"
echo ""

