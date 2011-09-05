%
% bit.mms -- tests bit-instructions
%

		LOC		#1000

			% prepare
Main	SETH	$2,#1111
		ORMH	$2,#2222
		ORML	$2,#3333
		ORL		$2,#4444
		
		SETH	$3,#FFFF
		ORMH	$3,#0FF0
		ORML	$3,#F00F
		ORL		$3,#00FF
		
		% test or(i)
		OR		$4,$2,#0F
		OR		$5,$2,#F0
		OR		$6,$2,$2
		OR		$7,$2,$3
		
		% test and(i)
		AND		$8,$2,#0F
		AND		$9,$2,#F0
		AND		$10,$2,$2
		AND		$11,$2,$3
		
		% test xor(i)
		XOR		$12,$2,#0F
		XOR		$13,$2,#F0
		XOR		$14,$2,$2
		XOR		$15,$2,$3
		
		% test andn(i)
		ANDN	$16,$2,#0F
		ANDN	$17,$2,#F0
		ANDN	$18,$2,$2
		ANDN	$19,$2,$3
		
		% test orn(i)
		ORN		$20,$2,#0F
		ORN		$21,$2,#F0
		ORN		$22,$2,$2
		ORN		$23,$2,$3
		
		% test nand(i)
		NAND	$24,$2,#0F
		NAND	$25,$2,#F0
		NAND	$26,$2,$2
		NAND	$27,$2,$3
		
		% test nor(i)
		NOR		$28,$2,#0F
		NOR		$29,$2,#F0
		NOR		$30,$2,$2
		NOR		$31,$2,$3
		
		% test nxor(i)
		NXOR	$32,$2,#0F
		NXOR	$33,$2,#F0
		NXOR	$34,$2,$2
		NXOR	$35,$2,$3
		
		% test mux(i)
		SETH	$36,#FFFF
		ORMH	$36,#0000
		ORML	$36,#FFFF
		ORL		$36,#0000
		PUT		rM,$36
		MUX		$37,$2,$3
		NOR		$38,$36,$36
		PUT		rM,$38
		MUX		$39,$3,$2
		
		TRAP	0
