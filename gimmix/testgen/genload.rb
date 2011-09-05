#!/usr/bin/ruby -w
if ARGV.length < 1
	$stderr.puts "Usage: genloadtest.rb mms|test"
	exit
end

instrs = [
	# [<name>,<reg>,<width>]
	["LDB","$2",8],
	["LDBU","$2",8],
	["LDW","$3",16],
	["LDWU","$3",16],
	["LDT","$4",32],
	["LDTU","$4",32],
	["LDO","$5",64],
	["LDOU","$5",64],
	["LDHT","$4",32]
]

if ARGV[0] == "mms"
	puts <<HEADER
%
% load.mms -- tests load-instructions (generated)
%

TEST1	BYTE	#FF,#FE,1,2
		LOC		@ + (2 - @) & 1		% align to 2
TEST2	WYDE	#FFFF,#FFFE,1,2
		LOC		@ + (4 - @) & 3		% align to 4
TEST3	TETRA	#FFFFFFFF,#FFFFFFFE,1,2
		LOC		@ + (8 - @) & 7		% align to 8
TEST4	OCTA	#FFFFFFFFFFFFFFFF,#FFFFFFFFFFFFFFFE,1,2

		LOC		#1000

Main	SET		$2,TEST1
		SET		$3,TEST2
		SET		$4,TEST3
		SET		$5,TEST4

HEADER

	reg = 6
	instrs.each { |instr|
		name,src,width = instr[0],instr[1],instr[2]
		puts "		% test #{name}(I)"
		for i in 0..3
			puts "		#{name}		$#{reg},#{src}," + (i * width / 8).to_s
			reg += 1
			puts "		SET		$#{reg}," + (i * width / 8).to_s
			puts "		#{name}		$#{reg},#{src},$#{reg}"
			reg += 1
		end
		puts
	}
	puts "		TRAP	0"
else
	print "r:$6..$" + (5 + instrs.length * 4 * 2).to_s
	reg = 6
	vals = [0xFFFFFFFFFFFFFFFF,0xFFFFFFFFFFFFFFFE,0x0000000000000001,0x0000000000000002]
	instrs.each { |instr|
		name,width = instr[0],instr[2]
		for i in 0..3
			if name == "LDHT"
				val = vals[i] << 32 & vals[0]
			elsif name.include?("U")
				val = vals[i] & ((1 << width) - 1)
			else
				val = vals[i]
			end
			printf("\n$%d: %016X",reg,val)
			reg += 1
			printf("\n$%d: %016X",reg,val)
			reg += 1
		end
	}
end

