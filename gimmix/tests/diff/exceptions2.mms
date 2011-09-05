%
% exceptions2.mms -- test some GIMMIX specific exceptions in kernel-mode
%

		% segmentsizes: 0,0,0,0; pageSize=2^1; r=0x90000; n=256
		LOC		#8000
RV		OCTA	#0000010000090800

		LOC		#00090000
		OCTA	#0000000000000807	% PTE  0    (#0000000000000000 .. #0000000000001FFF)
		
		LOC		#1000
		
		% these are GIMMIX-specific because of the dynamic-stack-extension-fix
Main	SET		$0,#1000
		PUT		rSS,$0			% will raise a b-exception, because rSS is not in kernel-space
		GET		$1,rQ
		PUT		rQ,0
		
		SAVE	$255,2			% will raise a b-exception, because z > 1
		GET		$2,rQ
		PUT		rQ,0
		
		UNSAVE	2,$255			% will raise a b-exception, because x > 1
		GET		$3,rQ
		PUT		rQ,0
		
		% try to set invalid bits in rQ (implementation specific)
		NEG		$0,0,1
		PUT		rQ,$0
		GET		$4,rQ
		PUT		rQ,0
		
		TRAP	0
