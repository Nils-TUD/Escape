%
% helloworld.mms -- prints hello world
%

		LOC		#1000
		JMP		Main

MSG		BYTE	"Hello World!",#d,#a,0

Main	SET		$1,0
		SET		$2,MSG
		ORH		$2,#8000
		PUSHJ	$0,puts
		
loop	JMP		loop

% void puts(octa term,char *string)
puts	GET		$2,:rJ				% save rJ
1H		LDBU	$5,$1,0				% load char from string
		BZ		$5,2F				% if its 0, we are done
		ADDU	$4,$0,0
		PUSHJ	$3,putc				% call putc(c)
		ADDU	$1,$1,1				% to next char
		JMP		1B
2H		PUT		:rJ,$2				% restore rJ
		POP		0,0

% void putc(octa term,octa character)
putc	SETH	$2,#8002			% base address: #8002000000000000
		SL		$0,$0,32			% or in terminal-number
		OR		$2,$2,$0			% -> #8002000100000000 for term 1, e.g.
1H		LDOU	$3,$2,#10			% read ctrl-reg
		AND		$3,$3,#1			% exract RDY-bit
		PBZ		$3,1B				% wait until its set
		STOU	$1,$2,#18			% write char
		POP		0,0
