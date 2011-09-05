%
% trapresume.mms -- test trap
%
		
		% dynamic trap address
		LOC		#600000

		RESUME	1

		% forced trap addresses
		LOC		#500000
		
		% test special-register-values and simple continue at rWW
ADDR	GET		$1,rXX
		GET		$2,rYY
		GET		$3,rZZ
		GET		$4,rWW
		GET		$5,rK
		GET		$6,rBB
		ADD		$7,$255,0
		RESUME	1

		% test RESUME_SET
ADDR2	SETH	$8,#0200
		ORML	$8,#0008		% we cant use a register >= rL
		PUT		rXX,$8
		PUT		rZZ,#FF			% set $8 to #FF
		RESUME	1
		
		% test RESUME_SET with arith ex
ADDR3	SETH	$9,#0200
		ORMH	$9,#7800		% raise arith exceptions 0x78
		ORML	$9,#0009		% we cant use a register >= rL
		PUT		rXX,$9
		PUT		rZZ,#FE			% set $9 to #FE
		RESUME	1
		
		% test RESUME_AGAIN
ADDR4	SETH	$11,#0000
		ORML	$11,#210B
		ORL		$11,#0B02		% execute ADD $11,$11,2
		PUT		rXX,$11
		SET		$11,0
		RESUME	1
		
		
		% test RESUME_AGAIN with arith ex
ADDR5	SETH	$12,#0000
		ORML	$12,#1D0C
		ORL		$12,#0C00		% execute DIV $12,$12,0
		PUT		rXX,$12
		SET		$12,#F1
		RESUME	1
		
		% test RESUME_AGAIN illegal instr exception
ADDR6	GET		$16,rXX
		GET		$17,rWW
		SETH	$15,#0000
		ORML	$15,#FA00
		ORL		$15,#0000		% execute SAVE $0 (0 < G)
		PUT		rXX,$15
		% this resume causes an illegal instruction exception
		% we will jump to rTT (top of file). there we do a resume again which will jump
		% to the instruction after the following resume
		RESUME	1
		% so, we will get here. now simply restore rXX and rWW and resume again
		% this will succeed so that we get back to the end of the file
		PUT		rXX,$16			% restore rXX and rWW
		PUT		rWW,$17
		RESUME	1
		
		% test RESUME_CONT with DSS
ADDR7	SETH	$16,#0100
		ORML	$16,#2110
		ORL		$16,#1000
		PUT		rXX,$16
		PUT		rYY,#F9
		PUT		rZZ,#01			% execute ADD $16,#F9,#01
		RESUME	1
		
		% test RESUME_CONT with DS
ADDR8	SETH	$17,#0100
		ORML	$17,#E311
		ORL		$17,#0000
		PUT		rXX,$17
		SET		$17,#F901
		PUT		rZZ,$17			% execute SETL $17,#F901
		RESUME	1
		
		% trap-target for RESUME_CONT with Trap
ADDR10	SET		$18,1
		RESUME	1
		
		% test RESUME_CONT with Trap
ADDR9	GET		$100,rXX			% backup rXX and rWW
		GET		$101,rWW
		SETH	$18,#0100
		ORML	$18,#0001
		ORL		$18,#0000
		PUT		rXX,$18			% execute TRAP 1,0,0
		SETH	$18,#8000
		ORMH	$18,ADDR10>>32
		ORML	$18,ADDR10>>16
		ORL		$18,ADDR10>>0
		PUT		rT,$18			% set trap addr to ADDR10
		RESUME	1
		% we will get here after resuming in ADDR10
		% so restore the original rXX and rWW and resume again
		PUT		rXX,$100
		PUT		rWW,$101
		RESUME	1
		
		LOC		#1000

		% test continue at rWW (rX < 0)
Main	SETH	$0,#8000
		ORMH	$0,ADDR>>32
		ORML	$0,ADDR>>16
		ORL		$0,ADDR>>0
		PUT		rT,$0
		
		% prepare a values to test
		ADD		$255,$1,4
		PUT		rJ,5
		PUT		rK,#0F
		SET		$2,#1234
		SET		$3,#5678
		
		TRAP	1,2,3
		ADD		$20,$255,0
		
		% test RESUME_SET
		SETH	$0,#8000
		ORMH	$0,ADDR2>>32
		ORML	$0,ADDR2>>16
		ORL		$0,ADDR2>>0
		PUT		rT,$0
		TRAP	1,2,3
		
		% test RESUME_SET with arith ex
		SETH	$0,#8000
		ORMH	$0,ADDR3>>32
		ORML	$0,ADDR3>>16
		ORL		$0,ADDR3>>0
		PUT		rT,$0
		TRAP	1,2,3
		GET		$10,rA
		
		% test RESUME_AGAIN
		SETH	$0,#8000
		ORMH	$0,ADDR4>>32
		ORML	$0,ADDR4>>16
		ORL		$0,ADDR4>>0
		PUT		rT,$0
		TRAP	1,2,3
		
		% test RESUME_AGAIN with arith ex
		SETH	$0,#8000
		ORMH	$0,ADDR5>>32
		ORML	$0,ADDR5>>16
		ORL		$0,ADDR5>>0
		PUT		rT,$0
		SETL	$0,#FF00
		PUT		rA,$0
		TRAP	1,2,3
		GET		$13,rA
		GET		$14,rR
		
		% test RESUME_AGAIN with illegal instr exception
		SETH	$0,#8000
		ORMH	$0,ADDR6>>32
		ORML	$0,ADDR6>>16
		ORL		$0,ADDR6>>0
		PUT		rT,$0
		SETH	$0,#8000
		ORMH	$0,#6000
		PUT		rTT,$0
		TRAP	1,2,3
		SET		$15,1
		
		% test RESUME_CONT with DSS
		SETH	$0,#8000
		ORMH	$0,ADDR7>>32
		ORML	$0,ADDR7>>16
		ORL		$0,ADDR7>>0
		PUT		rT,$0
		TRAP	1,2,3
		
		% test RESUME_CONT with DS
		SETH	$0,#8000
		ORMH	$0,ADDR8>>32
		ORML	$0,ADDR8>>16
		ORL		$0,ADDR8>>0
		PUT		rT,$0
		TRAP	1,2,3
		
		% test RESUME_CONT with Trap
		SETH	$0,#8000
		ORMH	$0,ADDR9>>32
		ORML	$0,ADDR9>>16
		ORL		$0,ADDR9>>0
		PUT		rT,$0
		TRAP	1,2,3
		SET		$19,1
		
		TRAP	0
