%
% resumetrans.mms -- tests RESUME_TRANS with the software-translation knuth lists in mmix-doc.pdf
%

		LOC		#00000000
		% segmentsizes: 1,0,0,0; pageSize=2^13; r=0x4000; n=16, f=1
RV		OCTA	#10000D0000004011

		LOC		#00004000
		OCTA	#0000000000000017	% PTE  0    (#0000000000000000 .. #0000000000001FFF)
		LOC		#00004008
		OCTA	#0000000000002017	% PTE  1    (#0000000000002000 .. #0000000000003FFF)
		
		
		% stack for unsave
		LOC		#600000
		OCTA	#0							% rL
		OCTA	#0							% $249 = f9
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
STACK	OCTA	#F900000000000000			% rG | rA


saveLoc	IS		$249
mask	IS		$250
s		IS		$251
virt	IS		$252
base	IS		$253
limit	IS		$254


		% forced trap address
		LOC		#700000
ATRAP	GET		virt,rYY
		GET		saveLoc,rXX
		SLU		saveLoc,saveLoc,32
		SRU		saveLoc,saveLoc,32
		CMPU	saveLoc,saveLoc,#01
		BNZ		saveLoc,1F			% no trap 0,0,1?
		SYNC	6
		SYNC	0
		JMP		Res
1H		SRU		saveLoc,virt,63
		BZ		saveLoc,1F			% MSB set?
		GET		virt,rYY
		LDOU	$0,virt,0			% load from that address
		TRAP	0					% quit
		
1H		SAVE	saveLoc,0			% save state first
		GET		$7,rV				% $7=(virtual translation register)
		SRU		$1,virt,61			% $1=i (segment number of virtual address)
		SLU		$1,$1,2
		NEG		$1,52,$1			% $1=52-4i
		SRU		$1,$7,$1			% $1=b[i]<<12
		SLU		$2,$1,4				% $2=b[i+1]<<12
		SETL	$0,#f000
		AND		$1,$1,$0			% $1=(b[i]<<12) & #f000
		AND		$2,$2,$0			% $2=(b[i+1]<<12) & #f000
		SLU		$3,$7,24
		SRU		$3,$3,37
		SLU		$3,$3,13			% $3=(r field of rV)
		ORH		$3,#8000			% make $3 a physical address
		2ADDU	base,$1,$3			% base=address of first page table
		2ADDU	limit,$2,$3			% limit=address after last page table
		SRU		s,$7,40
		AND		s,s,#FF				% s=(s field of rV)
		CMP		$0,s,13
		BN		$0,Fail				% s must be 13 or more
		CMP		$0,s,49
		BNN		$0,Fail				% s must be 48 or less
		SETH	mask,#8000
		ORL		mask,#1ff8			% mask=(sign bit and n field)
		ORH		$7,#8000			% set sign bit for PTP validation below
		ANDNH	virt,#e000			% zero out the segment number
		SRU		$0,virt,s			% $0=a4a3a2a1a0 (page number of virt)
		ZSZ		$1,$0,1				% $1=[page number is zero]
		ADD 	limit,limit,$1		% increase limit if page number is zero
		SETL	$6,#3ff
		
		% The next part of the routine finds the "digits" of the page number (a4 a3 a2 a1 a0 )1024,
		% from right to left:
		CMP		$5,base,limit		% check if page out of segment-range ($5 is used at 1F)
		SRU		$1,$0,10			% $1 /= 1024
		PBZ		$1,1F				% if thats zero, its inside the first PT (in root-location)
		AND		$0,$0,$6			% extract page-number in that PT
		INCL	base,#2000			% to next table in root-location
		
		CMP		$5,base,limit
		SRU		$2,$1,10
		PBZ		$2,2F				% if thats zero, its inside the second PT
		AND		$1,$1,$6
		INCL	base,#2000
		
		CMP		$5,base,limit
		SRU		$3,$2,10
		PBZ		$3,3F				% if thats zero, its inside the third PT
		AND		$2,$2,$6
		INCL	base,#2000
		
		CMP		$5,base,limit
		SRU		$4,$3,10
		PBZ		$4,4F				% if thats zero, its inside the fourth PT
		AND		$3,$3,$6
		INCL	base,#2000
		
		% Then the process cascades back through PTPs.
		CMP		$5,base,limit
		BNN		$5,Fail
		8ADDU	$6,$4,base			% build physical address for PTP (base + pageNo*8)
		LDO		base,$6,0
		XOR		$6,base,$7			% get difference to rV
		AND		$6,$6,mask			% just test sign-bit and n
		BNZ		$6,Fail				% if thats not zero, i.e. different, its an error
		
		ANDNL	base,#1fff			% clear 13 lowest bits of base (PTP)
4H		BNN		$5,Fail
		8ADDU	$6,$3,base
		LDO		base,$6,0
		XOR		$6,base,$7
		AND		$6,$6,mask
		BNZ		$6,Fail
		
		ANDNL	base,#1fff
3H		BNN		$5,Fail
		8ADDU	$6,$2,base
		LDO		base,$6,0
		XOR		$6,base,$7
		AND		$6,$6,mask
		BNZ		$6,Fail
		
		ANDNL	base,#1fff
2H		BNN		$5,Fail
		8ADDU	$6,$1,base
		LDO		base,$6,0
		XOR		$6,base,$7
		AND		$6,$6,mask
		BNZ		$6,Fail
		
		% Finally we obtain the PTE and communicate it to the machine. If errors have been
		% detected, we set the translation to zero; actually any translation with permission bits
		% zero would have the same effect.
		ANDNL	base,#1fff			% remove low 13 bits of PTP
1H		BNN		$5,Fail
		8ADDU	$6,$0,base
		LDO		base,$6,0			% base=PTE
		XOR		$6,base,$7			% get difference
		ANDN	$6,$6,#7			% zero last 3 bits (permissions / f in rV)
		SLU		$6,$6,51			% extract n
		PBZ		$6,Ready			% branch if n matches
Fail	SETL	base,0
Ready	PUT		rZZ,base
		UNSAVE	saveLoc				% restore state
Res		SETL	$255,#000F
		ORMH	$255,#00FF			% set rK
		RESUME	1					% now the machine will digest the translation
		
		
		LOC		#1000

		% setup rV
Main	SETH	$0,#8000
		ORMH	$0,RV>>32
		ORML	$0,RV>>16
		ORL		$0,RV
		LDOU	$0,$0,0
		PUT		rV,$0
		
		% setup stack
		SETH	$0,#8000
		ORMH	$0,STACK>>32
		ORML	$0,STACK>>16
		ORL		$0,STACK>>0
		UNSAVE	$0

		% setup rT
		SETH	$0,#8000
		ORMH	$0,ATRAP>>32
		ORML	$0,ATRAP>>16
		ORL		$0,ATRAP>>0
		PUT		rT,$0
		
		% set rK to trigger interrupt
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
		
		% this instruction will cause a page-fault since #2000 is not in the ITC
		SET		$0,#1234
		SET		$1,#20F8
		% the store will cause another page-fault since #2000 is not in the DTC
		STOU	$0,$1,0
		% clear itc/dtc
		TRAP	0,0,1
		% sync to memory (the instruction will cause a page-fault again)
		SET		$1,#20F8
		% this will also cause a page-fault for #2000 again
		SYNCD	#FF,$1,0
		
		% pass the physical address where we wrote to to the trap-handler
		SETL	$1,#20F8
		ORH		$1,#8000
		
		TRAP	1,1,1
