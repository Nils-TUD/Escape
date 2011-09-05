%
% interrupts.mms -- tests interrupts (please run it with 4 terminals!)
%

		LOC		#0
WELC	BYTE	"Waiting for interrupts...",#d,#a,0
MSGT	BYTE	"Got timer-interrupt: ",0
MSGR0	BYTE	"Got terminal 0 receiver-interrupt: ",0
MSGR1	BYTE	"Got terminal 1 receiver-interrupt: ",0
MSGR2	BYTE	"Got terminal 2 receiver-interrupt: ",0
MSGR3	BYTE	"Got terminal 3 receiver-interrupt: ",0
MSGI	BYTE	"Got interval-counter-interrupt: ",0
MSGX	BYTE	"Got unknown interrupt: ",0
CRLF	BYTE	#d,#a,0

IRQT	IS		#34
IRQR0	IS		#35
IRQR1	IS		#37
IRQR2	IS		#39
IRQR3	IS		#3B
IRQI	IS		#7

		LOC		#F000

		% put bit-position of interrupt-reason in $129
DTRAP	GET		$10,rQ
		NEG		$11,0,1
		ANDNMH	$11,#0001			% build interrupt-mask (all without privileged-ex)
		AND		$10,$10,$11
		SUBU	$11,$10,1
		SADD	$11,$11,$10			% determine bit-position and put it into $11
		SET		$13,MSGX			% default: unknown
		CMPU	$10,$11,IRQT		% compare to timer
		CSZ		$13,$10,MSGT		% set to timer-msg, if its the timer
		CMPU	$10,$11,IRQR0		% compare to receiver-0
		CSZ		$13,$10,MSGR0		% set to r0-msg, if its the receiver-0
		CMPU	$10,$11,IRQR1		% ...
		CSZ		$13,$10,MSGR1
		CMPU	$10,$11,IRQR2
		CSZ		$13,$10,MSGR2
		CMPU	$10,$11,IRQR3
		CSZ		$13,$10,MSGR3
		CMPU	$10,$11,IRQI
		CSZ		$13,$10,MSGI
		ORH		$13,#8000
		GET		$14,rI				% restart interval-counter, if necessary
		CMP		$15,$14,0
		BNN		$15,1F
		SETML	$15,#0100
		PUT		rI,$15
1H		SET		$12,0
		SET		$10,$11
		PUSHJ	$11,io:puts			% print message
		SET		$12,0
		SET		$13,$10
		PUSHJ	$11,io:putn			% print interrupt-number
		SET		$12,0
		SET		$13,CRLF
		ORH		$13,#8000
		PUSHJ	$11,io:puts			% print \r\n
		% resume
		PUT		rQ,0
		NEG		$255,0,1
		ANDNMH	$255,#0001			% set interrupt-mask
		RESUME	1
		
		
		LOC		#1000

Main	SETH	$0,#8000
		ORMH	$0,DTRAP>>32
		ORML	$0,DTRAP>>16
		ORL		$0,DTRAP>>0
		PUT		rTT,$0
		
		% write welcome-message
		SET		$1,0
		SET		$2,WELC
		ORH		$2,#8000
		PUSHJ	$0,io:puts
		
		% enable interrupts
		NEG		$0,0,1
		ANDNMH	$0,#0001
		PUT		rK,$0
		
		% configure interval-counter
		SETML	$0,#0100
		PUT		rI,$0
		
		% configure timer
		SETH	$0,#8001
		SET		$1,#1000
		STOU	$1,$0,8				% timer-divisor = #1000
		SET		$1,#2
		STOU	$1,$0,0				% enable timer-interrupts
		
		% configure terminals
		SET		$1,#2
		SETH	$0,#8002
		STOU	$1,$0,0				% enable receiver-interrupts (t0)
		INCMH	$0,#0001
		STOU	$1,$0,0				% enable receiver-interrupts (t1)
		INCMH	$0,#0001
		STOU	$1,$0,0				% enable receiver-interrupts (t2)
		INCMH	$0,#0001
		STOU	$1,$0,0				% enable receiver-interrupts (t3)
		
loop	JMP		loop


#include "io.mmi"
