%
% system.mms -- tests exception-behaviour of system instructions
%

		LOC		#0000
		% segmentsizes: 1,3,4,9; pageSize=2^13; r=0x20000; n=0
RV		OCTA	#13490D0000020000

		LOC		#20000
		OCTA	#0000000000000007	% PTE  0  (#0000000000000000 .. #0000000000001FFF)
		LOC		#20008
		OCTA	#0000000000002007	% PTE  1  (#0000000000002000 .. #0000000000003FFF)
		LOC		#20010
		OCTA	#0000000000004000	% PTE  2  (#0000000000004000 .. #0000000000005FFF)


		LOC		#1000
Main	SETH	$0,#8000
		LDOU	$0,$0,0
		PUT		rV,$0

		SETL	$0,#2000
		SETL	$1,#4000
		
		% LDUNC: should not set rQ
		LDUNC	$9,$0,0
		GET		$10,rQ
		PUT		rQ,0
		
		% LDUNC: should set rQ
		LDUNC	$9,$1,0
		GET		$11,rQ
		PUT		rQ,0
		
		% STUNC: should not set rQ
		STUNC	$0,$0,0
		GET		$12,rQ
		PUT		rQ,0
		
		% STUNC: should set rQ
		STUNC	$0,$1,0
		GET		$13,rQ
		PUT		rQ,0
		
		% PRELD: should not set rQ
		PRELD	#FF,$0,0
		GET		$14,rQ
		PUT		rQ,0
		
		% PRELD: should not set rQ
		PRELD	#FF,$1,0
		GET		$15,rQ
		PUT		rQ,0
		
		% PREGO: should not set rQ
		PREGO	#FF,$0,0
		GET		$16,rQ
		PUT		rQ,0
		
		% PREGO: should not set rQ
		PREGO	#FF,$1,0
		GET		$17,rQ
		PUT		rQ,0
		
		% PREST: should not set rQ
		PREST	#FF,$0,0
		GET		$18,rQ
		PUT		rQ,0
		
		% PREST: should not set rQ
		PREST	#FF,$1,0
		GET		$19,rQ
		PUT		rQ,0
		
		% SYNCD: should not set rQ
		SYNCD	#FF,$0,0
		GET		$20,rQ
		PUT		rQ,0
		
		% SYNCD: should not set rQ
		SYNCD	#FF,$1,0
		GET		$21,rQ
		PUT		rQ,0
		
		% SYNCID: should not set rQ
		SYNCID	#FF,$0,0
		GET		$22,rQ
		PUT		rQ,0
		
		% SYNCID: should not set rQ
		SYNCID	#FF,$1,0
		GET		$23,rQ
		PUT		rQ,0
		
		% just to execute them once.. (not really testable here)
		SYNC	5		% flush caches to mem
		SYNC	7		% clear IC and DC
		
		TRAP	0
