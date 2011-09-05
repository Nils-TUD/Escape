%
% paging4.mms -- test exception because of invalid rV in kernel-mode
%

		LOC		#8000
		% segmentsizes: 0,0,0,0; pageSize=2^13; r=0x90000; n=0
RV1		OCTA	#00000D0000090000
		% segmentsizes: 3,2,1,0; pageSize=2^13; r=0xA0000; n=0
RV2		OCTA	#32100D00000A0000

		% for RV1
		LOC		#00090000
		OCTA	#0000000000000007	% PTE  0    (#0000000000000000 .. #0000000000001FFF)
		LOC		#00090008
		OCTA	#0000000000000007	% PTE  1    (#0000000000002000 .. #0000000000003FFF)
		
		% for RV2
		LOC		#000A6008
		OCTA	#0000000000000007	% PTE  1024 (#0000080000000000 .. #0000080000001FFF)
		
		LOC		#1000
		
		% set RV1
Main	SETL	$0,RV1
		ORH		$0,#8000
		LDOU	$0,$0
		PUT		rV,$0
		
		SET		$0,#2000
		STOU	$0,$0,0				% #0000000000002000
		SYNC	0					% required because of the mmmix-bug
		GET		$2,rQ
		PUT		rQ,0
		
		% set RV2
		SETL	$0,RV2
		ORH		$0,#8000
		LDOU	$0,$0
		PUT		rV,$0
		
		SETMH	$0,#0800
		STOU	$0,$0,0				% #0000080000000000
		SYNC	0					% required because of the mmmix-bug
		GET		$3,rQ
		PUT		rQ,0
		
		SETH	$0,#2000
		STOU	$0,$0,0				% #2000000000000000
		SYNC	0					% required because of the mmmix-bug
		GET		$4,rQ
		PUT		rQ,0
		
		SETH	$0,#2000
		ORL		$0,#2000
		STOU	$0,$0,0				% #2000000000002000
		SYNC	0					% required because of the mmmix-bug
		GET		$5,rQ
		PUT		rQ,0
		
		SETH	$0,#2000
		ORMH	$0,#FFFF
		ORML	$0,#FFFF
		ORL		$0,#FFFF
		STOU	$0,$0,0				% #2000FFFFFFFFFFFF
		SYNC	0					% required because of the mmmix-bug
		GET		$6,rQ
		PUT		rQ,0
		
		SETH	$0,#4000
		STOU	$0,$0,0				% #4000000000000000
		SYNC	0					% required because of the mmmix-bug
		GET		$7,rQ
		PUT		rQ,0
		
		SETH	$0,#6000
		STOU	$0,$0,0				% #6000000000000000
		SYNC	0					% required because of the mmmix-bug
		GET		$8,rQ
		PUT		rQ,0
		
		TRAP	0
