%
% stackext-push.mms -- test new special register rSS and related changes
%
		
		LOC		#1000

Main	PUT		rG,#FE

		SET		$252,#FFFF
		PUSHJ	$253,F1
		ORL		$2,#4000
		STOU	$252,$2,64
		SYNCD	#FF,$2
		TRAP	0
		
F1		SET		$0,#1110
		SET		$1,#1111
		GET		$252,rJ
		PUSHJ	$253,F2
		PUT		rJ,$252
		ORL		$2,#4000
		STOU	$0,$2,0
		STOU	$1,$2,8
		POP		0,0

F2		SET		$0,#2220
		SET		$1,#2222
		GET		$252,rJ
		PUSHJ	$253,F3
		PUT		rJ,$252
		ORL		$2,#4000
		STOU	$0,$2,16
		STOU	$1,$2,24
		POP		0,0

F3		SET		$0,#3330
		SET		$1,#3333
		GET		$252,rJ
		PUSHJ	$253,F4
		PUT		rJ,$252
		ORL		$2,#4000
		STOU	$0,$2,32
		STOU	$1,$2,40
		POP		0,0

F4		SET		$0,#4440
		SET		$1,#4444
		GET		$252,rJ
		PUSHJ	$253,F5
		PUT		rJ,$252
		ORL		$2,#4000
		STOU	$0,$2,48
		STOU	$1,$2,56
		POP		0,0

F5		GET		$7,rJ
		PUSHJ	$10,F6
		PUT		rJ,$7
		POP		0,0

F6		POP		0,0
