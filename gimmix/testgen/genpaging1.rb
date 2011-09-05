#!/usr/bin/ruby -w
if ARGV.length < 1
	$stderr.puts "Usage: genpg1test.rb mms|test"
	exit
end

$LOAD_PATH << File.dirname(__FILE__)
require "pagingstructuregenerator.rb"

gen = PagingStructureGenerator.new("paging1",[1,3,4,9],[1,177],13,0x2000,4)

if ARGV[0] == "mms"
	puts gen.getHeader
	puts gen.getTests
	puts gen.getFooter
else
	print gen.getResults
end

