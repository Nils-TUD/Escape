%
% stunc-syncd-bug.mms -- MMIX-PIPE does not flush data to memory, but only to secondary-cache
%

		LOC		#1000

Main	SETL	$0,#5000
		SETL	$1,#1234
		STUNC	$1,$0,0			% note: it works with STOU
		SYNCD	#7,$0,0			% flush #5000..#5007
		LDOU	$2,$0,0			% will be #1234
FOO		JMP		FOO				% no matter how long we wait, m5000 is always 0; the data is only
								% present in secondary cache
								% this seems only to be the case if memreadtime >= 3 in the config
		TRAP	0
