%
% misc.mms -- tests misc-instructions
%

		LOC		#1000

Main	SETH	$2,#1111
		ORMH	$2,#2222
		ORML	$2,#3333
		ORL		$2,#4444
		
		% test put/get
		PUT		rD,#12
		PUT		rR,#34
		PUT		rH,$2
		GET		$3,rD
		GET		$4,rR
		GET		$5,rH
		
		% prepare compares
		NOR		$6,$6,$6		% = -1
		ADD		$7,$6,0
		ANDNL	$7,#0001		% = -2
		SET		$8,1
		NOR		$9,$9,$9
		ANDNH	$9,#8000		% = 2^63 - 1
		SET		$10,0
		
		% test cmp(i)
		CMP		$11,$10,$10		% 0 :: 0
		CMP		$12,$10,$6		% 0 :: -1
		CMP		$13,$10,$8		% 0 :: 1
		CMP		$14,$6,$7		% -1 :: -2
		CMP		$15,$8,2		% 1 :: 2
		CMP		$16,$9,$9		% 2^63-1 :: 2^63-1
		CMP		$17,$7,$9		% -1 :: 2^63-1
		CMP		$18,$10,$9		% 0 :: 2^63-1
		CMP		$19,$8,$9		% 1 :: 2^63-1
		ANDN	$20,$6,$9		% = -2^63
		CMP		$21,$20,$7		% -2^63 :: -1
		CMP		$22,$20,0		% -2^63 :: 0
		CMP		$23,$20,$9		% -2^63 :: 2^63-1
		
		% test cmpu(i)
		CMPU	$24,$10,$10		% 0 :: 0
		CMPU	$25,$8,2		% 1 :: 2
		CMPU	$26,$10,$6		% 0 :: 2^64-1
		CMPU	$27,$6,$10		% 2^64-1 :: 0
		CMPU	$28,$6,$6		% 2^64-1 :: 2^64-1
		
		% test neg(i)
		NEG		$29,0,1			% 0 - 1
		NEG		$30,1,2			% 1 - 2
		NEG		$31,0,$9		% 0 - 2^63-1
		
		GET		$32,rA			% ensure that we have no arith-exceptions yet
		NEG		$33,0,$20		% 0 - -2^63 -> overflow
		GET		$34,rA
		PUT		rA,0			% clear rA
		
		SUB		$35,$20,1		% = -2^63-1
		NEG		$36,1,$35		% 1 - -2^63-1 -> overflow
		GET		$37,rA
		PUT		rA,0			% clear rA
		
		ADD		$38,$20,1		% = -2^63+1
		NEG		$39,1,$38		% 1 - -2^63+1 -> overflow
		GET		$40,rA
		PUT		rA,0			% clear rA
		
		% test negu(i)
		NEGU	$41,0,1
		NEGU	$42,1,2
		NEGU	$43,0,$9		% 0 - 2^63-1
		NEGU	$44,0,$20		% 0 - -2^63
		NEGU	$45,1,$35		% 1 - -2^63-1
		NEGU	$46,1,$38		% 1 - -2^63+1
		GET		$47,rA
		
		% special case for GET: $49 will contain the value of rL AFTER $49 has been set -> 50
Foo1	GET		$49,rL
		% the same with rS, if it changes
		PUSHJ	$254,Test
		ADD		$50,$254,0
		
		GETA	$51,Foo1
		GETA	$52,Foo2
		
		% special case of PUT: can't increase rL
Foo2	PUT		rL,54
		PUT		rL,55
		GET		$53,rL
		SET		$55,#1234
		PUT		rL,55
		ADD		$54,$55,0

		TRAP	0

Test	GET		$0,rS			% this changes rS and fetches it at the same time -> 60..08
		POP		1,0
