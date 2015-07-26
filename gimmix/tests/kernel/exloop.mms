%
% exloop.mms -- test the raise of an exception whose handler does not zero rQ
%

		% paging setup: 1MiB pages, 1024 pages in seg0
		% one PTE in root-location that maps 0 to 0
		LOC		#8000
RV		OCTA	#1000140000090000
		LOC		#90000
PTE		OCTA	#0000000000000007

		% dynamic trap address
		LOC		#600000

		% dont store the location here because it differs. mmmix will execute one instruction
		% after each RESUME, gimmix wont
ATRAP	GET		$253,rQ
		STOU	$253,$254,0
		GET		$251,rXX
		SRU		$251,$251,32
		STOU	$251,$254,8
		ADD		$254,$254,16
		SUB		$250,$250,1
		BNN		$250,_DONE
		PUT		rQ,0
_DONE	SETL	$255,#000F
		ORMH	$255,#00FF		% set rK
		RESUME	1


		% forced trap address
		LOC		#500000

QUIT	SETH	$0,#8000
		ORL		$0,#4000
		SYNCD	#FF,$0,0
		TRAP	0


		LOC		#1000

		% first setup basic paging: 0 mapped to 0
Main	SETL	$0,RV
		ORH		$0,#8000
		LDOU	$0,$0
		PUT		rV,$0
		% global registers are better here because of PUSH/POP
		PUT		rG,128
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
		% setup address for results
		SETH	$254,#8000
		ORL		$254,#4000
		% set rK
		SETMH	$0,#00FE
		PUT		rK,$0

		% now go to user-mode (we are at #8000000000001000 atm)
		SETL	$0,#000F
		ORMH	$0,#00FF		% have to be set in usermode
		SET		$255,$0
		SET		$0,#2000
		PUT		rWW,$0
		SETH	$0,#8000
		PUT		rXX,$0
		RESUME	1


		LOC		#2000

		SET		$250,3			% stop after 3 exceptions

		% use a special-register >= 32 with put
		PUT		33,#0

		% some dummy instructions which will cause an exception because rQ & rK is still != 0
		SET		$0,0
		SET		$0,0
		SET		$0,0

		% now quit
		TRAP	1
