%
% system.mms -- tests system instructions
%

		LOC		#4000
		OCTA	#1234567890ABCDEF
		OCTA	#FEDCBA0987654321

		LOC		#1000

Main	SETL	$0,#4000
		SETL	$1,0

		% test LDUNC
		LDUNC	$10,$0,0
		LDUNC	$11,$0,$1
		LDUNC	$12,$0,8
		
		% not really testable; just for decoding and manual step-through
		PRELD	#7,$0,$1
		PRELD	#7,$0,8
		
		SETL	$2,#5000
		
		% test STUNC
		STUNC	$10,$2,$1
		STUNC	$12,$2,8
		SYNCD	#20,$2,0		% necessary for MMIX-PIPE
		
		SETL	$2,#6000		% take care that it affects a different cache-block
		
		% write some data
		STUNC	$10,$2,0
		STUNC	$11,$0,8
		% use prestore to tell MMIX that it can ignore the current content
		PREST	#7,$2,0
		STOU	$12,$2,0
		SYNCD	#7,$2,0
		
		% not really testable; just for decoding and manual step-through
		GETA	$2,@+8
		PREGO	#10,$2,0
		
		% build an instruction, write it to memory, sync data-cache with instr-cache and execute it
		SETL	$3,#0000
		ORML	$3,#220D		% ADDU $13,$0,$0
		GETA	$2,@+12
		STTU	$3,$2,0
		SYNCID	#3,$2,0
		AND		$13,$0,$0		% will be replaced
		
		TRAP	0
