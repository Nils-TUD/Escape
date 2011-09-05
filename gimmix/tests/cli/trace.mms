%
% trace.mms -- tests the trace-command
%

		LOC		#1000

Main	SETH	$0,#1234
		ORMH	$0,#5678
		ORML	$0,#90AB
		ORL		$0,#CDEF
		OR		$1,$0,$0
		SETL	$2,#2000
		LDOU	$1,$2,0
		SWYM
		ORH		$2,#8000
		STOU	$1,$2,0
		SWYM
		TRAP	0,0,0
