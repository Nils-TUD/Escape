%
% mul.mms -- tests mul-instructions
%
		LOC		#1000

Main	NOR		$2,$2,$2		% = -1
		NOR		$3,$3,$3
		ANDNL	$3,#0001		% = -2
		NOR		$4,$4,$4
		ANDNL	$4,#0002		% = -3
		NOR		$5,$5,$5
		ANDNL	$5,#0007		% = -8
		SET		$6,0
		SET		$7,1
		SET		$8,2
		SET		$9,5
		
		% test mulu(i)
		MULU	$10,$6,$6		% 0 * 0
		GET		$11,rH
		
		MULU	$12,$7,$7		% 1 * 1
		GET		$13,rH
		
		MULU	$14,$8,$9		% 2 * 5
		GET		$15,rH
		
		MULU	$16,$2,$6		% FF..FF * 0
		GET		$17,rH
		
		MULU	$18,$2,$3		% FF..FF * FF..FE = FFFF.FFFF.FFFF.FFFD.0000.0000.0000.0002
		GET		$19,rH
		
		MULU	$20,$2,$7		% FF..FF * 1
		GET		$21,rH
		
		MULU	$22,$2,$8		% FF..FF * 2
		GET		$23,rH
		
		GET		$24,rA			% ensure that there was no arith-exception yet
		
		% test mul(i)
		MUL		$25,$6,$6		% 0 * 0
		GET		$26,rA
		PUT		rA,$6			% clear rA
		
		MUL		$27,$7,$7		% 1 * 1
		GET		$28,rA
		PUT		rA,$6			% clear rA
		
		MUL		$29,$2,0		% -1 * 0
		GET		$30,rA
		PUT		rA,$6			% clear rA
		
		MUL		$31,$2,$3		% -1 * -2 = 2
		GET		$32,rA
		PUT		rA,$6			% clear rA
		
		MUL		$33,$2,1		% -1 * 1
		GET		$34,rA
		PUT		rA,$6			% clear rA
		
		NOR		$35,$35,$35		% FF..FF
		SETH	$36,#8000
		ANDN	$35,$35,$36		% 7F..FF (= 2^63 - 1)
		
		MUL		$36,$35,2		% 7F..FF * 2 -> ex
		GET		$37,rA
		PUT		rA,$6			% clear rA
		
		MUL		$38,$35,$35		% 7F..FF * 7F..FF -> ex
		GET		$39,rA
		PUT		rA,$6			% clear rA
		
		SETH	$40,#8000
		MUL		$40,$3,$40		% FF..FE * 80..00 -> ex
		GET		$41,rA
		PUT		rA,$6			% clear rA
		
		TRAP	0
