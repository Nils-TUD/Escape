%
% system-nonexmem.mms -- tests exception-behaviour of system instructions using non-existent mem
%


% note that this test is here because MMIX-PIPE wont fire that exception


		LOC		#0000
		% segmentsizes: 1,3,4,9; pageSize=2^13; r=0x20000; n=0
RV		OCTA	#13490D0000020000

		LOC		#20000
		OCTA	#0000000000000007	% PTE  0  (#0000000000000000 .. #0000000000001FFF)
		LOC		#20008
		OCTA	#0000000000002007	% PTE  1  (#0000000000002000 .. #0000000000003FFF)


		% dynamic trap address
		LOC		#600000
DYNTR	GET		$16,rQ
		PUT		rQ,0
		SETMH	$255,#00FF		% have to be set in usermode
		RESUME	1


		% forced trap address
		LOC		#500000
QUIT	TRAP	0					% quit


		LOC		#1000
Main	SETH	$0,#8000
		LDOU	$0,$0,0
		PUT		rV,$0

		SETH	$0,#8FFF		% not existing memory

		LDUNC	$9,$0,0
		GET		$10,rQ
		PUT		rQ,0

		STUNC	$0,$0,0
		GET		$11,rQ
		PUT		rQ,0

		PRELD	#FF,$0,0
		GET		$12,rQ
		PUT		rQ,0

		PREGO	#FF,$0,0
		GET		$13,rQ
		PUT		rQ,0

		PREST	#FF,$0,0
		GET		$14,rQ
		PUT		rQ,0

		% setup rTT
		SETH	$0,#8000
		ORMH	$0,DYNTR>>32
		ORML	$0,DYNTR>>16
		ORL		$0,DYNTR>>0
		PUT		rTT,$0

		% setup rT
		SETH	$0,#8000
		ORMH	$0,QUIT>>32
		ORML	$0,QUIT>>16
		ORL		$0,QUIT>>0
		PUT		rT,$0

		% now go to user-mode
		SETMH	$255,#00FF		% have to be set in usermode
		SET		$0,#2000
		PUT		rWW,$0
		SETH	$0,#8000
		PUT		rXX,$0
		RESUME	1

		LOC		#2000

		SET		$16,0
		% raises a privileged-access-exception because we try to access the privileged space
		SYNCD	#FF,$0,0
		SET		$15,$16

		% as well
		SET		$16,0
		SYNCID	#FF,$0,0

		TRAP	1
