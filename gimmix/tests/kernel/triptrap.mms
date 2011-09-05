%
% triptrap.mms -- test traps in trips
%

		% paging setup: 1MiB pages, 1024 pages in seg0
		% one PTE in root-location that maps 0 to 0
		LOC		#8000
RV		OCTA	#1000140000090000
		LOC		#90000
PTE		OCTA	#0000000000000007
		
		% forced trap addresses
		LOC		#500000
		
		% test some values that are affected by a trap
ATRAP	GET		$134,rXX
		GET		$135,rWW
		GET		$136,rYY
		GET		$137,rZZ
		GET		$138,rBB
		ADD		$139,$255,0
		SETL	$255,#000F
		ORMH	$255,#00FF		% set rK
		RESUME	1

		% forced trip address
		LOC		#0
		
		% test some values that are affected by a trip
ATRIP	GET		$128,rX
		GET		$129,rW
		GET		$130,rY
		GET		$131,rZ
		GET		$132,rB
		ADD		$133,$255,0
		
		% jump to our handler
		PUSHJ	$128,Handler
		PUT		rJ,$255
		GET		$255,rB
		RESUME	0

		% do a trap and test if the values have been restored afterwards
Handler	TRAP	1,2,3
		GET		$141,rJ
		GET		$142,rK
		POP		0
		
		LOC		#1000
		
		
		% first setup basic paging: 0 mapped to 0
Main	SETL	$0,RV
		ORH		$0,#8000
		LDOU	$0,$0
		PUT		rV,$0
		LDOU	$0,$1,0
		% global registers are better here because of PUSH/POP
		PUT		rG,128
		% setup rT
		SETH	$0,#8000
		ORMH	$0,ATRAP>>32
		ORML	$0,ATRAP>>16
		ORL		$0,ATRAP>>0
		PUT		rT,$0
		
		% set some values that will be changed
		SETL	$0,#000F
		ORMH	$0,#00FF		% have to be set in usermode
		SET		$255,$0
		PUT		rK,#0F
		PUT		rJ,#12
		SET		$1,#55
		SET		$2,#66
		SET		$3,#77
		
		% now go to user-mode (we are at #8000000000001000 atm)
		% thats required to get trips working
		SET		$4,#2000
		PUT		rWW,$4
		SETH	$4,#8000
		PUT		rXX,$4
		RESUME	1
		
		LOC		#2000
		
		SET		$255,#1234
		TRIP	1,2,3
		
		% check if they have been restored
		ADD		$143,$255,0
		GET		$144,rJ
		GET		$145,rK
		
		TRAP	0
