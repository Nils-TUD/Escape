%
% exceptions1.mms -- test exceptions in usermode
%

		% segmentsizes: 3,3,3,4; pageSize=2^13; r=0x90000; n=256
		LOC		#8000
RV		OCTA	#33340D0000090800
		
		% -- PTEs to be able to execute the code --
		LOC		#00090000
		OCTA	#0000000000000801	% PTE  0    (#0000000000000000 .. #0000000000001FFF)
		LOC		#00090008
		OCTA	#0000000000002801	% PTE  1    (#0000000000002000 .. #0000000000003FFF)
		
		
		% dynamic trap address
		LOC		#800000
		
ATRAP	GET		$252,rWW			% get rWW - 4
		SUBU	$252,$252,4
		GET		$253,rQ				% or rQ in
		OR		$253,$253,$252
		STOU	$253,$254,0			% store it
		GET		$251,rXX
		ANDNH	$251,#8000			% zero MSB (may be different, if mmmix continues the instr)
		SRU		$251,$251,32		% use the upper 32 bit
		STOU	$251,$254,8			% store it
		BZ		$250,1F
		SET		$251,0
		JMP		2F
1H		GET		$251,rYY
2H		STOU	$251,$254,16		% store rYY
		GET		$251,rZZ
		STOU	$251,$254,24		% store rZZ
		ADD		$254,$254,32
		PUT		rQ,0				% reset rQ
		GET		$253,rXX
		ORH		$253,#8000			% skip instruction
		PUT		rXX,$253
		SETMH	$255,#00FE			% rK
		RESUME	1

		
		LOC		#1000
		
		% first setup basic paging: 0 mapped to 0
Main	SETL	$0,RV
		ORH		$0,#8000
		LDOU	$0,$0
		PUT		rV,$0
		
		% global registers are better here because of PUSH/POP
		PUT		rG,128
		% setup rTT
		SETH	$0,#8000
		ORMH	$0,ATRAP>>32
		ORML	$0,ATRAP>>16
		ORL		$0,ATRAP>>0
		PUT		rTT,$0
		% setup address for results
		SETH	$254,#8000
		ORL		$254,#4000
		
		SETMH	$255,#00FE			% rK
		PUT		rK,$255
		
		GETA	$0,1F
		PUT		rWW,$0
		SETH	$0,#0000			% resume again
		ORML	$0,#8D00
		ORL		$0,#0000			% LDOI $0,$0,0
		PUT		rXX,$0
		SETL	$0,#6000			% #6000
		PUT		rYY,$0
		PUT		rZZ,0
		SETMH	$255,#00FE			% rK
		RESUME	1
		
1H		GETA	$0,2F
		PUT		rWW,$0
		SETH	$0,#0300			% resume trans
		ORML	$0,#8D00
		ORL		$0,#0000			% LDOI $0,$0,0
		PUT		rXX,$0
		SETL	$0,#1000
		PUT		rYY,$0
		SETL	$0,#1807
		PUT		rZZ,$0
		SETL	$0,#6000			% set address for LDOI
		SETMH	$255,#00FE			% rK
		RESUME	1
		
2H		SET		$250,1				% for the following: store 0 for rYY (mmix-pipe and gimmix differ)

		PUT		rL,0
		GETA	$0,3F
		PUT		rWW,$0
		SETH	$0,#0200			% resume set
		ORML	$0,#0004			% SET $4,#12345678 (increases rL)
		PUT		rXX,$0
		SETML	$0,#1234
		ORL		$0,#5678
		PUT		rZZ,$0
		SETL	$0,#6000			% set address for LDOI
		SETMH	$255,#00FE			% rK
		RESUME	1
		
3H		PUT		rL,0
		GETA	$0,4F
		PUT		rWW,$0
		SETH	$0,#0100			% resume continue
		ORML	$0,#2104
		ORL		$0,#0000			% ADDI $4,$0,0
		PUT		rXX,$0
		SETL	$0,#6000
		PUT		rYY,$0				% set address for LDOI
		PUT		rZZ,0
		SETMH	$255,#00FE			% rK
		RESUME	1

4H		SETH	$254,#8000
		ORL		$254,#4000
		SYNCD	#FF,$254,0
		TRAP	0
