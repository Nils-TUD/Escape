%
% cswap.mms -- tests a usage of CSWAP
%

CMPLOC	IS		#8000

		LOC		#0
WAIT	BYTE	"Waiting...",0
UNLCK	BYTE	"Unlocked!",#d,#a,0

		LOC		#F000

DTRAP	GET		$10,rQ
		PUT		rQ,0
		SET		$10,CMPLOC
		ORH		$10,#8000
		STCO	0,$10,0				% unlock
		NEG		$255,0,1
		ANDNMH	$255,#0001			% set interrupt-mask
		RESUME	1
		
		
		LOC		#1000

Main	SETH	$0,#8000
		ORMH	$0,DTRAP>>32
		ORML	$0,DTRAP>>16
		ORL		$0,DTRAP>>0
		PUT		rTT,$0
		
		% enable interrupts
		NEG		$0,0,1
		ANDNMH	$0,#0001
		PUT		rK,$0
		
		% configure timer
		SETH	$0,#8001
		SET		$1,#C00
		STOU	$1,$0,8				% timer-divisor = #C00
		SET		$1,#2
		STOU	$1,$0,0				% enable timer-interrupts
		
		SET		$10,CMPLOC
		ORH		$10,#8000
		STCO	1,$10,0				% lock
		
reset	SET		$1,CMPLOC			% set location
		ORH		$1,#8000
		
		SETL	$3,0
		SETL	$4,WAIT
		ORH		$4,#8000
		PUSHJ	$2,io:puts			% io:puts(0,"Waiting...")
		
loop	SET		$0,#1				% set $0 and rP
		PUT		rP,0
		CSWAP	$0,$1,0				% compare and swap
		BZ		$0,loop				% if its 0, its still locked
		
		SETL	$3,0
		SETL	$4,UNLCK
		ORH		$4,#8000
		PUSHJ	$2,io:puts			% io:puts(0,"Unlocked\r\n")
		JMP		reset

#include "io.mmi"
