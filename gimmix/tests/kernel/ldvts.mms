%
% ldvts.mms -- tests the ldvts instruction
%

		LOC		#00000000
		% segmentsizes: 1,0,0,0; pageSize=2^13; r=0x2000; n=16
RV		OCTA	#10000D0000002010

		LOC		#00002000
		OCTA	#0000000000200017	% PTE  0    (#0000000000000000 .. #0000000000001FFF)
		LOC		#00002008
		OCTA	#0000000000202017	% PTE  1    (#0000000000002000 .. #0000000000003FFF)
		
	
		LOC		#1000

Main	SETH	$0,#8000
		ORMH	$0,RV>>32
		ORML	$0,RV>>16
		ORL		$0,RV
		LDOU	$0,$0,0
		PUT		rV,$0
		
		SETML	$0,#1000
		PUT		rT,$0				% for MMIX-PIPE, because otherwise it will pre-read from 0 when
									% the quit-trap is used, which lets the LDVTS for 0 load 1
									% instead of the expected 0
		
		% put #0000 into DTC
		SET		$1,#0
		LDOU	$0,$1,0
		% put #2000 into DTC
		SET		$1,#2000
		LDOU	$0,$1,0

		% replace permission bits with 0x7
		SET		$0,#0017
		LDVTS	$4,$0,0
		SYNCD	#7,$1,0				% its required to do a sync first (at least it might be)
		LDOU	$0,$1,0				% load should still work
		STOU	$0,$1,0				% store should still work
		GET		$5,rQ
		
		% replace permission bits with 0x1
		SET		$0,#0011
		LDVTS	$6,$0,0
		SET		$1,#0000
		SYNCD	#7,$0,0
		LDOU	$0,$1,0				% load should fail
		STOU	$0,$1,0				% store should fail
		GET		$7,rQ
		
		% remove from DTC
		SET		$0,#2010
		LDVTS	$8,$0,0				% remove it
		LDVTS	$9,$0,0				% should yield 0
		
		% not present in DTC
		SET		$0,#4010
		LDVTS	$10,$0,0			% should yield 0
		
		% put #0000 and #2000 into DTC
		SET		$1,#0
		LDOU	$0,$1,0
		SET		$1,#2000
		LDOU	$0,$1,0
		
		SET		$0,#0017
		LDVTS	$11,$0,0			% should yield 2
		SET		$0,#2017
		LDVTS	$12,$0,0			% should yield 2
		
		% remove all entries in TCs
		SYNC	6
		
		SET		$0,#0010
		LDVTS	$13,$0,0			% should yield 0
		SET		$0,#2010
		LDVTS	$14,$0,0			% should yield 0
		
		TRAP	0
