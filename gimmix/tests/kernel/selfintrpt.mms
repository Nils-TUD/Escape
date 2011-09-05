%
% selfintrpt.mms -- tests setting rQ and rK so that a dynamic trap is triggered
%

		% dynamic trap address
		LOC		#600000
		
		% don't store rWW because its different in gimmix and mmmix
ATRAP	GET		$253,rQ
		STOU	$253,$254,0
		GET		$251,rXX
		SRU		$251,$251,32
		STOU	$251,$254,8
		ADD		$254,$254,16
		PUT		rQ,0
		RESUME	1

		
		LOC		#1000
		
		% setup rTT
Main	SETH	$0,#8000
		ORMH	$0,ATRAP>>32
		ORML	$0,ATRAP>>16
		ORL		$0,ATRAP>>0
		PUT		rTT,$0
		% setup address for results
		PUT		rG,250
		SETH	$254,#8000
		ORL		$254,#4000
		
		SETMH	$0,#2
		PUT		rQ,$0
		PUT		rK,$0			% raises exception (gimmix will raise an ex here)
		OR		$0,$0,0			% nop (mmmix will raise an ex here)
		
		% sync
		SETH	$254,#8000
		ORL		$254,#4000
		SYNCD	#FF,$254,0
		% now quit
		TRAP	0
