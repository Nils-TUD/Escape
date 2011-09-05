%
% resumesec.mms -- test whether resuming to rW=0 from user-mode can execute a instr in priv-mode
%

		% segmentsizes: 3,3,3,4; pageSize=2^13; r=0x90000; n=256
		LOC		#4000
RV		OCTA	#33340D0000090000
		
CHECKS	OCTA	CHECK2
		OCTA	CHECK3
		OCTA	0
		
		LOC		#00090008
		OCTA	#0000000000002001	% PTE  1    (#0000000000002000 .. #0000000000003FFF)
		
		% dynamic trap address
		LOC		#3000
		
ATRAP	GET		$1,rWW
		STOU	$1,$254,0
		GET		$1,rQ
		STOU	$1,$254,8
		GET		$1,rXX
		ANDNH	$1,#FFFF
		ANDNMH	$1,#FF00
		ANDNML	$1,#FFFF
		ANDNL	$1,#FFFF
		STOU	$1,$254,16
		ADDU	$254,$254,24
		PUT		rQ,0
		NEG		$255,0,1
		GETA	$0,CHECKS
		LDOU	$0,$0,$253
		BZ		$0,QUIT
		ADDU	$253,$253,8
		PUT		rWW,$0
		SETH	$0,#8000
		PUT		rXX,$0
		RESUME	1

QUIT	SETH	$254,#8000
		ORL		$254,#5000
		SYNCD	#FF,$254,0
		TRAP	0,0,0

		LOC		#1000
		
		% first setup basic paging: 0 mapped to 0
Main	PUT		rG,253
		SET		$253,0
		SETH	$254,#8000
		ORL		$254,#5000
		SETL	$0,RV
		ORH		$0,#8000
		LDOU	$0,$0
		PUT		rV,$0
		
		% setup rTT
		SETH	$0,#8000
		ORMH	$0,ATRAP>>32
		ORML	$0,ATRAP>>16
		ORL		$0,ATRAP>>0
		PUT		rTT,$0
		
		% to user mode
		SETL	$0,#2000
		PUT		rWW,$0
		SETH	$0,#8000
		PUT		rXX,$0
		NEG		$255,0,1			% rK
		RESUME	1
		
		LOC		#2000
CHECK1	PUT		rW,0
		SETH	$0,#0000			% resume again
		ORML	$0,#8D00
		ORL		$0,#0000			% LDOI $0,$0,0
		PUT		rX,$0
		SETH	$0,#8000			% #8000000000000000
		ORL		$0,#1000
		PUT		rY,$0
		PUT		rZ,0
		RESUME	0
		
CHECK2	PUT		rW,0
		SETH	$0,#0200			% resume set
		ORML	$0,#0000			% set $0
		PUT		rX,$0
		PUT		rY,#12				% to #12
		RESUME	0
		
CHECK3	PUT		rW,0
		SETH	$0,#0100			% resume continue
		ORML	$0,#C100
		ORL		$0,#0000			% OR $0,$0,0
		PUT		rX,$0
		SETH	$0,#8000			% #8000000000000000
		ORL		$0,#1000
		PUT		rY,$0
		PUT		rZ,0
		RESUME	0

		TRAP	0
