%
% terminal-ie.mms -- testing terminal with interrupts enabled (run it with >= 2 terminals)
%

% terminal 0 rcvr ctrl: #8002000000000000
% terminal 0 rcvr data: #8002000000000008
% terminal 0 xmtr ctrl: #8002000000000010
% terminal 0 xmtr data: #8002000000000018
% terminal 1 rcvr ctrl: #8002000100000000
% terminal 1 rcvr data: #8002000100000008
% terminal 1 xmtr ctrl: #8002000100000010
% terminal 1 xmtr data: #8002000100000018

		LOC		#0
STR1	BYTE	"Hello World Term0!",#d,#a,0
STR2	BYTE	"Hello World Term1!",#d,#a,0
STR3	BYTE	0


		% dynamic traps
		LOC		#600000
DTRAP	GET		$254,rQ
		PUT		rQ,0
		SET		$254,1
		SETH	$255,#FFFF
		ORMH	$255,#FF00
		RESUME	1

		LOC		#1000

Main	PUT		rG,254
		% setup rTT
		SETH	$0,#8000
		ORMH	$0,DTRAP>>32
		ORML	$0,DTRAP>>16
		ORL		$0,DTRAP>>0
		PUT		rTT,$0
		
		% enable interrupts
		NEG		$0,0,1
		ANDNMH	$0,#0001
		PUT		rK,$0
		
		% set IEN-flag in control-registers of terminals
		SETH	$0,#8002			% terminal 0
		LDOU	$1,$0,0
		OR		$1,$1,#2
		STOU	$1,$0,0				% receiver
		LDOU	$1,$0,#10
		OR		$1,$1,#2
		STOU	$1,$0,#10			% transmitter
		ORMH	$0,#0001			% terminal 1
		LDOU	$1,$0,0
		OR		$1,$1,#2
		STOU	$1,$0,0				% receiver
		LDOU	$1,$0,#10
		OR		$1,$1,#2
		STOU	$1,$0,#10			% transmitter
		
		% write STR1 to term0
		SET		$1,0
		SET		$2,STR1
		ORH		$2,#8000
		PUSHJ	$0,puts
		
		% write STR2 to term1
		SET		$1,1
		SET		$2,STR2
		ORH		$2,#8000
		PUSHJ	$0,puts
		
		% read into STR3 from term0
		SET 	$1,0
		SET		$2,STR3
		ORH		$2,#8000
		PUSHJ	$0,gets
		
		% write STR3 to term1
		SET		$1,1
		SET		$2,STR3
		ORH		$2,#8000
		PUSHJ	$0,puts

loop	JMP		loop


% octa gets(octa term,char *dst)
gets	GET		$2,rJ				% save rJ
		ADDU	$3,$1,0
1H		ADDU	$5,$0,0
		PUSHJ	$4,getc				% call getc(term)
		ADDU	$6,$0,0
		ADDU	$7,$4,0
		PUSHJ	$5,putc				% call putc(term,c)
		CMP		$5,$4,#0D			% c == \n?
		BZ		$5,1F				% if so, stop
		STBU	$4,$1,0				% otherwise, store char
		ADDU	$1,$1,1				% to next
		JMP		1B
1H		STBU	$10,$1,0			% null-termination
		SUBU	$0,$1,$3			% determine length
		PUT		rJ,$2				% restore rJ
		POP		1,0					% return length

% octa getc(octa term)
getc	SETH	$1,#8002			% base address: #8002000000000000
		SL		$0,$0,32			% or in terminal-number
		OR		$1,$1,$0			% -> #8002000100000000 for term 1, e.g.
1H		SET		$254,0				% first, reset our flag-register
_gcwait	PBZ		$254,_gcwait		% wait here until the interrupt-handler sets the flag
		LDOU	$2,$1,#0			% read ctrl-reg
		AND		$2,$2,#1			% extract RDY-bit
		BZ		$2,1B				% try again if its not set
		LDOU	$0,$1,#8			% load char
		POP		1,0					% return it

% void puts(octa term,char *string)
puts	GET		$2,rJ				% save rJ
1H		LDBU	$5,$1,0				% load char from string
		BZ		$5,2F				% if its 0, we are done
		ADDU	$4,$0,0
		PUSHJ	$3,putc				% call putc(c)
		ADDU	$1,$1,1				% to next char
		JMP		1B
2H		PUT		rJ,$2				% restore rJ
		POP		0,0

% void putc(octa term,octa character)
putc	SETH	$2,#8002			% base address: #8002000000000000
		SL		$0,$0,32			% or in terminal-number
		OR		$2,$2,$0			% -> #8002000100000000 for term 1, e.g.
		LDOU	$3,$2,#10			% read ctrl-reg
		AND		$3,$3,#1			% extract RDY-bit
		BNZ		$3,1F				% ready?
		SET		$254,0				% first, reset our flag-register
_pcwait	PBZ		$254,_pcwait		% wait here until the interrupt-handler sets the flag
1H		STOU	$1,$2,#18			% write char
		POP		0,0
