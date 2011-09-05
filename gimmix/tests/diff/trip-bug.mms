%
% trip-bug.mms -- tests the bug in MMIX-PIPE that bits that have not been triped don't appear in rA
%
		
		% float overflow
		LOC		#50
		GET		$1,rA			% should be #FF01 because float inexact has not been triped
								% is actually #FF00 on MMIX-PIPE
		RESUME	0

		LOC		#1000
		

		% enable trips		
Main	SETL	$0,#FF00
		PUT		rA,$0

		% test float overflow
		SETH	$0,#7FEF
		ORMH	$0,#FFFF
		ORML	$0,#FFFF
		ORL		$0,#FFFF
		FADD	$0,$0,$0
		
		TRAP	0
