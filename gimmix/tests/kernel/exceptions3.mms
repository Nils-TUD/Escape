%
% exceptions3.mms -- test exception because of invalid rV in usermode
%

		% segmentsizes: 0,0,0,0; pageSize=2^49; r=0x90000; n=256
		LOC		#8000
RV		OCTA	#0000310000090800

		LOC		#00090008
		OCTA	#0000000000002801	% PTE  1    (#0000000000002000 .. #0000000000003FFF)
		
		% dynamic trap address
		LOC		#600000
		
ATRAP	GET		$1,rWW				% get rWW - 4
		SUBU	$1,$1,4
		GET		$2,rQ
		GET		$3,rXX
		SRU		$3,$3,32
		TRAP	0					% quit


		LOC		#1000
		
		% first setup paging
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
		SET		$1,0
		SET		$2,0
		SET		$3,0
		RESUME	1
		
		
		LOC		#2000
		
LOOP	SET		$0,0			% rV is invalid, so it should raise an exception here
		JMP		LOOP
