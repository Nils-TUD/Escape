%
% pushpop-rg.mms -- tests push/pop when changing rG in between
%

		LOC		#1000
		
Main	SET		$28,#11
		SET		$29,#22
		SET		$30,#33
		SET		$31,#44
		SET		$32,#55
		SET		$33,#66
		SET		$34,#77
		SET		$35,#88
		SET		$253,#99
		PUSHJ	$254,F1
		
		% $28..$35 should still be present; of course, $253 is lost, as is the return-value
		GET		$0,rO
		GET		$1,rS
		GET		$2,rL
		TRAP	0,0,0

F1		GET		$0,rJ				% save rJ
		SET		$248,#1234			% cause some stores on the stack
		PUSHJ	$249,F2
		PUT		rJ,$0				% restore rJ and return it
		POP		1,0
		
F2		GET		$35,rJ				% save rJ (in $X with X < 36)
		SET		$40,#1234
		PUSHJ	$41,F3
		PUT		rJ,$35				% restore rJ
		SET		$0,$35				% return it
		POP		1,0

F3		SET		$35,#1234			% cause more stores
		PUT		rG,36				% now decrease rG
		POP		0,0
