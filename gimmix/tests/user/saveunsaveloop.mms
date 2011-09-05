%
% saveunsaveloop.mms -- tests save/unsave in a loop
%

		LOC		#0000
CNT		OCTA	#0
START	OCTA	#80

		LOC		#1000
	
Main	PUT		rL,0				% discard local vars
		PUT		rG,33
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
		
		SET		$0,0
		LDOU	$253,$0,START
SLOOP	SAVE	$254,0
		SUB		$253,$253,1
		STOU	$253,$0,CNT
		BNN		$253,SLOOP
		
		SET		$0,0
		LDOU	$253,$0,START
		STOU	$253,$0,CNT
		
ULOOP	UNSAVE	0,$254
		SET		$0,0
		LDOU	$253,$0,CNT
		SUB		$253,$253,1
		STOU	$253,$0,CNT
		BNN		$253,ULOOP
		
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
