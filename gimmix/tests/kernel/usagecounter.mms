%
% usagecounter.mms -- tests the usage-counter (rU)
%

		% segmentsizes: 1,0,0,0; pageSize=2^13; r=0x90000; n=0
		LOC		#8000
RV		OCTA	#10000D0000090000

		% -- PTEs to be able to execute the code --
		LOC		#00090000
		OCTA	#0000000000000001	% PTE  0    (#0000000000000000 .. #0000000000001FFF)
		LOC		#00090008
		OCTA	#0000000000002001	% PTE  1    (#0000000000002000 .. #0000000000003FFF)
		

		LOC		#500000
ATRAP5	GET		$255,rXX
		AND		$255,$255,#1		% 1 means continue, not 1 means stop
		BNZ		$255,1F
		TRAP	0
1H		SETMH	$255,#FFFF			% count instructions at negative addresses, test overflow
		ORML	$255,#FFFF
		ORL		$255,#FFFA
		PUT		rU,$255
2H		SET		$255,0
		NOR		$255,$255,$255		% set rK to -1
		RESUME	1
		
ATRAP4	GET		$255,rXX
		AND		$255,$255,#1		% 1 means continue, not 1 means go to next
		BNZ		$255,1F
		SETH	$255,#8000
		ORMH	$255,ATRAP5>>32
		ORML	$255,ATRAP5>>16
		ORL		$255,ATRAP5>>0
		PUT		rT,$255
		JMP		2F
1H		SETMH	$255,#8000			% count instructions at negative addresses
		PUT		rU,$255
2H		SET		$255,0
		NOR		$255,$255,$255		% set rK to -1
		RESUME	1
		
ATRAP3	GET		$255,rXX
		AND		$255,$255,#1		% 1 means continue, not 1 means go to next
		BNZ		$255,1F
		SETH	$255,#8000
		ORMH	$255,ATRAP4>>32
		ORML	$255,ATRAP4>>16
		ORL		$255,ATRAP4>>0
		PUT		rT,$255
		JMP		2F
1H		SETH	$255,#4040			% 4X = branches
		PUT		rU,$255
2H		SET		$255,0
		NOR		$255,$255,$255		% set rK to -1
		RESUME	1

ATRAP2	GET		$255,rXX
		AND		$255,$255,#1		% 1 means continue, not 1 means go to next
		BNZ		$255,1F
		SETH	$255,#8000
		ORMH	$255,ATRAP3>>32
		ORML	$255,ATRAP3>>16
		ORL		$255,ATRAP3>>0
		PUT		rT,$255
		JMP		2F
1H		SETH	$255,#F2F3			% F2,F3 = pushj,pushjb
		PUT		rU,$255
2H		SET		$255,0
		NOR		$255,$255,$255		% set rK to -1
		RESUME	1

ATRAP	GET		$255,rXX
		AND		$255,$255,#1		% 1 means continue, not 1 means go to next
		BNZ		$255,1F
		SETH	$255,#8000
		ORMH	$255,ATRAP2>>32
		ORML	$255,ATRAP2>>16
		ORL		$255,ATRAP2>>0
		PUT		rT,$255
		JMP		2F
1H		PUT		rU,0				% simply count all instructions
2H		SET		$255,0
		NOR		$255,$255,$255		% set rK to -1
		RESUME	1


		LOC		#1000

		% first setup basic paging: 0 mapped to 0
Main	SETL	$0,RV
		ORH		$0,#8000
		LDOU	$0,$0
		PUT		rV,$0
		
		% setup rT
		SETH	$0,#8000
		ORMH	$0,ATRAP>>32
		ORML	$0,ATRAP>>16
		ORL		$0,ATRAP>>0
		PUT		rT,$0
		
		% now go to user-mode (we are at #8000000000001000 atm)
		SET		$255,0
		NOR		$255,$255,$255
		SET		$0,#2000
		PUT		rWW,$0
		SETH	$0,#8000
		PUT		rXX,$0
		RESUME	1
		
		
		LOC		#2000
	
		% count all
		TRAP	1
		SET		$1,8
		PUSHJ	$0,FIB
		GET		$1,rU
		
		TRAP	1
		SET		$3,4
		PUSHJ	$2,FIB
		GET		$3,rU
		
		TRAP	1
		SET		$5,7
		PUSHJ	$4,FIB
		GET		$5,rU
		
		% count pushj
		TRAP	2
		
		TRAP	1
		SET		$7,3
		PUSHJ	$6,FIB
		GET		$7,rU
		
		TRAP	1
		SET		$9,7
		PUSHJ	$8,FIB
		GET		$9,rU
		
		TRAP	1
		SET		$11,6
		PUSHJ	$10,FIB
		GET		$11,rU
		
		% count branches
		TRAP	2
		
		TRAP	1
		SET		$13,6
		PUSHJ	$12,FIB
		GET		$13,rU
		
		TRAP	1
		SET		$15,7
		PUSHJ	$14,FIB
		GET		$15,rU

		TRAP	1
		SET		$17,4
		PUSHJ	$16,FIB
		GET		$17,rU
		
		% count instructions @ negative addresses
		TRAP	2
		
		TRAP	1
		OR		$0,$0,$0
		OR		$0,$0,$0
		OR		$0,$0,$0
		OR		$0,$0,$0
		GET		$18,rU

		% count instructions @ negative addresses, test overflow
		TRAP	2
		
		TRAP	1
		OR		$0,$0,$0
		OR		$0,$0,$0
		OR		$0,$0,$0
		OR		$0,$0,$0
		GET		$19,rU
		
		% stop
		TRAP	2


% octa FIB(octa n)
FIB		GET		$1,rJ				% save rJ
		CMPU	$2,$0,1				% compare with 1
		BNP		$2,1F				% n <= 1? then return n
		SUBU	$4,$0,2
		PUSHJ	$3,FIB				% $3 = FIB(n - 2)
		SUBU	$20,$0,1			% use a higher register to produce stack stores/loads
		PUSHJ	$19,FIB				% $19 = FIB(n - 1)
		ADDU	$0,$3,$19			% return $3 + $19
1H		PUT		rJ,$1				% restore rJ
		POP		1,0
