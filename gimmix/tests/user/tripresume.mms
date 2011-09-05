%
% tripresume.mms -- tests trip
%

		LOC		#0
		% forced trip
		PUSHJ	255,HTrip
		PUT		rJ,$255
		GET		$255,rB
		RESUME	0
		
		% #10 (div by zero)
		PUSHJ	255,HDiv
		PUT		rJ,$255
		GET		$255,rB
		RESUME	0
		
		% #20 (integer overflow)
		PUSHJ	255,HIOver
		PUT		rJ,$255
		GET		$255,rB
		RESUME	0
		
		LOC		#50
		% #50 (float overflow)
		PUSHJ	255,FLOver
		PUT		rJ,$255
		GET		$255,rB
		RESUME	0
		
		% #60 (float underflow)
		PUSHJ	255,FLUnder
		PUT		rJ,$255
		GET		$255,rB
		RESUME	0

		LOC		#1000
	
		% test forced trip
Main	PUT		rG,64
		SET		$64,0			% put some values into the stuff that is affected by TRIP 1,2,3
		SET		$72,0
		SET		$255,#1234
		SET		$2,#15
		SET		$3,#16
		PUT		rJ,#11
		TRIP	1,2,3
		SET		$72,1
		
		% enable arithmetic exceptions
		GET		$4,rA
		ORL		$4,#FF00
		PUT		rA,$4
		
		% test div by zero
		SET		$81,0
		SET		$6,0
		SET		$91,12
		DIV		$91,$2,$6		% raise div by zero exception
		SET		$81,1
		
		% test int overflow of add
		SET		$90,0
		SET		$5,1
		SLU		$5,$5,63		% $5 = #80...0
		ORN		$5,$6,$5		% $5 = ~$5
		SET		$92,#11
		ADD		$92,$5,$5		% raise int overflow ex
		SET		$90,1
		
		% test float overflow
		SETH	$93,#7FEF
		ORMH	$93,#FFFF
		ORML	$93,#FFFF
		ORL		$93,#FFFF
		FADD	$93,$93,$93
		SET		$101,1
		
		% test float underflow
		SETL	$102,#0001
		SETL	$103,#0002
		FSUB	$103,$102,$103
		SET		$110,1
		
		TRAP	0

HTrip	SET		$64,1			% just to indicate we got here..
		GET		$65,rX			% 80000000 + instr
		GET		$66,rY			% $Y
		GET		$67,rZ			% $Z
		GET		$68,rW			% next instr
		GET		$69,rB			% old $255
		ADD		$70,$255,0		% old rJ
		GET		$71,rA
		POP		0
	
HDiv	SET		$73,1			% just to indicate we got here..
		GET		$74,rX			% 80000000 + instr
		GET		$75,rY			% $Y
		GET		$76,rZ			% $Z
		GET		$77,rW			% next instr
		GET		$78,rB			% old $255
		ADD		$79,$255,0		% old rJ
		GET		$80,rA
		POP		0

HIOver	SET		$82,1			% just to indicate we got here..
		GET		$83,rX			% 80000000 + instr
		GET		$84,rY			% $Y
		GET		$85,rZ			% $Z
		GET		$86,rW			% next instr
		GET		$87,rB			% old $255
		ADD		$88,$255,0		% old rJ
		GET		$89,rA
		POP		0

FLOver	SET		$93,1			% just to indicate we got here..
		GET		$94,rX			% 80000000 + instr
		GET		$95,rY			% $Y
		GET		$96,rZ			% $Z
		GET		$97,rW			% next instr
		GET		$98,rB			% old $255
		ADD		$99,$255,0		% old rJ
		GET		$100,rA
		POP		0

FLUnder	SET		$102,1			% just to indicate we got here..
		GET		$103,rX			% 80000000 + instr
		GET		$104,rY			% $Y
		GET		$105,rZ			% $Z
		GET		$106,rW			% next instr
		GET		$107,rB			% old $255
		ADD		$108,$255,0		% old rJ
		GET		$109,rA
		POP		0
