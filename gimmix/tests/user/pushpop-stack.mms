%
% pushpop-stack.mms -- tests push/pop with stack-usage
%

		LOC		#1000

Main	PUT		rL,0				% discard arguments in mmix/mmmix

		% load/store on stack
		SET		$0,#1234
		SET		$1,#4567
		SET		$254,#10
		PUSHJ	$255,1F				% all 256 regs in use -> 1 is pushed
		STOU	$0,$2,#0
		STOU	$1,$2,#8
		GET		$0,rL				% 255
		STOU	$0,$2,#10
		STOU	$254,$2,#18	% #10
		JMP		2F

1H		SET		$0,#1234			% another one is pushed
		POP		0,0					% restores both regs


		% passing an arg to a nested func and returning it back
2H		PUT		rL,0				% discard regs
		SET		$1,#6666
		PUSHJ	$0,3F
		STOU	$0,$2,#20			% #6666
		JMP		5F
			
3H		ADD		$2,$0,0				% save arg
		GET		$0,rJ				% save rJ
		PUSHJ	$1,4F				% call 4F(arg)
		PUT		rJ,$0				% restore rJ
		ADD		$0,$1,0				% return arg
		POP		1,0

4H		POP		1,0					% return arg

5H		SYNCD	#20,$2,0			% flush to mem (for mmmix)
		TRAP	0
