#!/usr/bin/ruby -w
if ARGV.length < 1
	$stderr.puts "Usage: genmul.rb mms|test [--unsigned]"
	exit
end

vals = [
	0,1,2,-1,-5,5,10,72,-831,1024,
	0x7FFFFFFFFFFFFFFF,
	0x8000000000000000
]
instr = ARGV[1] == "--unsigned" ? "MULU" : "MUL"

if ARGV[0] == "mms"
	puts "%"
	puts "% #{instr}.mms -- tests #{instr}-instruction (generated)"
	puts "%"
	puts ""
	puts "PAT		OCTA	#0000000000000000"
	
	for i in 1...vals.length
		printf("		OCTA	#%016X\n",vals[i])
	end
	
	puts <<HEADER

		LOC		#1000

Main	SET		$2,PAT
		SET		$3,#8000
HEADER

	reg = 4
	for i in 0...vals.length
		printf("		LDOU	$%d,$2,#%02X\n",reg,i * 8)
		reg += 1
	end
	puts
	i = 0
	loc = 0x8000;
	vals.each { |x|
		j = 0
		vals.each { |y|
			printf("		%s	$%d,$%d,$%d		%% %016X * %016X\n",instr,reg,4 + i,4 + j,x,y)
			printf("		STOU	$%d,$3,0	%% #%X\n",reg,loc);
			printf("		GET	$%d,rH\n",reg);
			printf("		STOU	$%d,$3,8	%% #%X\n",reg,loc + 8);
			printf("		GET $%d,rA\n",reg);
			printf("		STOU	$%d,$3,16	%% #%X\n",reg,loc + 16);
			printf("		ADDU	$3,$3,24\n");
			puts
			loc += 24
			j += 1
		}
		i += 1
	}
	
	printf("		%% Sync memory\n");
	size = vals.length * vals.length * 24;
	printf("		SETL	$3,#8000\n");
	while size > 0
		amount = 0xFE < size ? 0xFE : size
		printf("		SYNCD	#%X,$3\n",amount);
		printf("		ADDU	$3,$3,#%X\n",amount + 1);
		size -= amount;
	end
	
	puts "		TRAP	0"
else
	printf("m:8000..%X",0x8000 + vals.length * vals.length * 24 - 8)
	# no expected results here because its too complicated to calculate and isn't worth it
end
