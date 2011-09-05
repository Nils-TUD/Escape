%
% string-test.mms -- tests str:compare
%

		LOC		#0

SEP		BYTE	" <> ",0
RES		BYTE	" -> ",0
CRLF	BYTE	#d,#a,0
S1		BYTE	"abc",0
S2		BYTE	"def",0
S3		BYTE	0
S4		BYTE	"a",0
S5		BYTE	"ab",0
S6		BYTE	"ABC",0

STRS	OCTA	S1,S2,S3,S4,S5,S6

		LOC		#1000

strs	IS		$0
i		IS		$1
j		IS		$2

Main	SETL	strs,STRS
		ORH		strs,#8000
		SET		i,0
		SET		j,0
		
		% print strs[i] " <> " strs[j] " -> "
L1		LDOU	$5,strs,i
		PUSHJ	$4,putmsg
		SETL	$5,SEP
		PUSHJ	$4,putmsg
		LDOU	$5,strs,j
		PUSHJ	$4,putmsg
		SETL	$5,RES
		PUSHJ	$4,putmsg
		
		% compare strs[i] with strs[j]
		LDOU	$5,strs,i
		ORH		$5,#8000
		LDOU	$6,strs,j
		ORH		$6,#8000
		PUSHJ	$4,str:compare
		
		% print result
		SET		$5,0
		SET		$6,$4
		PUSHJ	$4,io:putn
		
		% print "\r\n"
		SETL	$4,CRLF
		PUSHJ	$3,putmsg
		
		CMP		$3,j,5*8
		BZ		$3,1F				% if j == 5*8, goto 1F
		ADDU	j,j,8
		JMP		L1
		
1H		CMP		$3,i,5*8
		BZ		$3,_done			% if i == 5*8, goto _done
		ADDU	i,i,8
		SET		j,0
		JMP		L1
		
_done	JMP		_done

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
