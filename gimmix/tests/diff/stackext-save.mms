%
% stackext-save.mms -- test new special register rSS and related changes
%

		LOC		#00000000
		% segmentsizes: 1,0,0,0; pageSize=2^13; r=0x20000; n=0
RV		OCTA	#10000D0000020000

		LOC		#00020000
		OCTA	#0000000000200007	% PTE  0    (#0000000000000000 .. #0000000000001FFF)
		LOC		#00020008
		OCTA	#0000000000202007	% PTE  1    (#0000000000002000 .. #0000000000003FFF)
		LOC		#00020010
		OCTA	#0000000000204000	% PTE  2    (#0000000000004000 .. #0000000000005FFF)


		% stack for unsave
		LOC		#202008
		OCTA	#0							% rL
		OCTA	#0							% $254 = fe
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
STACK	OCTA	#FE00000000000000			% rG | rA


		% dynamic trap handler
		LOC		#400000
DTRAP	SAVE	$255,1						% save user-state

		% to next kernel-stack if we are on the first one
		GET		$0,rS
		SET		$2,#FFFF
		AND		$2,$0,$2
		SET		$1,#0A00
		CMP		$2,$2,$1
		BP		$2,NONEST					% is rS > #...A000, stop nesting
		GET		$5,rWW						% save some special-registers
		GET		$6,rXX
		GET		$7,rYY
		GET		$8,rZZ
		GET		$9,rBB
		SETMH	$0,#00FE					% set rK and raise exception
		PUT		rK,$0
		SET		$4,1

NONEST	SETML	$0,#0002
		ORL		$0,#0010
		ORH		$0,#8000
		LDOU	$1,$0,0						% load pte
		OR		$1,$1,#7					% give permission
		STOU	$1,$0,0						% store pte
		SET		$1,#4007
		LDVTS	$1,$1,0						% update DTC
		GET		$1,rQ
		PUT		rQ,0						% clear rQ

		% to prev kernel-stack if we were on the first one
		BZ		$4,NOREST
		PUT		rBB,$9						% restore special-registers
		PUT		rZZ,$8
		PUT		rYY,$7
		PUT		rXX,$6
		PUT		rWW,$5

NOREST	UNSAVE	1,$255						% restore user-state
		RESUME	1


		LOC		#1000

		% setup paging
Main	SETH	$0,#8000
		LDOU	$0,$0,0
		PUT		rV,$0

		% setup rS and rO
		SETL	$0,STACK
		UNSAVE	0,$0

		% setup rSS
		SETH	$0,#8000
		ORML	$0,#0020
		PUT		rSS,$0

		% setup dynamic-trap address
		SETH	$0,#8000
		ORMH	$0,DTRAP>>32
		ORML	$0,DTRAP>>16
		ORL		$0,DTRAP>>0
		PUT		rTT,$0

		SYNC	0
		GET		$0,rQ
		PUT		rQ,0

		% enable exceptions
		SETMH	$0,#00FE
		PUT		rK,$0

		SET		$252,#FFFF
		PUSHJ	$253,F1
		SETH	$2,#8000
		ORL		$2,#4000
		STOU	$252,$2,64
		SYNCD	#FF,$2,0
		TRAP	0

F1		SET		$0,#1110
		SET		$1,#1111
		GET		$252,rJ
		PUSHJ	$253,F2
		PUT		rJ,$252
		SETH	$2,#8000
		ORL		$2,#4000
		STOU	$0,$2,0
		STOU	$1,$2,8
		POP		0,0

F2		SET		$0,#2220
		SET		$1,#2222
		GET		$252,rJ
		PUSHJ	$253,F3
		PUT		rJ,$252
		SETH	$2,#8000
		ORL		$2,#4000
		STOU	$0,$2,16
		STOU	$1,$2,24
		POP		0,0

F3		SET		$0,#3330
		SET		$1,#3333
		GET		$252,rJ
		PUSHJ	$253,F4
		PUT		rJ,$252
		SETH	$2,#8000
		ORL		$2,#4000
		STOU	$0,$2,32
		STOU	$1,$2,40
		POP		0,0

F4		SET		$0,#4440
		SET		$1,#4444
		GET		$252,rJ
		PUSHJ	$253,F5
		PUT		rJ,$252
		SETH	$2,#8000
		ORL		$2,#4000
		STOU	$0,$2,48
		STOU	$1,$2,56
		POP		0,0

F5		SAVE	$254,0
		UNSAVE	0,$254
		POP		0,0
