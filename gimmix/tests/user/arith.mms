%
% arith.mms -- tests basic arithmetic-instructions
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
		
		% test add(i)
		ADD		$10,$6,$6		% 0 + 0
		ADD		$11,$7,$9		% 1 + 5
		ADD		$12,$6,0		% 0 + 0
		ADD		$13,$9,2		% 5 + 2
		
		ADD		$14,$2,$8		% -1 + 2
		ADD		$15,$3,$2		% -2 + -1
		ADD		$16,$4,#FF		% -3 + #FF ; note: -1 is treated unsigned!
		ADD		$17,$5,$9		% -8 + 5
		
		% test addu(i)
		ADDU	$18,$6,$6		% 0 + 0
		ADDU	$19,$7,$9		% 1 + 5
		ADDU	$20,$6,0		% 0 + 0
		ADDU	$21,$9,2		% 5 + 2
		
		ADDU	$22,$2,$8		% FF..FF + 2
		ADDU	$23,$3,$2		% FF..FE + FF..FF
		ADDU	$24,$4,#FF		% FF..FD + FF
		ADDU	$25,$5,$9		% FF..F8 + 5
		
		% test sub(i)
		SUB		$26,$6,$6		% 0 - 0
		SUB		$27,$9,$7		% 5 - 1
		SUB		$28,$6,0		% 0 - 0
		SUB		$29,$9,2		% 5 - 2
		
		SUB		$30,$2,$8		% -1 - 2
		SUB		$31,$3,$2		% -2 - -1
		SUB		$32,$4,#FF		% -3 - #FF ; note: -1 is treated unsigned!
		SUB		$33,$5,$9		% -8 - 5
		
		% test subu(i)
		SUBU	$34,$6,$6		% 0 - 0
		SUBU	$35,$7,$9		% 1 - 5
		SUBU	$36,$6,0		% 0 - 0
		SUBU	$37,$9,2		% 5 - 2
		
		SUBU	$38,$2,$8		% FF..FF - 2
		SUBU	$39,$3,$2		% FF..FE - FF..FF
		SUBU	$40,$4,#FF		% FF..FD - FF
		SUBU	$41,$5,$9		% FF..F8 - 5
		
		% test 2addu(i)
		2ADDU	$42,$6,$6		% 2*0 + 0
		2ADDU	$43,$7,$9		% 2*1 + 5
		2ADDU	$44,$6,0		% 2*0 + 0
		2ADDU	$45,$9,2		% 2*5 + 2
		
		2ADDU	$46,$2,$8		% 2 * FF..FF + 2 = 1.FFFF.FFFF.FFFF.FFFE + 2 = 2.0000.0000.0000.0000 = 0
		2ADDU	$47,$3,$2		% 2 * FF..FE + FF..FF
		2ADDU	$48,$4,#FF		% 2 * FF..FD + FF
		2ADDU	$49,$5,$9		% 2 * FF..F8 + 5
		
		% test 4addu(i)
		4ADDU	$50,$6,$6		% 4*0 + 0
		4ADDU	$51,$7,$9		% 4*1 + 5
		4ADDU	$52,$6,0		% 4*0 + 0
		4ADDU	$53,$9,2		% 4*5 + 2
		
		4ADDU	$54,$2,$8		% 4 * FF..FF + 2 = 3.FFFF.FFFF.FFFF.FFFC + 2 = 3.FFFF.FFFF.FFFF.FFFE
		4ADDU	$55,$3,$2		% 4 * FF..FE + FF..FF
		4ADDU	$56,$4,#FF		% 4 * FF..FD + FF
		4ADDU	$57,$5,$9		% 4 * FF..F8 + 5
		
		% test 8addu(i)
		8ADDU	$58,$6,$6		% 8*0 + 0
		8ADDU	$59,$7,$9		% 8*1 + 5
		8ADDU	$60,$6,0		% 8*0 + 0
		8ADDU	$61,$9,2		% 8*5 + 2
		
		8ADDU	$62,$2,$8		% 8 * FF..FF + 2 = 7.FFFF.FFFF.FFFF.FFF8 + 2 = 7.FFFF.FFFF.FFFF.FFFA
		8ADDU	$63,$3,$2		% 8 * FF..FE + FF..FF
		8ADDU	$64,$4,#FF		% 8 * FF..FD + FF
		8ADDU	$65,$5,$9		% 8 * FF..F8 + 5
		
		% test 16addu(i)
		16ADDU	$66,$6,$6		% 16*0 + 0
		16ADDU	$67,$7,$9		% 16*1 + 5
		16ADDU	$68,$6,0		% 16*0 + 0
		16ADDU	$69,$9,2		% 16*5 + 2
		
		16ADDU	$70,$2,$8		% 16 * FF..FF + 2 = F.FFFF.FFFF.FFFF.FFF0 + 2 = F.FFFF.FFFF.FFFF.FFF2
		16ADDU	$71,$3,$2		% 16 * FF..FE + FF..FF
		16ADDU	$72,$4,#FF		% 16 * FF..FD + FF
		16ADDU	$73,$5,$9		% 16 * FF..F8 + 5
		
		% overflow preparations
		NOR		$74,$74,$74		% $74 = FF..FF
		SETH	$75,#8000
		ANDN	$74,$74,$75		% $74 = 7F..FF (= 2^63 - 1)
		GET		$76,rA			% ensure that there was no arith-exception yet
		
		% test add(i) overflow
		ADD		$80,$74,1		% 7F..FF + 1 -> ex
		GET		$81,rA
		PUT		rA,$6			% clear rA
		
		ADD		$82,$74,$74		% 7F..FF + 7F..FF -> ex
		GET		$83,rA
		PUT		rA,$6			% clear rA
		
		ADD		$84,$2,$2		% -1 + -1 -> NO ex
		GET		$85,rA
		PUT		rA,$6				% clear rA
		
		ADD		$86,$2,2		% -1 + 2 -> NO ex
		GET		$87,rA
		PUT		rA,$6				% clear rA
		
		% test sub(i) overflow
		
		SUB		$88,$75,1		% 80..00 - 1 -> ex
		GET		$89,rA
		PUT		rA,$6			% clear rA
		
		SUB		$90,$75,$75		% 80..00 - 80..00 -> NO ex
		GET		$91,rA
		PUT		rA,$6			% clear rA
		
		SUB		$92,$2,$2		% -1 - -1 -> NO ex
		GET		$93,rA
		PUT		rA,$6			% clear rA
		
		SUB		$94,$2,2		% -1 - 2 -> NO ex
		GET		$95,rA
		PUT		rA,$6			% clear rA
		
		SUB		$96,$75,$74		% 80..00 - 7F..FF -> ex
		GET		$97,rA
		PUT		rA,$6			% clear rA
		
		TRAP	0
