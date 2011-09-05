%
% cswap-bug.mms -- tests the bug in MMIX-PIPE regarding CSWAP
%

		LOC		#1000
		
Main	PUT		rL,0		% just to be sure
		CSWAP	$1,$0,0		% should set rL to 2
		GET		$255,rL		% is actually 0 on MMIX-PIPE
		
		TRAP	0
