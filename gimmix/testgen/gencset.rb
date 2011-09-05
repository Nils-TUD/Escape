#!/usr/bin/ruby -w
if ARGV.length < 1
	$stderr.puts "Usage: gencstest.rb mms|test"
	exit
end

instrs = [
	["CSN",lambda { |x| (x & (1 << 63)) != 0 }],
	["CSZ",lambda { |x| x == 0 }],
	["CSP",lambda { |x| (x & (1 << 63)) == 0 && x != 0 }],
	["CSOD",lambda { |x| (x & 1) != 0 }],
	["CSNN",lambda { |x| (x & (1 << 63)) == 0 }],
	["CSNZ",lambda { |x| x != 0 }],
	["CSNP",lambda { |x| (x & (1 << 63)) != 0 || x == 0 }],
	["CSEV",lambda { |x| (x & 1) == 0 }],
]
values = [
	["$2",0xFFFFFFFFFFFFFFFF],
	["$3",0],
	["$4",1],
	["$5",2],
	["$6",0x8000000000000000],
]

if ARGV[0] == "mms"
	puts <<HEADER
%
% cset.mms -- tests conditional-set-instructions (generated)
%
		LOC		#1000

Main	NOR		$2,$2,$2	% = -1
		SET		$3,0
		SET		$4,1
		SET		$5,2
		SETH	$6,#8000

HEADER

	reg = 7;
	instrs.each { |instr|
		for i in 0...values.length
			printf("		%% test %s, #%X\n",instr[0],values[i][1])
			puts "		SET		$#{reg},1"
			puts "		#{instr[0]}		$#{reg},#{values[i][0]},$5"
			puts
			reg += 1
		end
		
		for i in 0...values.length
			printf("		%% test %s, #%X\n",instr[0],values[i][1])
			puts "		SET		$#{reg},1"
			puts "		#{instr[0]}		$#{reg},#{values[i][0]},2"
			puts
			reg += 1
		end
	}
	
	puts "		TRAP	0"
else
	print "r:$7..$" + (6 + instrs.length * values.length * 2).to_s
	reg = 7
	instrs.each { |instr|
		for j in 0..1
			for i in 0...values.length
				if instr[1].call(values[i][1])
					print "\n$#{reg}: 0000000000000002"
				else
					print "\n$#{reg}: 0000000000000001"
				end
				reg += 1
			end
		end
	}
end
