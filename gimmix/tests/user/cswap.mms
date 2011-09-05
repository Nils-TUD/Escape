%
% cswap.mms -- tests CSWAP instruction
%

		LOC		#1000

Main	PUT		rP,#88
		SET		$0,#2000
		SET		$1,#89
		STOU	$1,$0,0
		
		SET		$2,#2233
		CSWAP	$2,$0,0
		GET		$3,rP
		
		SET		$4,#4455
		CSWAP	$4,$0,0
		GET		$5,rP
		
		SYNCD	#8,$0
		
		TRAP	0
