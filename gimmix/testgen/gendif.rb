#!/usr/bin/ruby -w
if ARGV.length < 1
	$stderr.puts "Usage: gendiftest.rb mms|test"
	exit
end

regs = [
	# [<value>,<reg>]
	[0x1234567890ABCDEF,2],
	[0xCDEF90AB56781234,3],
	[0xFFFFFFFFFFFFFFFF,4],
	[0x00FF00FF00FF00FF,5],
]
instrs = [
	# [<name>,<width>]
	["BDIF",8],
	["WDIF",16],
	["TDIF",32],
	["ODIF",64],
]

def diff(x,y,w)
	res = 0
	mask = (1 << w) - 1
	(0..64).step(w) do |i|
		xi = (x >> i) & mask
		yi = (y >> i) & mask
		res |= (yi > xi ? 0 : xi - yi) << i
	end
	return res
end

if ARGV[0] == "mms"
	puts <<HEADER
%
% dif.mms -- tests dif-instructions (generated)
%

		LOC		#1000

Main	SETH	$2,#1234
		ORMH	$2,#5678
		ORML	$2,#90AB
		ORL		$2,#CDEF
		SETH	$3,#CDEF
		ORMH	$3,#90AB
		ORML	$3,#5678
		ORL		$3,#1234
		SETH	$4,#FFFF
		ORMH	$4,#FFFF
		ORML	$4,#FFFF
		ORL		$4,#FFFF
		SETH	$5,#00FF
		ORMH	$5,#00FF
		ORML	$5,#00FF
		ORL		$5,#00FF

HEADER
	
	reg = 10
	instrs.each { |instr|
		name,width = instr[0],instr[1]
		puts "		% test #{name}(I)"
		for j in 0...regs.length
			for i in 0...regs.length
				printf("		%s	$%d,$%d,$%d		%% #%016X - #%016X\n",
					name,reg,regs[j][1],regs[i][1],regs[j][0],regs[i][0])
				reg += 1
			end
			puts
		end
		puts
	}
	puts "		TRAP	0"
else
	printf("r:$10..$%d",10 + instrs.length * regs.length ** 2 - 1);
	reg = 10
	instrs.each { |instr|
		name,width = instr[0],instr[1]
		for j in 0...regs.length
			for i in 0...regs.length
				res = diff(regs[j][0],regs[i][0],width)
				printf("\n$%d: %016X",reg,res)
				reg += 1
			end
		end
	}
end

