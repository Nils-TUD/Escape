#!/usr/bin/ruby -w
if ARGV.length < 1
	$stderr.puts "Usage: genmortest.rb mms|test"
	exit
end

regs = [
	[0x1234567890ABCDEF,2],
	[0x0102040810204080,3],
	[0xFF00FF00FF00FF00,4],
	[0x00FF00FF00FF00FF,5],
	[0xF0F0F0F0F0F0F0F0,6]
]
instrs = [
	"MOR",
	"MXOR"
]

def createMat()
	return [
		[0,0,0,0,0,0,0,0],
		[0,0,0,0,0,0,0,0],
		[0,0,0,0,0,0,0,0],
		[0,0,0,0,0,0,0,0],
		[0,0,0,0,0,0,0,0],
		[0,0,0,0,0,0,0,0],
		[0,0,0,0,0,0,0,0],
		[0,0,0,0,0,0,0,0],
	]
end

def boolmult(x,y,xor)
	z = createMat
	for i in 0...8
		for j in 0...8
			val = 0
			for k in 0...8
				if xor
					val ^= x[i][k] & y[k][j]
				else
					val |= x[i][k] & y[k][j]
				end
			end
			z[i][j] = val
		end
	end
	return z
end

def int2mat(x)
	res = createMat
	for i in 0...8
		for j in 0...8
			byte = x >> (j * 8)
			res[i][j] = (byte & (1 << i)) != 0 ? 1 : 0
		end
	end
	return res
end

def mat2int(m)
	res = 0
	for i in 0...8
		for j in 0...8
			if m[i][j] != 0
				res |= 1 << (i + j * 8)
			end
		end
	end
	return res
end

def printmat(m)
	for i in 0...8
		for j in 0...8
			printf("%d ",m[i][j])
		end
		puts
	end
end

if ARGV[0] == "mms"
	puts <<HEADER
%
% mor.mms -- tests mor and mxor-instructions (generated)
%

		LOC		#1000

Main	SETH	$2,#1234
		ORMH	$2,#5678
		ORML	$2,#90AB
		ORL		$2,#CDEF
		SETH	$3,#0102
		ORMH	$3,#0408
		ORML	$3,#1020
		ORL		$3,#4080
		SETH	$4,#FF00
		ORMH	$4,#FF00
		ORML	$4,#FF00
		ORL		$4,#FF00
		SETH	$5,#00FF
		ORMH	$5,#00FF
		ORML	$5,#00FF
		ORL		$5,#00FF
		SETH	$6,#F0F0
		ORMH	$6,#F0F0
		ORML	$6,#F0F0
		ORL		$6,#F0F0

HEADER

	reg = 10
	instrs.each { |name|
		puts "		% test #{name}(I)"
		for j in 0...regs.length
			for i in 0...regs.length
				printf("		%s	$%d,$%d,$%d		%% #%016X * #%016X\n",
					name,reg,regs[j][1],regs[i][1],regs[j][0],regs[i][0])
				reg += 1
			end
			puts
		end
		puts
	}
	puts "		TRAP	0"
else
	printf("r:$10..$%d",10 + instrs.length * regs.length ** 2 - 1)
	reg = 10
	instrs.each { |name|
		for j in 0...regs.length
			for i in 0...regs.length
				m1 = int2mat(regs[j][0])
				m2 = int2mat(regs[i][0])
				res = mat2int(boolmult(m1,m2,name == "MXOR"))
				printf("\n$%d: %016X",reg,res)
				reg += 1
			end
		end
	}
end

