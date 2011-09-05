%
% pushpop.mms -- tests push/pop
%

		LOC		#1000

Main	PUT		rG,64
		PUT		rL,0				% discard arguments in mmix/mmmix


		% 0 args, 0 preserve, 0 return
		PUSHJ	$0,f1
		GET		$65,rL				% 0 return -> hole disappears -> 0
		JMP		1F

f1		GET		$64,rL				% 0 args -> 0
		POP		0,0


		% 1 args, 0 preserve, 0 return
1H		SET		$1,#11
		GETA	$0,f2
		PUSHGO	$0,$0,0
		GET		$68,rL				% 0 return -> hole disappears -> 0
		JMP		2F

f2		GET		$66,rL				% 1 arg -> 1
		ADD		$67,$0,0			% #11
		POP		0,0


		% 1 args, 1 preserve, 0 return
2H		SET		$0,#10
		SET		$2,#12
		GETA	$1,f3
		SET		$255,0
		PUSHGO	$1,$1,$255
		GET		$71,rL				% 0 return + 1 preserve -> 1
		ADD		$72,$0,0			% #10
		JMP		3F

f3		GET		$69,rL				% 1 arg -> 1
		ADD		$70,$0,0			% #12
		POP		0,0


f4		GET		$73,rL				% 1 arg -> 1
		ADD		$74,$0,0			% #12
		SET		$0,#13
		POP		1,0

		% 1 args, 1 preserve, 1 return
3H		SET		$0,#10
		SET		$2,#12
		PUSHJ	$1,f4
		GET		$75,rL				% 1 return + 1 preserve -> 2
		ADD		$76,$0,0			% #10
		ADD		$77,$1,0			% #13
		JMP		4F


		% 0 args, 0 preserve, 1 return
4H		PUT		rL,0				% discard regs
		PUSHJ	$0,f5
		GET		$79,rL				% 1 return + 0 preserve -> 1
		ADD		$80,$0,0			% #13
		JMP		5F

f5		GET		$78,rL				% 0 args -> 0
		SET		$0,#13
		POP		1,0


		% 1 args, 0 preserve, 1 return
5H		PUT		rL,0				% discard regs
		SET		$1,#12
		PUSHJ	$0,f6
		GET		$83,rL				% 1 return + 0 preserve -> 1
		ADD		$84,$0,0			% #13
		JMP		6F

f6		GET		$81,rL				% 1 args -> 1
		ADD		$82,$0,0			% #12
		SET		$0,#13
		POP		1,0


		% 0 args, 1 preserve, 1 return
6H		PUT		rL,0				% discard regs
		SET		$0,#10
		PUSHJ	$1,f7
		GET		$86,rL				% 1 return + 1 preserve -> 2
		ADD		$87,$0,0			% #10
		ADD		$88,$1,0			% #13
		JMP		7F

f7		GET		$85,rL				% 0 args -> 0
		SET		$0,#13
		POP		1,0


		% 0 args, 1 preserve, 0 return
7H		PUT		rL,0				% discard regs
		SET		$0,#10
		PUSHJ	$1,f8
		GET		$90,rL				% 0 return + 1 preserve -> 1
		ADD		$91,$0,0			% #10
		JMP		8F

f8		GET		$89,rL				% 0 args -> 0
		POP		0,0


		% 2 args, 2 preserve, 2 return
8H		PUT		rL,0				% discard regs
		SET		$0,#10
		SET		$1,#11
		SET		$3,#12
		SET		$4,#13
		PUSHJ	$2,f9
		GET		$95,rL				% 2 return + 2 preserve -> 4
		ADD		$96,$0,0			% #10
		ADD		$97,$1,0			% #11
		ADD		$98,$2,0			% #13
		ADD		$99,$3,0			% #12
		JMP		9F

f9		GET		$92,rL				% 2 args -> 2
		ADD		$93,$0,0			% #12
		ADD		$94,$1,0			% #13
		POP		2,0


9H		TRAP	0
			
