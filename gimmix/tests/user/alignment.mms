%
% alignment.mms -- tests alignment (generated)
%

		LOC		#0000
DATA	OCTA	#1234567890ABCDEF

		LOC		#1000

Main	SETH	$0,0
		
		% test loads
		LDOU	$1,$0,#0
		LDOU	$2,$0,#1
		LDOU	$3,$0,#2
		LDOU	$4,$0,#3
		LDOU	$5,$0,#4
		LDOU	$6,$0,#5
		LDOU	$7,$0,#6
		LDOU	$8,$0,#7
		
		LDTU	$9,$0,#0
		LDTU	$10,$0,#1
		LDTU	$11,$0,#2
		LDTU	$12,$0,#3
		
		LDWU	$13,$0,#0
		LDWU	$14,$0,#1
		
		
		SETL	$15,#3000
		LDOU	$0,$0,0
		
		% test stores
		STOU	$0,$15,#0
		ADDU	$15,$15,8
		STOU	$0,$15,#1
		ADDU	$15,$15,8
		STOU	$0,$15,#2
		ADDU	$15,$15,8
		STOU	$0,$15,#3
		ADDU	$15,$15,8
		STOU	$0,$15,#4
		ADDU	$15,$15,8
		STOU	$0,$15,#5
		ADDU	$15,$15,8
		STOU	$0,$15,#6
		ADDU	$15,$15,8
		STOU	$0,$15,#7
		ADDU	$15,$15,8
		
		STTU	$0,$15,#0
		ADDU	$15,$15,8
		STTU	$0,$15,#1
		ADDU	$15,$15,8
		STTU	$0,$15,#2
		ADDU	$15,$15,8
		STTU	$0,$15,#3
		ADDU	$15,$15,8
		
		STWU	$0,$15,#0
		ADDU	$15,$15,8
		STWU	$0,$15,#1
		ADDU	$15,$15,8
		
		STBU	$0,$15,#0
		ADDU	$15,$15,8
		STBU	$0,$15,#1
		ADDU	$15,$15,8
		STBU	$0,$15,#2
		ADDU	$15,$15,8
		STBU	$0,$15,#3
		ADDU	$15,$15,8
		STBU	$0,$15,#4
		ADDU	$15,$15,8
		STBU	$0,$15,#5
		ADDU	$15,$15,8
		STBU	$0,$15,#6
		ADDU	$15,$15,8
		STBU	$0,$15,#7
		ADDU	$15,$15,8
		
		SETL	$15,#3000
		SYNCD	#FF,$15
		
		TRAP	0
