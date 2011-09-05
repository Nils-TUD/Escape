#!/usr/bin/ruby -w
if ARGV.length < 1
	$stderr.puts "Usage: gensaddtest.rb mms|test"
	exit
end

vals = [
	0x0000000000000000,
	0xFFFFFFFFFFFFFFFF,
	0x00FF00FF00FF00FF,
	0x0F0F0F0F0F0F0F0F,
	0x0001000100010001,
	0x8000800080008000,
	0xFFFFFFFF00000000,
	0x00000000FFFFFFFF
]

def countBits(x)
	c = 0
	for i in 0...64
		if (x & (1 << i)) != 0
			c += 1
		end
	end
	return c
end

if ARGV[0] == "mms"
	puts <<HEADER
%
% sadd.mms -- tests sadd-instruction (generated)
%

PAT		OCTA	#0000000000000000
HEADER
	
	for i in 1...vals.length
		printf("		OCTA	#%016X\n",vals[i])
	end
	
	puts <<HEADER

		LOC		#1000

Main	SET		$2,PAT
HEADER

	reg = 3
	for i in 0...vals.length
		printf("		LDOU	$%d,$2,#%02X\n",reg,i * 8)
		reg += 1
	end
	puts
	j = 0
	vals.each { |val|
		printf("		SADD	$%d,$%d,00		%% count(%016X & ~%016X)\n",reg,3 + j,val,0)
		reg += 1
		printf("		SADD	$%d,$%d,$3		%% count(%016X & ~%016X)\n",reg,3 + j,val,vals[0])
		reg += 1
		printf("		SADD	$%d,$%d,$5		%% count(%016X & ~%016X)\n",reg,3 + j,val,vals[2])
		reg += 1
		j += 1
		puts
	}
	puts "		TRAP	0"
else
	print "r:$3..$" + (3 + vals.length + vals.length * 3 - 1).to_s
	reg = 3
	for i in 0...vals.length
		printf("\n$%d: %016X",reg,vals[i])
		reg += 1
	end
	vals.each { |val|
		[0,vals[0],vals[2]].each { |add|
			printf("\n$%d: %016X",reg,countBits(val & ~add))
			reg += 1
		}
	}
end
