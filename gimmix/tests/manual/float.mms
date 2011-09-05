%
% float.mms -- some fun with floats
%

		% stack for unsave
		LOC		#600000
		OCTA	#0							% rL
		OCTA	#0							% $255 = ff
		OCTA	#0							% rB
		OCTA	#0							% rD
		OCTA	#0							% rE
		OCTA	#0							% rH
		OCTA	#0							% rJ
		OCTA	#0							% rM
		OCTA	#0							% rP
		OCTA	#0							% rR
		OCTA	#0							% rW
		OCTA	#0							% rX
		OCTA	#0							% rY
		OCTA	#0							% rZ
ADDR	OCTA	#FF00000000000000			% rG | rA

		LOC		#1000
		JMP		Main

WELC	BYTE	"Please enter a float:",#d,#a,0
MSG		BYTE	"> ",0
SQRT	BYTE	"sqrt(",0
RBR		BYTE	")=",0
POW1	BYTE	"pow(",0
POW2	BYTE	",2)=",0
CRLF	BYTE	#d,#a,0
BUF		BYTE	0
		LOC		@+31

		
		% set the stackpointer her because when printing floats the recursion may be quite deep
Main	SETH	$0,#8000
		ORML	$0,#6000
		ORL		$0,ADDR
		UNSAVE	$0

		SET		$1,0
		SET		$2,WELC
		ORH		$2,#8000
		PUSHJ	$0,io:puts
		
loop	SET		$1,MSG
		PUSHJ	$0,putmsg
		
		SET		$1,0
		SET		$2,BUF
		ORH		$2,#8000
		SET		$3,30
		PUSHJ	$0,io:gets
		
		SET		$1,BUF
		ORH		$1,#8000
		PUSHJ	$0,str:tofloat
		
		SET		$2,SQRT
		PUSHJ	$1,putmsg
		
		SET		$2,0
		SET		$3,$0
		PUSHJ	$1,io:putf
		
		SET		$2,RBR
		PUSHJ	$1,putmsg
		
		SET		$2,0
		FSQRT	$3,$0
		PUSHJ	$1,io:putf
		
		SET		$2,CRLF
		PUSHJ	$1,putmsg
		
		SET		$2,POW1
		PUSHJ	$1,putmsg
		
		SET		$2,0
		SET		$3,$0
		PUSHJ	$1,io:putf
		
		SET		$2,POW2
		PUSHJ	$1,putmsg
		
		SET		$2,0
		FMUL	$3,$0,$0
		PUSHJ	$1,io:putf
		
		SET		$2,CRLF
		PUSHJ	$1,putmsg
		
		JMP		loop

% void putmsg(char *msg)
putmsg	GET		$1,rJ
		SET		$3,0
		SET		$4,$0
		ORH		$4,#8000
		PUSHJ	$2,io:puts
		PUT		rJ,$1
		POP		0,0

#include "string.mmi"
#include "io.mmi"
