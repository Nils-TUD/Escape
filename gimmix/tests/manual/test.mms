		
		% stack for unsave
		LOC		#2000
		OCTA	#0							% rL
		OCTA	#0							% $255 = ff
		OCTA	#0							% rB
		OCTA	#0							% rD
		OCTA	#0							% rE
		OCTA	#0							% rH
		OCTA	#0							% rJ
		OCTA	#0							% rM
		OCTA	#0							% rP
		OCTA	#0							% rR
		OCTA	#0							% rW
		OCTA	#0							% rX
		OCTA	#0							% rY
		OCTA	#0							% rZ
STACK	OCTA	#FF00000000000000			% rG | rA
		
		% segmentsizes: 3,2,1,0; pageSize=2^13; r=0x90000; n=0
		LOC		#8000
RV		OCTA	#32100D0000090000
		
		LOC		#00090000
		OCTA	#0000000000000001	% PTE  0    (#0000000000000000 .. #0000000000001FFF)
		LOC		#00090008
		OCTA	#0000000000002007	% PTE  1    (#0000000000002000 .. #0000000000003FFF)
		
		LOC		#1000
Main	GETA	$0,RV
		LDOU	$0,$0,0
		PUT		rV,$0
		
		NEG		$0,0,1
		ANDNMH	$0,#01
		PUT		rK,$0

		GETA	$0,STACK
		ANDNH	$0,#8000
		UNSAVE	0,$0
		
		PUT		rG,224

		SET		$223,1
		PUSHJ	$224,F1

F1		SET		$223,1
		PUSHJ	$224,F2
F2		SET		$223,1
		PUSHJ	$224,F3
F3		SET		$223,1
		PUSHJ	$224,F4
F4		SET		$223,1
		PUSHJ	$224,F5
F5		SET		$152,1
		PUSHJ	$153,F6
F6		SET		$223,1

		TRAP	0
