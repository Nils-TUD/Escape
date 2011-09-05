#!/usr/bin/ruby -w
if ARGV.length < 1
	$stderr.puts "Usage: genpg2test.rb mms|test"
	exit
end

$LOAD_PATH << File.dirname(__FILE__)
require "pagingstructuregenerator.rb"

gen = PagingStructureGenerator.new("paging2",[1,3,4,5],[4,1023],18,0x2000,0x200)

if ARGV[0] == "mms"
	puts gen.getHeader
	puts gen.getTests
	puts gen.getFooter
else
	print gen.getResults
end

