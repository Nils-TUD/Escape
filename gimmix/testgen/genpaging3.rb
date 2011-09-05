#!/usr/bin/ruby -w
if ARGV.length < 1
	$stderr.puts "Usage: genpg3test.rb mms|test"
	exit
end

$LOAD_PATH << File.dirname(__FILE__)
require "pagingstructuregenerator.rb"

gen = PagingStructureGenerator.new("paging3",[0,0,0,0],[4,581],13,0x2000,5)

if ARGV[0] == "mms"
	puts gen.getHeader
	puts gen.getTests
	puts gen.getFooter
else
	print gen.getResults
end

