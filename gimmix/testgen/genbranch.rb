#!/usr/bin/ruby -w
if ARGV.length < 1
	$stderr.puts "Usage: genbrtest.rb mms|test [--probable]"
	exit
end

instrs = [
	["BN",lambda { |x| (x & (1 << 63)) != 0 }],
	["BZ",lambda { |x| x == 0 }],
	["BP",lambda { |x| (x & (1 << 63)) == 0 && x != 0 }],
	["BOD",lambda { |x| (x & 1) != 0 }],
	["BNN",lambda { |x| (x & (1 << 63)) == 0 }],
	["BNZ",lambda { |x| x != 0 }],
	["BNP",lambda { |x| (x & (1 << 63)) != 0 || x == 0 }],
	["BEV",lambda { |x| (x & 1) == 0 }],
]
values = [
	["$2",0xFFFFFFFFFFFFFFFF],
	["$3",0],
	["$4",1],
	["$5",2],
	["$6",0x8000000000000000],
]

if ARGV.length > 1 && ARGV[1] == "--probable"
	instrs.each { |instr|
		instr[0] = "P" + instr[0]
	}
end

if ARGV[0] == "mms"
	puts <<HEADER
%
% (p)branch.mms -- tests branch-instructions (generated)
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
			puts "		#{instr[0]}		#{values[i][0]},1F"
			puts "		SET		$#{reg},1"
			puts "		JMP		2F"
			puts "1H		SET		$#{reg},2"
			puts "2H		OR		$#{reg},$#{reg},$#{reg}"
			puts
			reg += 1
		end
		
		for i in 0...values.length
			printf("		%% test %sB, #%X\n",instr[0],values[i][1])
			puts "		JMP		2F"
			puts "1H		SET		$#{reg},2"
			puts "		JMP		3F"
			puts "2H		#{instr[0]}		#{values[i][0]},1B"
			puts "		SET		$#{reg},1"
			puts "3H		OR		$#{reg},$#{reg},$#{reg}"
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
