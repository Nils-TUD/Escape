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
		OCTA	#0000000000208804	% PTE  4    (#0000000000008000 .. #0000000000009FFF)
		% PTE not writable, not readable
		LOC		#00090028
		OCTA	#000000000020A800	% PTE  5    (#000000000000A000 .. #000000000000BFFF)
		% PTE writable, not readable
		LOC		#00090030
		OCTA	#000000000020C802	% PTE  6    (#000000000000C000 .. #000000000000DFFF)
		
		
		% data for writing to M[#8000], M[#A000], M[#C000]
		LOC		#208000
		OCTA	#123456789ABCDEF
		LOC		#20A000
		OCTA	#123456789ABCDEF
		LOC		#20C000
		OCTA	#123456789ABCDEF
		
		
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
		
		
		% dynamic trap address
		LOC		#800000
		
ATRAP	BZ		$8,_DEF				% if $8 != 0, use $8 as rWW
		ADD		$252,$8,0
		PUT		rWW,$8
		JMP		_DONE
_DEF	GET		$252,rWW			% get rWW - 4
_DONE	SUBU	$252,$252,4
		GET		$253,rQ				% or rQ in
		OR		$253,$253,$252
		STOU	$253,$254,0			% store it
		GET		$251,rXX
		ANDNH	$251,#8000			% zero MSB (may be different, if mmmix continues the instr)
		SRU		$251,$251,32		% use the upper 32 bit
		STOU	$251,$254,8			% store it
		GET		$251,rYY
		STOU	$251,$254,16		% store rYY
		GET		$251,rZZ
		STOU	$251,$254,24		% store rZZ
		ADD		$254,$254,32
		PUT		rQ,0				% reset rQ
		BZ		$250,_END			% if $250 != 0, set sign-bit in rXX to skip instruction
		GET		$253,rXX
		ORH		$253,#8000
		PUT		rXX,$253
_END	SETL	$255,#000F
		ORMH	$255,#00FF			% set rK
		RESUME	1


		% forced trap address
		LOC		#700000

QUIT	SETH	$0,#8000
		ORL		$0,#4000
		SYNCD	#FF,$0,0
		ADDU	$0,$0,#FF
		SYNCD	#FF,$0,0
		ADDU	$0,$0,#FF
		SYNCD	#FF,$0,0
		ADDU	$0,$0,#FF
		SYNCD	#FF,$0,0
		ADDU	$0,$0,#FF
		SYNCD	#FF,$0,0
		TRAP	0
		
		
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
		
		% global registers are better here because of PUSH/POP
		PUT		rG,128
		% setup rTT
		SETH	$0,#8000
		ORMH	$0,ATRAP>>32
		ORML	$0,ATRAP>>16
		ORL		$0,ATRAP>>0
		PUT		rTT,$0
		% setup rT
		SETH	$0,#8000
		ORMH	$0,QUIT>>32
		ORML	$0,QUIT>>16
		ORL		$0,QUIT>>0
		PUT		rT,$0
		% setup address for results
		SETH	$254,#8000
		ORL		$254,#4000
		% set rK
		SETMH	$0,#00FE
		PUT		rK,$0
		
		% now go to user-mode (we are at #8000000000001000 atm)
		SETL	$0,#000F
		ORMH	$0,#00FF		% have to be set in usermode
		SET		$255,$0
		SET		$0,#2000
		PUT		rWW,$0
		SETH	$0,#8000
		PUT		rXX,$0
		RESUME	1
		
		LOC		#2000
		
		% go from non-negative to negative address
		SETH	$0,#8000
		% special case: the pc is set BEFORE the exception occurs. and $X is set as well.
		% therefore we set $8 to use that as rWW
		GO		$8,$0,0
		
		% reset $8
		SET		$8,0
		
		% use a special-register >= 32 with put
		PUT		33,#4
		
		% write to readonly registers
		PUT		rC,#4
		PUT		rN,#4
		PUT		rO,#4
		PUT		rS,#4
		
		% write to privileged registers
		PUT		rI,#4
		PUT		rT,#4
		PUT		rTT,#4
		PUT		rK,#4
		PUT		rQ,#4
		PUT		rU,#4
		PUT		rV,#4
		
		% save with a wrong target-register
		SAVE	$0,0		% illegal because $0 < rG
		SAVE	$1,0		% illegal because $1 < rG
		SAVE	$127,0		% illegal because $127 < rG
		
		SET		$250,1		% set sign-bit in rXX to skip instruction
		
		% access page where n does not match in PTP1
		SETML	$0,#0080
		ORL		$0,#0000
		LDOU	$0,$0,8		% #800008
		
		% access page where n does not match in PTE
		SETML	$0,#0100
		ORL		$0,#0000
		LDOU	$0,$0,0		% #1000000
		
		% write to not-readable, writable page (wyde)
		SETL	$0,#C001
		STWU	$1,$0,0
		
		% write to page with invalid n (wyde)
		SETML	$0,#0100
		ORL		$0,#0000
		STWU	$1,$0,0
		
		SET		$250,0		% no longer necessary
		
		% jump to a privileged address via resume
		SETH	$0,#8000
		GETA	$1,@+16
		OR		$0,$0,$1
		PUT		rW,$0
		RESUME	0
		PUT		rW,0
		
		% use the privileged RESUME_TRANS
		SETH	$0,#0300
		PUT		rX,$0
		GETA	$0,@+12
		PUT		rW,$0
		RESUME	0
		
		% use an invalid ropcode
		SETH	$0,#0400
		PUT		rX,$0
		GETA	$0,@+12
		PUT		rW,$0
		RESUME	0
		
		% try to increase rL via RESUME_SET
		SETH	$0,#0200
		ORML	$0,#0020
		PUT		rX,$0
		GETA	$0,@+12
		PUT		rW,$0
		RESUME	0
		
		% use invalid instruction with RESUME_CONT
		SETH	$0,#0100
		ORML	$0,#4000
		PUT		rX,$0
		GETA	$0,@+12
		PUT		rW,$0
		RESUME	0
		
		% try to increase rL via RESUME_CONT
		SETH	$0,#0100
		ORML	$0,#0020
		PUT		rX,$0
		GETA	$0,@+12
		PUT		rW,$0
		RESUME	0
		
		% try to use RESUME via RESUME_AGAIN
		SETH	$0,#0000
		ORML	$0,#F900
		PUT		rX,$0
		GETA	$0,@+12
		PUT		rW,$0
		RESUME	0
		
		% put invalid bits in rQ
		SETML	$0,#0001
		PUT		rQ,$0
		
		TRAP	1
