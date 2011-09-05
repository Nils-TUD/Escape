%
% saveunsave1.mms -- tests save/unsave
%

		LOC		#1000
	
Main	PUT		rG,251
		% set some global registers
		SET		$251,1
		SET		$252,2
		SET		$253,3
		SET		$254,4
		% set special registers
		PUT		rB,5
		PUT		rD,6
		PUT		rE,7
		PUT		rH,8
		PUT		rJ,9
		PUT		rM,10
		PUT		rR,11
		PUT		rP,12
		PUT		rW,13
		PUT		rX,14
		PUT		rY,15
		PUT		rZ,16
		PUT		rA,1
		% set some local registers
		SET		$0,17
		SET		$1,18
		SET		$2,19
		
		% now save the state
		SAVE	$251,0
		
		% overwrite everything with something else
		PUT		rG,64
		SET		$252,12
		SET		$253,13
		SET		$254,14
		PUT		rB,15
		PUT		rD,16
		PUT		rE,17
		PUT		rH,18
		PUT		rJ,19
		PUT		rM,20
		PUT		rR,21
		PUT		rP,22
		PUT		rW,23
		PUT		rX,24
		PUT		rY,25
		PUT		rZ,26
		PUT		rA,3
		SET		$0,1
		
		% now restore the state
		UNSAVE	0,$251
		
		GET		$3,rB
		GET		$4,rD
		GET		$5,rE
		GET		$6,rH
		GET		$7,rJ
		GET		$8,rM
		GET		$9,rR
		GET		$10,rP
		GET		$11,rW
		GET		$12,rX
		GET		$13,rY
		GET		$14,rZ
		GET		$15,rA
		
		TRAP	0
