%
% wyde.mms -- tests wyde-instructions
%

		LOC		#1000

		% test set
Main	SETH	$2,#1234
		SETMH	$3,#5678
		SETML	$4,#90AB
		SETL	$5,#CDEF
		
		% test inc with overflow
		SETH	$6,#FFFF
		ORMH	$6,#FFFF
		ORML	$6,#FFFF
		ORL		$6,#FFFF
		
		INCH	$6,#1111
		INCMH	$6,#2222
		INCML	$6,#3333
		INCL	$6,#4444
		
		% test inc with normal
		SETH	$7,#0000
		ORMH	$7,#1111
		ORML	$7,#2222
		ORL		$7,#3333
		
		INCH	$7,#1111
		INCMH	$7,#2222
		INCML	$7,#3333
		INCL	$7,#4444
		
		% test or
		ORH		$8,#1234
		ORMH	$8,#5678
		ORML	$8,#90AB
		ORL		$8,#CDEF
		
		ORH		$8,#4321
		ORMH	$8,#8765
		ORML	$8,#BA09
		ORL		$8,#FEDC
		
		% test and-not
		ORH		$9,#1234
		ORMH	$9,#5678
		ORML	$9,#90AB
		ORL		$9,#CDEF
		
		ANDNH	$9,#FF00
		ANDNMH	$9,#0F0F
		ANDNML	$9,#F0F0
		ANDNL	$9,#00FF

		TRAP	0
