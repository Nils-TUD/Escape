%
% resumetrip.mms -- test trips triggered by RESUME in kernel-mode
%

		% segmentsizes: 1,0,0,0; pageSize=2^13; r=0x4000; n=0, f=0
RV		OCTA	#10000D0000004000

		LOC		#00004000
		OCTA	#0000000000000007	% PTE  0    (#0000000000000000 .. #0000000000001FFF)
		OCTA	#0000000000002007	% PTE  1    (#0000000000002000 .. #0000000000003FFF)
		
		
		LOC		#10
		% #10 (div by zero)
		ADD		$5,$5,1
		PUT		rJ,$255
		GET		$255,rB
		RESUME	0


		LOC		#500000
		% forced trap address for RESUME AGAIN
RESAG	SETH	$128,#0000
		ORML	$128,#1D00
		ORL		$128,#0000
		PUT		rXX,$128		% DIVI $0,$0,0
		SETMH	$255,#00FF
		RESUME	1
		
		% forced trap address for the TRAP of RESUME CONTINUE
DUMMY	ADD		$5,$5,1
		SETH	$0,#8000
		ORMH	$0,RESAG>>32
		ORML	$0,RESAG>>16
		ORL		$0,RESAG>>0
		PUT		rT,$0
		SETMH	$255,#00FF
		RESUME	1

		% forced trap address for RESUME CONTINUE
RESCON	SETH	$0,#8000
		ORMH	$0,DUMMY>>32
		ORML	$0,DUMMY>>16
		ORL		$0,DUMMY>>0
		PUT		rT,$0
		SETH	$128,#0100
		ORL		$128,#0001
		PUT		rXX,$128		% TRAP 0,0,1
		SETMH	$255,#00FF
		RESUME	1

		% forced trap address for RESUME SET
RESSET	SETH	$0,#8000
		ORMH	$0,RESCON>>32
		ORML	$0,RESCON>>16
		ORL		$0,RESCON>>0
		PUT		rT,$0
		SETH	$128,#0200
		ORMH	$128,#8000
		ORML	$128,#0004
		PUT		rXX,$128
		SETL	$128,#9988
		PUT		rZZ,$128		% SET $4,#9988
		SETMH	$255,#00FF
		RESUME	1
		
		
		LOC		#1000
		% setup rT
Main	SETH	$0,#8000
		ORMH	$0,RESSET>>32
		ORML	$0,RESSET>>16
		ORL		$0,RESSET>>0
		PUT		rT,$0
	
		% setup rV
		SETH	$0,#8000
		ORMH	$0,RV>>32
		ORML	$0,RV>>16
		ORL		$0,RV
		LDOU	$0,$0,0
		PUT		rV,$0
	
		SETL	$0,#FF00
		PUT		rA,$0
		PUT		rG,128
		
		% now go to user-mode (we are at #8000000000001000 atm)
		SET		$0,#2000
		PUT		rWW,$0
		SETH	$0,#8000
		PUT		rXX,$0
		SETMH	$255,#00FF		% have to be set in usermode
		RESUME	1

		
		LOC		#2000
		SET		$5,0
		
		% test RESUME SET
		TRAP	1,2,3
		GET		$6,rA
		
		% test RESUME CONTINUE (with TRAP)
		TRAP	1,2,3
		GET		$7,rA

		% test RESUME AGAIN		
		TRAP	1,2,3
		GET		$8,rA
		
		TRAP	0
