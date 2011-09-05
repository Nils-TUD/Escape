#!/usr/bin/ruby -w
puts <<HEADER
%
% resumecont.mms -- test resume
%

% --- IMPORTANT: atm we don't compare the DS-instructions executed by resume-continue!
%                because mmmix has a different behaviour but the difference is ok in this case

		LOC		#4000
DATA	OCTA	#5
		OCTA	#6
		
		% forced trap addresses
		LOC		#500000
		
ADDR	LDTU	$4,$0
		ORH		$4,#0100
		PUT		rXX,$4
		LDOU	$4,$1,0
		PUT		rYY,$4
		LDOU	$4,$1,8
		PUT		rZZ,$4
		RESUME	1
		
		LOC		#1000

		% test continue at rWW (rX < 0)
Main	SET		$0,#200C	% jump over loads and the set
		ORH		$0,#8000
		SET		$1,#4000
		ORH		$1,#8000
		SETH	$2,#8000	% 8000000000100000
		ORML	$2,#0010
		SETH	$3,#8000	% 8000000000102000
		ORML	$3,#0010
		ORL		$3,#2000
		
		SETH	$4,#8000
		ORMH	$4,ADDR>>32
		ORML	$4,ADDR>>16
		ORL		$4,ADDR>>0
		PUT		rT,$4
		JMP		#2000
		
		LOC		#2000

HEADER

instrs = [
	"MUL", "MULI", "MULU", "MULUI", "DIV", "DIVI", "DIVU", "DIVUI", "ADD", "ADDI", "ADDU", "ADDUI",
	"SUB", "SUBI", "SUBU", "SUBUI", "2ADDU", "2ADDUI", "4ADDU", "4ADDUI", "8ADDU", "8ADDUI",
	"16ADDU", "16ADDUI", "CMP", "CMPI", "CMPU", "CMPUI", "NEG", "NEGI", "NEGU", "NEGUI", "SL",
	"SLI", "SLU", "SLUI", "SR", "SRI", "SRU", "SRUI",
	"CSN", "CSNI", "CSZ", "CSZI", "CSP", "CSPI", "CSOD", "CSODI", "CSNN", "CSNNI", "CSNZ", "CSNZI",
	"CSNP", "CSNPI", "CSEV", "CSEVI", "ZSN", "ZSNI", "ZSZ", "ZSZI", "ZSP", "ZSPI", "ZSOD", "ZSODI",
	"ZSNN", "ZSNNI", "ZSNZ", "ZSNZI", "ZSNP", "ZSNPI", "ZSEV", "ZSEVI",
	"OR", "ORI", "ORN", "ORNI", "NOR", "NORI", "XOR", "XORI", "AND", "ANDI", "ANDN", "ANDNI",
	"NAND", "NANDI", "NXOR", "NXORI", "BDIF", "BDIFI", "WDIF", "WDIFI", "TDIF", "TDIFI", "ODIF",
	"ODIFI", "MUX", "MUXI", "SADD", "SADDI", "MOR", "MORI", "MXOR", "MXORI",
	"SETH", "SETMH", "SETML", "SETL", "INCH", "INCMH", "INCML", "INCL", "ORH", "ORMH", "ORML",
	"ORL", "ANDNH", "ANDNMH", "ANDNML", "ANDNL"
]
addr = 0x100000
instrs.each { |i|
	printf("		%% -- #{i} #5 #6 -> #%x --\n",addr)
	puts <<DOC
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
DOC
	if i == "NEG" || i == "NEGI" || i == "NEGU" || i == "NEGUI"
		if i =~ /^[0-9A-Z]+I$/
			puts "		" + i[0...i.length - 1] + "		$6,5,6"
		else
			puts "		" + i + "		$6,5,$5"
		end
	elsif i =~ /^(SET|INC|OR|AND).*(H|MH|ML|L)$/
		puts "		" + i + "		$6,6"
	else
		if i =~ /^[0-9A-Z]+I$/
			puts "		" + i[0...i.length - 1] + "		$6,$4,6"
		else
			puts "		" + i + "		$6,$4,$5"
		end
	end
	puts <<DOC
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
DOC
	addr += 8
}

puts <<DOC
		
		SETH	$2,#8000	% 8000000000100000
		ORML	$2,#0010
		SETH	$3,#8000	% 8000000000102000
		ORML	$3,#0010
		ORL		$3,#2000
		
DOC

count = instrs.length * 8
printf("		%% sync #%x bytes\n",count)
while count > 0
	printf("		SYNCD	#%x,$2,0\n",[count,0xFF].min)
	printf("		ADD		$2,$2,#FF\n")
	printf("		SYNCD	#%x,$3,0\n",[count,0xFF].min)
	printf("		ADD		$3,$3,#FF\n")
	count -= 0xFF
end

puts <<DOC
		TRAP	0
DOC

