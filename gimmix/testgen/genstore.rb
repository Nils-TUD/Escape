#!/usr/bin/ruby -w
if ARGV.length < 1
	$stderr.puts "Usage: genstoretest.rb mms|test"
	exit
end

instrs = [
	# [<name>,<width>,<overflow>]
	['STB',8,true],
	['STBU',8,false],
	['STW',16,true],
	['STWU',16,false],
	['STT',32,true],
	['STTU',32,false],
	['STO',64,false],
	['STOU',64,false],
	['STHT',64,false],
	['STCO',8,false],
]
vals = [
	0,1,10,
	0xFF,0xFFFF,0xFFFFFFFF,-1,
	0x7F,0x7FFF,0x7FFFFFFF,0x7FFFFFFFFFFFFFFF,
	0x80,0x8000,0x80000000,-9223372036854775808,
	127,128,32767,32768,2147483647,2147483648,
	-128,-129,-32768,-32769,-2147483648,-2147483649,
]

if ARGV[0] == "mms"
	puts <<HEADER
%
% store.mms -- tests store-instructions (generated)
%

		LOC		#1000

Main	SET		$0,#F000
		
HEADER

	reg = 2
	vals.each { |val|
		printf("		SETH	$%d,#%04X\n",reg,(val >> 48) & 0xFFFF)
		printf("		ORMH	$%d,#%04X\n",reg,(val >> 32) & 0xFFFF)
		printf("		ORML	$%d,#%04X\n",reg,(val >> 16) & 0xFFFF)
		printf("		ORL		$%d,#%04X\n",reg,(val >>  0) & 0xFFFF)
		reg += 1
	}
	puts
	
	addr = 0xF000
	instrs.each { |instr|
		name,width = instr[0],instr[1]
		puts "		% test #{name}(I)"
		for i in 0..vals.length - 1
			if name == "STCO"
				printf("		%s		#%02X,$0,0		%% -> #%X\n",name,vals[i] & 0xFF,addr)
			else
				printf("		%s		$%d,$0,0		%% %d -> #%X\n",name,i + 2,vals[i],addr)
			end
			puts "		GET		$1,rA"
			puts "		PUT		rA,0"
			puts "		STOU	$1,$0,8"
			puts "		ADDU	$0,$0,16"
			puts
			addr += 16
		end
		puts
	}
	
	# note that this is necessary in MMMIX, because otherwise the data would be in the data-cache
	# but not in memory
	printf("		%% Sync memory\n")
	size = instrs.length * vals.length * 2 * 8
	printf("		SETL	$1,#F000\n")
	while(size > 0)
		amount = size > 0xFE ? 0xFE : size
		printf("		SYNCD	#%X,$1\n",amount)
		printf("		ADDU	$1,$1,#%X\n",amount + 1)
		size -= amount;
	end
	puts "		TRAP	0"
else
	printf("m:F000..%X",0xF000 + (instrs.length * vals.length * 2 - 1) * 8)
	addr = 0xF000
	instrs.each { |instr|
		name,width,overflow = instr[0],instr[1],instr[2]
		for i in 0..vals.length - 1
			if name == "STHT"
				printf("\nm[%016X]: %016X",addr,vals[i] & 0xFFFFFFFF00000000)
			elsif name == "STCO"
				printf("\nm[%016X]: %016X",addr,vals[i] & 0xFF)
			elsif width == 64
				printf("\nm[%016X]: %016X",addr,vals[i] & 0xFFFFFFFFFFFFFFFF)
			else
				printf("\nm[%016X]: %016X",addr,(vals[i] & ((1 << width) - 1)) << (64 - width))
			end
			addr += 8
			
			rA = 0
			if overflow
				if vals[i] >= 2 ** (width - 1) || vals[i] < -(2 ** (width - 1))
					rA = 0x40
				end
			end
			printf("\nm[%016X]: %016X",addr,rA)
			addr += 8
		end
	}
end
