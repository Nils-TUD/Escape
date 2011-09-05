%
% pushpop-rec.mms -- tests recursion with push/pop
%

		LOC		#1000

Main	PUT		rL,0				% discard arguments in mmix/mmmix

		SET		$1,0
		PUSHJ	$0,FIB
		
		SET		$2,1
		PUSHJ	$1,FIB
		
		SET		$3,2
		PUSHJ	$2,FIB
		
		SET		$4,3
		PUSHJ	$3,FIB
		
		SET		$5,4
		PUSHJ	$4,FIB
		
		SET		$6,5
		PUSHJ	$5,FIB
		
		SET		$7,6
		PUSHJ	$6,FIB
		
		SET		$8,7
		PUSHJ	$7,FIB
		
		SET		$9,12
		PUSHJ	$8,FIB
		
		SET		$10,15
		PUSHJ	$9,FIB

		TRAP	0


% octa FIB(octa n)
FIB		GET		$1,rJ				% save rJ
		CMPU	$2,$0,1				% compare with 1
		BNP		$2,1F				% n <= 1? then return n
		SUBU	$4,$0,2
		PUSHJ	$3,FIB				% $3 = FIB(n - 2)
		SUBU	$20,$0,1			% use a higher register to produce stack stores/loads
		PUSHJ	$19,FIB				% $19 = FIB(n - 1)
		ADDU	$0,$3,$19			% return $3 + $19
1H		PUT		rJ,$1				% restore rJ
		POP		1,0
