#!/usr/bin/ruby -w
if ARGV.length < 1
	$stderr.puts "Usage: genshifttest.rb mms|test"
	exit
end

instrs = [
	["SL",true,"s<<"],
	["SLU",false,"u<<"],
	["SR",false,"s>>"],
	["SRU",false,"u>>"],
]
shifts = [
	0,1,4,63,64,65,128
]

def gentests(reg,name,desc,valreg,i)
	puts "		% $#{valreg} #{desc} #{i}"
	puts "		#{name}		$#{reg},$#{valreg},#{i}"
	reg += 1
	puts "		SET		$#{reg},#{i}"
	puts "		#{name}		$#{reg},$#{valreg},$#{reg}"
	reg += 1
	puts "		GET		$#{reg},rA"
	puts "		PUT		rA,0"
	reg += 1
	puts
	return reg
end

def sar(x,i)
	res = x >> i
	if (x & (1 << 63)) != 0
		res |= ((1 << i) - 1) << (63 - i + 1)
	end
	return res
end

def genres(reg,name,overflow,val,i)
	res = case name
		when "SL","SLU" then
			i > 63 ? 0 : val << i
		when "SR" then
			i > 63 ? ((val & (1 << 63)) != 0 ? 0xFFFFFFFFFFFFFFFF : 0) : sar(val,i)
		when "SRU"
			i > 63 ? 0 : val >> i
	end
	res &= 0xFFFFFFFFFFFFFFFF
	printf("\n$#{reg}: %016X",res)
	reg += 1
	printf("\n$#{reg}: %016X",res)
	reg += 1
	printf("\n$#{reg}: ")
	reg += 1
	if overflow
		overflow = sar(res,i) != val
	end
	if overflow
		print "0000000000000040"
	else
		print "0000000000000000"
	end
	return reg
end

if ARGV[0] == "mms"
	puts <<HEADER
%
% shift.mms -- tests shift-instructions (generated)
%
		LOC		#1000

Main	NOR		$2,$2,$2	% = -1
		SETH	$3,#1234
		ORMH	$3,#5678
		ORML	$3,#90AB
		ORL		$3,#CDEF
	
HEADER
	
	reg = 4
	instrs.each { |instr|
		name,overflow,desc = instr[0],instr[1],instr[2]
		puts "		% == test #{name}(i) =="
		shifts.each { |sh|
			reg = gentests(reg,name,desc,2,sh)
			reg = gentests(reg,name,desc,3,sh)
		}
	}
	
	puts "		TRAP	0"
else
	puts "r:$2..$" + (3 + instrs.length * shifts.length * 2 * 3).to_s
	puts "$2: FFFFFFFFFFFFFFFF"
	print "$3: 1234567890ABCDEF"
	reg = 4
	instrs.each { |instr|
		name,overflow,desc = instr[0],instr[1],instr[2]
		shifts.each { |sh|
			reg = genres(reg,name,overflow,0xFFFFFFFFFFFFFFFF,sh)
			reg = genres(reg,name,overflow,0x1234567890ABCDEF,sh)
		}
	}
end

