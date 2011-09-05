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

		% -- MUL #5 #6 -> #100000 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		MUL		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- MULI #5 #6 -> #100008 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		MUL		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- MULU #5 #6 -> #100010 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		MULU		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- MULUI #5 #6 -> #100018 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		MULU		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- DIV #5 #6 -> #100020 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		DIV		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- DIVI #5 #6 -> #100028 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		DIV		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- DIVU #5 #6 -> #100030 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		DIVU		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- DIVUI #5 #6 -> #100038 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		DIVU		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ADD #5 #6 -> #100040 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ADD		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ADDI #5 #6 -> #100048 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ADD		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ADDU #5 #6 -> #100050 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ADDU		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ADDUI #5 #6 -> #100058 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ADDU		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- SUB #5 #6 -> #100060 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		SUB		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- SUBI #5 #6 -> #100068 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		SUB		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- SUBU #5 #6 -> #100070 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		SUBU		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- SUBUI #5 #6 -> #100078 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		SUBU		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- 2ADDU #5 #6 -> #100080 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		2ADDU		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- 2ADDUI #5 #6 -> #100088 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		2ADDU		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- 4ADDU #5 #6 -> #100090 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		4ADDU		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- 4ADDUI #5 #6 -> #100098 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		4ADDU		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- 8ADDU #5 #6 -> #1000a0 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		8ADDU		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- 8ADDUI #5 #6 -> #1000a8 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		8ADDU		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- 16ADDU #5 #6 -> #1000b0 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		16ADDU		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- 16ADDUI #5 #6 -> #1000b8 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		16ADDU		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- CMP #5 #6 -> #1000c0 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		CMP		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- CMPI #5 #6 -> #1000c8 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		CMP		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- CMPU #5 #6 -> #1000d0 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		CMPU		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- CMPUI #5 #6 -> #1000d8 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		CMPU		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- NEG #5 #6 -> #1000e0 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		NEG		$6,5,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- NEGI #5 #6 -> #1000e8 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		NEG		$6,5,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- NEGU #5 #6 -> #1000f0 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		NEGU		$6,5,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- NEGUI #5 #6 -> #1000f8 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		NEGU		$6,5,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- SL #5 #6 -> #100100 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		SL		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- SLI #5 #6 -> #100108 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		SL		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- SLU #5 #6 -> #100110 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		SLU		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- SLUI #5 #6 -> #100118 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		SLU		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- SR #5 #6 -> #100120 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		SR		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- SRI #5 #6 -> #100128 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		SR		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- SRU #5 #6 -> #100130 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		SRU		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- SRUI #5 #6 -> #100138 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		SRU		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- CSN #5 #6 -> #100140 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		CSN		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- CSNI #5 #6 -> #100148 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		CSN		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- CSZ #5 #6 -> #100150 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		CSZ		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- CSZI #5 #6 -> #100158 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		CSZ		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- CSP #5 #6 -> #100160 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		CSP		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- CSPI #5 #6 -> #100168 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		CSP		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- CSOD #5 #6 -> #100170 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		CSOD		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- CSODI #5 #6 -> #100178 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		CSOD		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- CSNN #5 #6 -> #100180 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		CSNN		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- CSNNI #5 #6 -> #100188 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		CSNN		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- CSNZ #5 #6 -> #100190 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		CSNZ		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- CSNZI #5 #6 -> #100198 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		CSNZ		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- CSNP #5 #6 -> #1001a0 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		CSNP		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- CSNPI #5 #6 -> #1001a8 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		CSNP		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- CSEV #5 #6 -> #1001b0 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		CSEV		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- CSEVI #5 #6 -> #1001b8 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		CSEV		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ZSN #5 #6 -> #1001c0 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ZSN		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ZSNI #5 #6 -> #1001c8 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ZSN		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ZSZ #5 #6 -> #1001d0 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ZSZ		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ZSZI #5 #6 -> #1001d8 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ZSZ		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ZSP #5 #6 -> #1001e0 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ZSP		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ZSPI #5 #6 -> #1001e8 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ZSP		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ZSOD #5 #6 -> #1001f0 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ZSOD		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ZSODI #5 #6 -> #1001f8 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ZSOD		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ZSNN #5 #6 -> #100200 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ZSNN		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ZSNNI #5 #6 -> #100208 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ZSNN		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ZSNZ #5 #6 -> #100210 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ZSNZ		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ZSNZI #5 #6 -> #100218 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ZSNZ		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ZSNP #5 #6 -> #100220 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ZSNP		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ZSNPI #5 #6 -> #100228 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ZSNP		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ZSEV #5 #6 -> #100230 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ZSEV		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ZSEVI #5 #6 -> #100238 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ZSEV		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- OR #5 #6 -> #100240 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		OR		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ORI #5 #6 -> #100248 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		OR		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ORN #5 #6 -> #100250 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ORN		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ORNI #5 #6 -> #100258 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ORN		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- NOR #5 #6 -> #100260 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		NOR		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- NORI #5 #6 -> #100268 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		NOR		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- XOR #5 #6 -> #100270 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		XOR		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- XORI #5 #6 -> #100278 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		XOR		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- AND #5 #6 -> #100280 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		AND		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ANDI #5 #6 -> #100288 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		AND		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ANDN #5 #6 -> #100290 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ANDN		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ANDNI #5 #6 -> #100298 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ANDN		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- NAND #5 #6 -> #1002a0 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		NAND		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- NANDI #5 #6 -> #1002a8 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		NAND		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- NXOR #5 #6 -> #1002b0 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		NXOR		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- NXORI #5 #6 -> #1002b8 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		NXOR		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- BDIF #5 #6 -> #1002c0 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		BDIF		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- BDIFI #5 #6 -> #1002c8 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		BDIF		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- WDIF #5 #6 -> #1002d0 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		WDIF		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- WDIFI #5 #6 -> #1002d8 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		WDIF		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- TDIF #5 #6 -> #1002e0 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		TDIF		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- TDIFI #5 #6 -> #1002e8 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		TDIF		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ODIF #5 #6 -> #1002f0 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ODIF		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ODIFI #5 #6 -> #1002f8 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ODIF		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- MUX #5 #6 -> #100300 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		MUX		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- MUXI #5 #6 -> #100308 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		MUX		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- SADD #5 #6 -> #100310 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		SADD		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- SADDI #5 #6 -> #100318 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		SADD		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- MOR #5 #6 -> #100320 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		MOR		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- MORI #5 #6 -> #100328 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		MOR		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- MXOR #5 #6 -> #100330 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		MXOR		$6,$4,$5
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- MXORI #5 #6 -> #100338 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		MXOR		$6,$4,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- SETH #5 #6 -> #100340 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		SETH		$6,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- SETMH #5 #6 -> #100348 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		SETMH		$6,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- SETML #5 #6 -> #100350 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		SETML		$6,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- SETL #5 #6 -> #100358 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		SETL		$6,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- INCH #5 #6 -> #100360 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		INCH		$6,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- INCMH #5 #6 -> #100368 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		INCMH		$6,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- INCML #5 #6 -> #100370 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		INCML		$6,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- INCL #5 #6 -> #100378 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		INCL		$6,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ORH #5 #6 -> #100380 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ORH		$6,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ORMH #5 #6 -> #100388 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ORMH		$6,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ORML #5 #6 -> #100390 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ORML		$6,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ORL #5 #6 -> #100398 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ORL		$6,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ANDNH #5 #6 -> #1003a0 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ANDNH		$6,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ANDNMH #5 #6 -> #1003a8 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ANDNMH		$6,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ANDNML #5 #6 -> #1003b0 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ANDNML		$6,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		% -- ANDNL #5 #6 -> #1003b8 --
		% use instruction
		LDOU	$4,$1,0
		LDOU	$5,$1,8
		SET		$6,0
		ANDNL		$6,6
		STOU	$6,$2,0
		ADD		$2,$2,8			% to next storage-place
		
		% use resume
		SET		$6,0
		TRAP	1,2,3
		STOU	$6,$3,0
		ADD		$3,$3,8			% to next storage-place
		ADD		$0,$0,11*4		% to next instr
		
		
		SETH	$2,#8000	% 8000000000100000
		ORML	$2,#0010
		SETH	$3,#8000	% 8000000000102000
		ORML	$3,#0010
		ORL		$3,#2000
		
		% sync #3c0 bytes
		SYNCD	#ff,$2,0
		ADD		$2,$2,#FF
		SYNCD	#ff,$3,0
		ADD		$3,$3,#FF
		SYNCD	#ff,$2,0
		ADD		$2,$2,#FF
		SYNCD	#ff,$3,0
		ADD		$3,$3,#FF
		SYNCD	#ff,$2,0
		ADD		$2,$2,#FF
		SYNCD	#ff,$3,0
		ADD		$3,$3,#FF
		SYNCD	#c3,$2,0
		ADD		$2,$2,#FF
		SYNCD	#c3,$3,0
		ADD		$3,$3,#FF
		TRAP	0
