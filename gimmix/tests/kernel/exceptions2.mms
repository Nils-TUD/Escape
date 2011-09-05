%
% exceptions2.mms -- test exceptions in kernel-mode
%

		% segmentsizes: 3,3,3,4; pageSize=2^13; r=0x90000; n=256
		LOC		#8000
RV		OCTA	#33340D0000090800
		% segmentsizes: 0,0,0,0; pageSize=2^1; r=0x90000; n=256
RVI		OCTA	#0000010000090800
		% segmentsizes: 0,0,0,0; pageSize=2^13; r=0x90000; n=256; f=2
RVI2	OCTA	#00000D0000090802

		% -- PTEs to be able to execute the code --
		LOC		#00090000
		OCTA	#0000000000000801	% PTE  0    (#0000000000000000 .. #0000000000001FFF)
		LOC		#00090008
		OCTA	#0000000000002801	% PTE  1    (#0000000000002000 .. #0000000000003FFF)
		LOC		#00090010
		OCTA	#0000000000004801	% PTE  2    (#0000000000004000 .. #0000000000005FFF)
		% -- PTEs to be able to access the stack --
		LOC		#00096000
		OCTA	#0000000000600807	% PTE  0    (#6000000000000000 .. #6000000000001FFF)
		
		% -- PTPs and PTEs for testing exceptions --
		% n does not match in PTP1
		LOC		#00092008
		OCTA	#8000000000400400	% PTP1 0    (#0000000000800000 .. #0000000000FFFFFF)
		LOC		#00400000
		OCTA	#0000000000202807	% PTE  0    (#0000000000800000 .. #0000000000801FFF)
		% n does not match in PTE
		LOC		#00092010
		OCTA	#8000000000402800	% PTP1 1    (#0000000001000000 .. #00000000017FFFFF)
		LOC		#00402000
		OCTA	#0000000000204407	% PTE  0    (#0000000001000000 .. #0000000001001FFF)
		% PTE not readable
		LOC		#00090018
		OCTA	#0000000000206803	% PTE  3    (#0000000000006000 .. #0000000000007FFF)
		% PTE not writable
		LOC		#00090020
		OCTA	#0000000000208805	% PTE  4    (#0000000000008000 .. #0000000000009FFF)
		
		
		% stack for unsave
		LOC		#600000
		OCTA	#0							% rL
		OCTA	#0							% $250 = fa
		OCTA	#0							% $251 = fb
		OCTA	#0							% $252 = fc
		OCTA	#0							% $253 = fd
		OCTA	#0							% $254 = fe
		OCTA	#0							% $255 = ff
		OCTA	#0							% rB
		OCTA	#0							% rD
		OCTA	#0							% rE
		OCTA	#0							% rH
		OCTA	#0							% rJ
		OCTA	#0							% rM
		OCTA	#0							% rP
		OCTA	#0							% rR
		OCTA	#0							% rW
		OCTA	#0							% rX
		OCTA	#0							% rY
		OCTA	#0							% rZ
ADDR	OCTA	#FA00000000000000			% rG | rA
		
		
		LOC		#1000
		
		% first setup basic paging: 0 mapped to 0
Main	SETL	$0,RV
		ORH		$0,#8000
		LDOU	$0,$0
		PUT		rV,$0
		
		% setup our environment
		SETH	$0,#6000
		ORL		$0,ADDR
		UNSAVE	$0
		
		
		% 1. use a special-register >= 32 with put
		PUT		33,#0
		GET		$4,rQ
		PUT		rQ,0
		
		% 2. write to readonly registers
		PUT		rC,#0
		GET		$5,rQ
		PUT		rQ,0
		
		PUT		rN,#0
		GET		$6,rQ
		PUT		rQ,0
		
		PUT		rO,#0
		GET		$7,rQ
		PUT		rQ,0
		
		PUT		rS,#0
		GET		$8,rQ
		PUT		rQ,0
		
		% 3. use a special-register >= 32 with get
		GET		$9,33
		GET		$10,rQ
		PUT		rQ,0
		
		% 4. put illegal value in rA
		SETML	$0,#0004
		PUT		rA,$0
		GET		$11,rQ
		PUT		rQ,0
		
		SETH	$0,#FFFF
		PUT		rA,$0
		GET		$12,rQ
		PUT		rQ,0
		
		% 5. put illegal value in rG
		PUT		rG,0
		GET		$13,rQ
		PUT		rQ,0
		
		PUT		rG,31
		GET		$14,rQ
		PUT		rQ,0
		
		PUT		rG,256
		GET		$15,rQ
		PUT		rQ,0
		
		PUT		rG,500
		GET		$16,rQ
		PUT		rQ,0
		
		% 6. save with a wrong target-register
		SAVE	$0,0		% illegal because $0 < rG
		GET		$18,rQ
		PUT		rQ,0
		
		SAVE	$1,0		% illegal because $1 < rG
		GET		$19,rQ
		PUT		rQ,0
		
		SAVE	$249,0		% illegal because $249 < rG
		GET		$20,rQ
		PUT		rQ,0
		
		% 7. access page where n does not match in PTP1
		SETML	$0,#0080
		ORL		$0,#0000
		SET		$22,1
		LDOU	$22,$0,0		% #800000
		GET		$23,rQ
		PUT		rQ,0
		
		% 8. access page where n does not match in PTE
		SETML	$0,#0100
		ORL		$0,#0000
		SET		$24,1
		LDOU	$24,$0,0		% #1000000
		GET		$25,rQ
		PUT		rQ,0
		
		% 9. load from writeonly page
		SETL	$0,#6000
		SET		$26,1
		LDOU	$26,$0,0		% #6000
		GET		$27,rQ
		PUT		rQ,0
		
		% 10. write to readonly page
		SETL	$0,#8000
		STOU	$0,$0,0			% #8000
		SETH	$0,#8000
		ORML	$0,#0020
		ORL		$0,#8000
		LDOU	$28,$0,0
		GET		$29,rQ
		PUT		rQ,0
		
		% 11. use an invalid sync-command
		SYNC	8
		GET		$30,rQ
		PUT		rQ,0
		
		% 12. use an invalid RESUME-code
		RESUME	2
		GET		$31,rQ
		PUT		rQ,0
		
		% 13. use an invalid rV (s invalid)
		SETL	$0,RVI
		ORH		$0,#8000
		LDOU	$0,$0
		PUT		rV,$0
		
		SET		$0,0
		LDOU	$0,$0,0			% will raise a r-exception, because rV is invalid
		GET		$32,rQ
		PUT		rQ,0
		
		SET		$2,0
		STOU	$2,$2,0			% will raise a w-exception, because rV is invalid
		GET		$33,rQ
		PUT		rQ,0
		
		% 13. use an invalid rV (f invalid)
		SETL	$0,RVI2
		ORH		$0,#8000
		LDOU	$0,$0
		PUT		rV,$0
		
		SET		$0,0
		LDOU	$0,$0,0			% will raise a r-exception, because rV is invalid
		GET		$34,rQ
		PUT		rQ,0
		
		SET		$2,0
		STOU	$2,$2,0			% will raise a w-exception, because rV is invalid
		GET		$35,rQ
		PUT		rQ,0
		
		TRAP	0
