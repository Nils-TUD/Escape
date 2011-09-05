%
% cswap-ex.mms -- tests exceptions the CSWAP instruction may raise
%


% Note that this test is in diff because of the bug in the CSWAP-implementation of MMMIX


		% segmentsizes: 1,0,0,0; pageSize=2^13; r=0x90000; n=0
		LOC		#8000
RV		OCTA	#10000D0000090000

		LOC		#00090008
		OCTA	#0000000000002004	% PTE  1    (#0000000000002000 .. #0000000000003FFF)
		LOC		#00090030
		OCTA	#000000000000C006	% PTE  6    (#000000000000C000 .. #000000000000DFFF)


		% stack for unsave
		LOC		#C000
		OCTA	#0							% rL
		OCTA	#0							% $248
		OCTA	#0							% $249
		OCTA	#0							% $250
		OCTA	#0							% $251
		OCTA	#0							% $252
		OCTA	#0							% $253
		OCTA	#0							% $254
		OCTA	#0							% $255
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
STACK	OCTA	#F800000000000000			% rG | rA


		LOC		#2000
DATA	OCTA	#89


		% dynamic trap address
		LOC		#4000
DTRAP	SETH	$248,#8000
		ORML	$248,#0009
		% determine address of PTE by dividing pagefault-address by pagesize/8
		GET		$249,rYY
		SET		$250,#2000>>3
		DIVU	$249,$249,$250
		ADDU	$248,$248,$249
		% set writeable-flag
		LDOU	$249,$248,0
		OR		$249,$249,#2
		STOU	$249,$248,0
		% update DTC
		GET		$249,rYY
		OR		$249,$249,#6
		LDVTS	$249,$249,0
		% reset rQ
		GET		$249,rQ
		PUT		rQ,0
		SETMH	$255,#00FE				% set rK
		RESUME	1


		LOC		#1000
		% setup paging
Main	SETH	$0,#8000
		ORL		$0,RV
		LDOU	$0,$0,0
		PUT		rV,$0
		
		% setup stack
		SET		$0,STACK
		UNSAVE	0,$0
		
		% setup rTT
		SETH	$0,#8000
		ORL		$0,DTRAP
		PUT		rTT,$0

		% enable exceptions
		SETMH	$0,#00FE
		PUT		rK,$0

		% setup rP and memory-address
		PUT		rP,#88
		SET		$0,DATA
		
		% --- store causes exception ---
		% first try, not successfull
		SET		$2,#2233
		CSWAP	$2,$0,0
		GET		$3,rP
		
		% second try, successfull
		SET		$4,#4455
		CSWAP	$4,$0,0
		GET		$5,rP
		
		
		% --- reg-set causes exception ---
		% make stack not-writable
		SET		$1,#C004
		LDVTS	$1,$1,0
		SET		$1,#4455
		PUT		rP,$1
		SET		$247,1
		PUSHJ	$248,F1
		

		% --- reg-set AND store causes exception ---
		% make stack not-writable
		SET		$1,#C004
		LDVTS	$1,$1,0
		% make data not-writable
		SET		$1,#2004
		LDVTS	$1,$1,0
		PUT		rP,0
		SET		$247,1
		PUSHJ	$248,F2


		SYNCD	#20,$0
		TRAP	0
		
		% should succeed
F1		SET		$0,DATA
		CSWAP	$6,$0,0					% won't trigger an exception on MMMIX because of the bug
		GET		$1,rP					% that rL is not increased and so on
		STOU	$1,$0,8
		STOU	$6,$0,16
		POP		0,0

		% should succeed
F2		SET		$0,DATA
		CSWAP	$6,$0,0
		GET		$1,rP
		STOU	$1,$0,24
		STOU	$6,$0,32
		POP		0,0
		