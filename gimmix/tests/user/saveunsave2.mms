%
% saveunsave2.mms -- tests save/unsave without global regs and local regs
%

		LOC		#1000
	
Main	PUT		rL,0				% discard local vars
		PUT		rG,254				% we need at least two; 255 is special, one for SAVE
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
		
		% now save the state
		SAVE	$254,0
		
		% overwrite everything with something else
		PUT		rG,64
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
		
		% now restore the state
		UNSAVE	0,$254
		
		GET		$254,rL
		ADD		$1,$254,0			% indirectly because getting rL into a local reg, 
		GET		$2,rG				% may change rL (before)
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
