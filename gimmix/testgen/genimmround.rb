#!/usr/bin/ruby -w
if ARGV.length < 1
	$stderr.puts "Usage: genimmround.rb mms|test"
	exit
end

vals = [
	0x3FFFFFFFFFFFFFFB,
	0x3FF6666666666666,
	0xC002666666666666,
	0x41678BC640000000,
]
roundings = [
	"ROUND_OFF",
	"ROUND_UP",
	"ROUND_DOWN",
	"ROUND_NEAR"
]
instrs = [
	"FINT",
	"FSQRT",
	"FLOT",
	"FLOTU",
	"SFLOT",
	"SFLOTU",
	"FIX",
	"FIXU",
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
% immrounding.mms -- tests rounding as immediate-value
%

		LOC		#0000
HEADER

	vals.each { |val|
		printf("		OCTA	#%016X\n",val)
	}

	puts <<HEADER
	
		LOC		#1000

Main	SET		$0,#0000

HEADER

	reg = 2
	instrs.each { |instr|
		off = 0
		printf("		%% test %s\n",instr)
		vals.each { |val|
			printf("		LDOU	$1,$0,#%x			%% #%016X\n",off,val)
			roundings.each { |r|
				printf("		%s	$%d,%s,$1\n",instr,reg,r)
				reg += 1
			}
			puts
			off += 8
		}
		puts
	}
	puts "		TRAP	0"
else
	printf("r:$2..$%d",2 + instrs.length * vals.length * roundings.length - 1);
end

