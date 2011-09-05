%
% step.mms -- tests stepout and stepover
%

		LOC		#1000

Main	PUSHJ	$0,F1
		PUSHJ	$0,F2
		PUSHJ	$0,F3
		PUSHJ	$0,F1
		TRAP	0,0,0

F1		GET		$0,rJ
		PUSHJ	$1,F2
		PUT		rJ,$0
		POP		0,0

F2		GET		$0,rJ
		PUSHJ	$1,F3
		PUT		rJ,$0
		POP		0,0

F3		POP		0,0
