%
% storeex-bug.mms -- demonstrates that MMIX-PIPE does not provide the value to store in rZZ
%

		% segmentsizes: 3,3,3,4; pageSize=2^13; r=0x90000; n=256
		LOC		#8000
RV		OCTA	#33340D0000090800

		% PTE to be able to execute the code
		LOC		#00090008
		OCTA	#0000000000002801	% PTE  1    (#0000000000002000 .. #0000000000003FFF)
		% PTE not writable
		LOC		#00090020
		OCTA	#0000000000208804	% PTE  4    (#0000000000008000 .. #0000000000009FFF)
		% PTE not writable, not readable
		LOC		#00090028
		OCTA	#000000000020A800	% PTE  5    (#000000000000A000 .. #000000000000BFFF)
		
		
		% data for writing to M[#8000], M[#A000]
		LOC		#208000
		OCTA	#123456789ABCDEF
		LOC		#20A000
		OCTA	#123456789ABCDEF
		
		
		% dynamic trap address
		LOC		#600000
		
ATRAP	GET		$9,rZZ
		ADDU	$13,$13,1			% count exceptions
		% store rZZ in $10, $11 or $12, respectivly
		BNZ		$10,1F				% if already used, to next
		SET		$10,$9				% store rZZ in $10
		JMP		3F
1H		BNZ		$11,2F				% if already used, to next
		SET		$11,$9
		JMP		3F
2H		SET		$12,$9
3H		GET		$255,rQ
		PUT		rQ,0				% reset rQ
		GET		$255,rXX			% skip instr
		ORH		$255,#8000
		PUT		rXX,$255
		SETL	$255,#000F
		ORMH	$255,#00FF			% set rK
		RESUME	1
		
		
		% forced trap address
		LOC		#500000
QUIT	TRAP	0					% quit
		
		
		LOC		#1000
		
		% first setup basic paging: 0 mapped to 0
Main	SETL	$0,RV
		ORH		$0,#8000
		LDOU	$0,$0
		PUT		rV,$0
		
		% setup rTT
		SETH	$0,#8000
		ORMH	$0,ATRAP>>32
		ORML	$0,ATRAP>>16
		ORL		$0,ATRAP>>0
		PUT		rTT,$0
		% setup rT
		SETH	$0,#8000
		ORMH	$0,QUIT>>32
		ORML	$0,QUIT>>16
		ORL		$0,QUIT>>0
		PUT		rT,$0
		
		% now go to user-mode (we are at #8000000000001000 atm)
		SETL	$255,#000F
		ORMH	$255,#00FF		% have to be set in usermode
		SET		$0,#2000
		PUT		rWW,$0
		SETH	$0,#8000
		PUT		rXX,$0
		RESUME	1
		
		
		LOC		#2000
		
		% write to readonly page; MMIX-PIPE does not provide the value to store in rZZ
		SETL	$0,#8000
		SETL	$1,#BEAF
		STOU	$1,$0,0		% #8000
		
		% write to readonly page (wyde); MMIX-PIPE does not provide the value to store in rZZ
		SETL	$0,#8001
		STWU	$1,$0,0
		
		% write to not-readable, not-writable page (wyde); the same as above
		SETL	$0,#A001
		STWU	$1,$0,0
		
		TRAP	1
