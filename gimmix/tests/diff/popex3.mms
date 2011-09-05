%
% popex.mms -- test exception during pop; let it fail first, handle the exception but jump to
%				signal-handler first before resuming the pop
%

		% segmentsizes: 3,3,3,4; pageSize=2^13; r=0x90000; n=256
		LOC		#8000
RV		OCTA	#33340D0000090800

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
		LOC		#00096008
		OCTA	#0000000000602807	% PTE  1    (#6000000000002000 .. #6000000000003FFF)
		LOC		#00096010
		OCTA	#0000000000604807	% PTE  2    (#6000000000004000 .. #6000000000005FFF)
		
		
		% stack for unsave
		LOC		#603808
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
		
		
		% dynamic trap address
		LOC		#800000
SREGS	OCTA	0
		OCTA	0
		OCTA	0
		OCTA	0
		OCTA	0
		
DTRAP	SETML	$250,#0009
		ORL		$250,#6008
		ORH		$250,#8000
		SETML	$251,#0060
		ORL		$251,#2807
		STOU	$251,$250,0			% set rwx in PTE
		SYNC	6					% clear TCs
		
		GETA	$250,SREGS
		GET		$251,rWW
		STOU	$251,$250,0			% SREGS[0] = rWW
		GET		$251,rXX
		STOU	$251,$250,8			% SREGS[1] = rXX
		GET		$251,rYY
		STOU	$251,$250,16		% SREGS[2] = rYY
		GET		$251,rZZ
		STOU	$251,$250,24		% SREGS[3] = rZZ
		
		SETL	$250,#2200
		PUT		rWW,$250			% set rWW to address of sighandler-entry
		SETH	$250,#8000
		PUT		rXX,$250			% skip to #2200
		GET		$250,rQ
		PUT		rQ,0				% clear rQ
		NEG		$255,0,1			% set rK
		RESUME	1


		% forced trap address
FTRAP	GET		$251,rXX
		AND		$251,$251,#FF		% Z-field specifies the syscall
		CMPU	$252,$251,1
		BZ		$252,1F				% end-signal?
		
		SETML	$250,#0009
		ORL		$250,#6008
		ORH		$250,#8000
		STCO	0,$250,0			% make PTE invalid
		SYNC	6					% clear TCs
		JMP		2F
		
1H		GETA	$250,SREGS
		LDOU	$251,$250,0
		PUT		rWW,$251			% rWW = SREGS[0]
		LDOU	$251,$250,8
		PUT		rXX,$251			% rXX = SREGS[1]
		LDOU	$251,$250,16
		PUT		rYY,$251			% rYY = SREGS[2]
		LDOU	$251,$250,24
		PUT		rZZ,$251			% rZZ = SREGS[3]
		
2H		NEG		$255,0,1			% set rK
		RESUME	1
		
		
		LOC		#1000
		% first setup basic paging
Main	SETL	$0,RV
		ORH		$0,#8000
		LDOU	$0,$0
		PUT		rV,$0
		
		% setup stack
		SETH	$0,#6000
		ORL		$0,ADDR
		UNSAVE	$0
		
		% setup rTT
		SETH	$0,#8000
		ORMH	$0,DTRAP>>32
		ORML	$0,DTRAP>>16
		ORL		$0,DTRAP>>0
		PUT		rTT,$0
		% setup rT
		SETH	$0,#8000
		ORMH	$0,FTRAP>>32
		ORML	$0,FTRAP>>16
		ORL		$0,FTRAP>>0
		PUT		rT,$0
		
		% now go to user-mode
		SET		$0,#2000
		PUT		rWW,$0
		SETH	$0,#8000
		PUT		rXX,$0
		NEG		$255,0,1			% set rK
		RESUME	1
		
		
		LOC		#2000
		SET		$24,#11
		SET		$25,#22
		SET		$26,#33
		SET		$27,#44
		SET		$244,#55
		SET		$245,#66
		SET		$246,#77
		SET		$247,#88
		SET		$248,#99
		PUSHJ	$249,F1
		
		% we should reach this and $240..$248 should still have the values above
		% $249 should be the return-value of F2, i.e. #2038
		TRAP	0,0,0
		
F1		GET		$0,rJ
		SET		$248,#1234			% cause some stores on the stack
		PUSHJ	$249,F2
		PUT		rJ,$0
		SET		$0,$249
		POP		1,0
		
F2		GET		$30,rJ
		SET		$40,#1234
		PUSHJ	$41,F3
		PUT		rJ,$30				% here rL=3
		SET		$0,$30
		POP		1,0					% this fails somewhere in the middle, but will be resumed
									% after the exception and the signal has been handled.
									% now rL=1, because of one return-value

F3		SET		$35,#1234			% cause more stores
		TRAP	0,0,2				% make a part of the stack unreadable & unwritable
		PUT		rG,36
		POP		0,0

		% signal-handler entry
		LOC		#2200
SIGHE	GET		$250,rJ
		PUSHJ	$250,SIGH
		PUT		rJ,$250
		TRAP	0,0,1				% finish signal-handling; i.e. return to ordinary execution
		
		% the real signal-handler
SIGH	SET		$240,#99			% cause some stores on the stack
		POP		0,0
