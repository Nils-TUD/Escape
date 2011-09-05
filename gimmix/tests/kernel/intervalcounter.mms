%
% intervalcounter.mms -- test the interval counter
%

		% dynamic trap address
		LOC		#600000
		
ATRAP	BNZ		$253,CONT
		TRAP	0
CONT	SUBU	$253,$253,1
		GET		$254,rQ
		ADDU	$252,$252,$254
		PUT		rQ,0
		PUT		rI,#16
		SETL	$255,#00FF
		RESUME	1
		
		
		LOC		#1000
		
Main	SETH	$0,#8000
		ORML	$0,#0060
		PUT		rTT,$0
		PUT		rK,#FF

		PUT		rG,252
		SET		$252,0
		SET		$253,5
		PUT		rI,#16
		
		SET		$0,0
1H		ADDU	$0,$0,1
		JMP		1B
