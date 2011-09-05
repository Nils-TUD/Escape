%
% pushpop-special.mms -- tests push/pop (special cases)
%

		LOC		#1000

Main	PUT		rG,64
		PUT		rL,0				% discard arguments in mmix/mmmix


		% jump over instructions with pop
		SET		$64,0
		PUSHJ	$0,1F
1H		POP		0,2
		ADD		$64,$64,1			% not executed
		ADD		$64,$64,1
		
		
		% returning more values than we have
		PUT		rL,0
		SET		$0,#10
		PUSHJ	$1,2F
		GET		$65,rL				% 1 return + 1 hole + 1 preserved -> 3
		ADD		$66,$0,0			% #10
		ADD		$67,$1,0			% #0
		ADD		$68,$2,0			% #11
		JMP		3F

2H		SET		$0,#11
		POP		3,0
			
			
		% returning a value with rL = 0
3H		PUT		rL,0
		SET		$0,#10
		PUSHJ	$1,4F
		GET		$69,rL				% 0 return + 1 hole + 1 preserved -> 2
		ADD		$70,$0,0			% #10
		ADD		$71,$1,0			% #0
		JMP		5F

4H		POP		1,0


		% push all local registers
5H		PUT		rL,0
		SET		$2,#1234
		PUSHJ	$255,6F
		GET		$73,rL				% 3
		ADD		$74,$2,0			% #1234
		JMP		7F
6H		GET		$72,rL				% 0
		POP		0,0

		
		% manipulate the stack: give pop a value > rG for the number of registers of the caller
7H		SET		$63,0
		PUSHJ	$64,1F

1H		GET		$0,rJ
		SET		$63,0
		PUSHJ	$64,2F
		GET		$75,rL				% should be 64, not 65
		TRAP	0					% we can't really continue here because we've destroyed our
									% stack (loaded 1 value too much from the stack)


2H		GET		$0,rJ
		SET		$63,0
		PUSHJ	$64,3F
		PUT		rJ,$0
		POP		0,0
		
3H		GET		$0,rJ
		SET		$63,0
		PUSHJ	$64,4F
		PUT		rJ,$0
		POP		0,0
		
4H		GET		$0,rJ
		SET		$63,0
		PUSHJ	$64,5F
		PUT		rJ,$0
		POP		0,0
		
5H		SET		$63,0
		SETH	$0,#6000
		ORL		$0,#0408
		STCO	65,$0,0		% overwrite it
		POP		0,0
