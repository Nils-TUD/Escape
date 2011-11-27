		LOC		#1000
		
Main	SETML	$3,#0010
x		OR		$0,$0,$0
		OR		$0,$0,$0
		SETH	$1,#0000
		LDOU	$0,$1,0
		STOU	$0,$1,0
		ADD		$2,$2,1
		CMP		$4,$2,$3
		PBNZ	$4,x
		TRAP	0
