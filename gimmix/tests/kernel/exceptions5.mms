%
% exceptions5.mms -- test rWW for execution-protection-faults
%

		% segmentsizes: 1,0,0,0; pageSize=2^13; r=0x90000; n=256
		LOC		#8000
RV		OCTA	#10000D0000090800

		LOC		#00090008
		OCTA	#0000000000002800	% PTE  1    (#0000000000002000 .. #0000000000003FFF)
		
		% dynamic trap address
		LOC		#600000
		
ATRAP	GET		$1,rWW
		GET		$2,rQ
		GET		$3,rXX
		GET		$4,rYY
		TRAP	0					% quit


		LOC		#1000
		
		% first setup paging
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
		
		% now go to user-mode (we are at #8000000000001000 atm)
		NEG		$255,0,1
		SET		$0,#2000
		PUT		rWW,$0
		SETH	$0,#8000
		PUT		rXX,$0
		SET		$1,0
		SET		$2,0
		SET		$3,0
		RESUME	1
		
		
		LOC		#2000
		
LOOP	SET		$0,0			% not executable -> exec-protection-fault
		JMP		LOOP
