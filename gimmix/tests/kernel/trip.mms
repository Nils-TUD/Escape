%
% trip.mms -- test trips in kernel-mode (have no effect)
%

		% forced trip address
		LOC		#0
		SET		$0,#4567
		RESUME	0
		
		LOC		#1000
		
Main	TRIP	1,2,3
		SET		$0,#1234
		TRAP	0
