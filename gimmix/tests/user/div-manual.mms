%
% div.mms -- tests div-instructions
%
		LOC		#1000

Main	NOR		$2,$2,$2	% = -1
		NOR		$3,$3,$3
		ANDNL	$3,#0001	% = -2
		NOR		$4,$4,$4
		ANDNL	$4,#0002	% = -3
		NOR		$5,$5,$5
		ANDNL	$5,#0007	% = -8
		SET		$6,0
		SET		$7,1
		SET		$8,5
		SET		$9,10
		
		% test divu(i)
		DIVU	$10,$7,$7		% 1 / 1
		GET		$11,rR
		
		DIVU	$12,$9,$8		% 10 / 5
		GET		$13,rR
		
		DIVU	$14,$8,$9		% 5 / 10
		GET		$15,rR
		
		DIVU	$16,$9,3		% 10 / 3
		GET		$17,rR
		
		DIVU	$18,$2,16		% FF..FF / 16
		GET		$19,rR
		
		PUT		rD,$2
		DIVU	$20,$2,16		% FF..FF FF..FF / 16
		GET		$21,rR			% since rD >= 16, the result is FF..FF (rD) and the remainder
								% FF..FF ($2)
		
		MULU	$22,$2,16		% FF..FF * 16
		GET		$23,rH
		PUT		rD,$23
		DIVU	$24,$22,16		% F FF..FF / 16
		GET		$25,rR
		
		% test div(i)
		DIV		$26,$7,$7		% 1 / 1 => 1:0
		GET		$27,rR
		
		DIV		$28,$9,$8		% 10 / 5 => 2:0
		GET		$29,rR
		
		DIV		$30,$8,$9		% 5 / 10 => 0:5
		GET		$31,rR
		
		DIV		$32,$9,3		% 10 / 3 => 3:1 ((10 - 1) % 3 == 0)
		GET		$33,rR
		
		DIV		$34,$9,$4		% 10 / -3 => -4:-2 ((10 - -2) % 3 == 0)
		GET		$35,rR
		
		NOR		$36,$36,$36
		ANDNL	$36,#0009		% = -10
		DIV		$37,$36,3		% -10 / 3 => -4:2 ((-10 - 2) % 3 == 0)
		GET		$38,rR
		
		DIV		$39,$36,$4		% -10 / -3 => 3:-1 ((-10 - -1) % 3 == 0)
		GET		$40,rR
		
		DIV		$41,$36,5		% -10 / 5 => -2:0
		GET		$42,rR
		
		NOR		$43,$43,$43
		ANDNL	$43,#0004		% = -5
		DIV		$44,$9,$43		% 10 / -5 => -2:0
		GET		$45,rR
		
		DIV		$46,$36,$43		% -10 / -5 => 2:0
		GET		$47,rR
		
		% test div by zero
		GET		$48,rA			% first, ensure that no arith-exception occurred yet
		SET		$49,1
		DIV		$49,$9,$6		% 10 / 0 => 0:10 + ex
		GET		$50,rR
		GET		$51,rA
		PUT		rA,$6			% clear rA
		
		% test big numbers
		NOR		$52,$52,$52
		ANDNH	$52,#8000		% = 2^63 - 1
		DIV		$53,$52,$52		% (2^63 - 1) / (2^63 - 1) = 1:0
		GET		$54,rR
		
		% test div-overflow
		SETH	$55,#8000		% = -2^63
		DIV		$56,$55,$2		% -2^63 / -1 = -2^63:0 + ex
		GET		$57,rR
		GET		$58,rA
		
		TRAP	0
