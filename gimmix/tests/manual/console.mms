%
% console.mms -- interactive MMIX-console
%

		LOC		#1000
		JMP		Main

CRLF	BYTE	#d,#a,0
WELC1	BYTE	"Welcome to the interactive MMIX-console!",#d,#a,0
WELC2	BYTE	"You can use 'm <addr>' and 'r <rno>' (<addr> and <rno> are hex)",#d,#a,#d,#a,0
PROMPT	BYTE	"> ",0
ERROR	BYTE	"Invalid command!",#d,#a,0
MEM1	BYTE	"m[",0
MEM2	BYTE	"]=",0
RA		BYTE	"rA=",0
RB		BYTE	"rB=",0
RC		BYTE	"rC=",0
RD		BYTE	"rD=",0
RE		BYTE	"rE=",0
RF		BYTE	"rF=",0
RG		BYTE	"rG=",0
RH		BYTE	"rH=",0
RI		BYTE	"rI=",0
RJ		BYTE	"rJ=",0
RK		BYTE	"rK=",0
RL		BYTE	"rL=",0
RM		BYTE	"rM=",0
RN		BYTE	"rN=",0
RO		BYTE	"rO=",0
RP		BYTE	"rP=",0
RQ		BYTE	"rQ=",0
RR		BYTE	"rR=",0
RS		BYTE	"rS=",0
RT		BYTE	"rT=",0
RU		BYTE	"rU=",0
RV		BYTE	"rV=",0
RW		BYTE	"rW=",0
RX		BYTE	"rX=",0
RY		BYTE	"rY=",0
RZ		BYTE	"rZ=",0
RBB		BYTE	"rBB=",0
RTT		BYTE	"rTT=",0
RWW		BYTE	"rWW=",0
RXX		BYTE	"rXX=",0
RYY		BYTE	"rYY=",0
RZZ		BYTE	"rZZ=",0
RSS		BYTE	"rSS=",0
SPREGS	OCTA	RB,RD,RE,RH,RJ,RM,RR,RBB,RC,RN,RO,RS,RI,RT,RTT,RK,RQ,RU,RV,RG,RL,RA,RF,RP,RW,RX,RY
		OCTA	RZ,RWW,RXX,RYY,RZZ,RSS

cmdm	JMP		_cmdm
cmdr	JMP		_cmdr

CMDNM	BYTE	"m ",0
CMDNR	BYTE	"r ",0
CMDNS	OCTA	CMDNM,CMDNR,0
CMDS	OCTA	cmdm,cmdr,0
BUF		BYTE	0
		LOC		@+31

		% write welcome-message
Main	SET		$1,0
		SET		$2,WELC1
		ORH		$2,#8000
		PUSHJ	$0,io:puts
		SET		$1,0
		SET		$2,WELC2
		ORH		$2,#8000
		PUSHJ	$0,io:puts

		% print prompt
loop	SET		$1,PROMPT
		PUSHJ	$0,putmsg
		
		% read into BUF
		SET 	$1,0
		SET		$2,BUF
		ORH		$2,#8000
		SET		$3,30
		PUSHJ	$0,io:gets

		% search for requested command
		SET		$0,0		
		SET		$1,CMDNS
		ORH		$1,#8000
		SET		$2,BUF
		ORH		$2,#8000
1H		LDOU	$5,$1,$0
		BZ		$5,2F
		% compare BUF with command-name
		LDBU	$3,$2,2				% $3 = buf[2]
		STBU	$10,$2,2			% buf[2] = 0
		ORH		$5,#8000
		SET		$6,$2
		PUSHJ	$4,str:compare
		STBU	$3,$2,2				% buf[2] = $3
		BZ		$4,3F
		ADDU	$0,$0,8
		JMP		1B
		
		% print unknown command
2H		SET		$1,ERROR
		PUSHJ	$0,putmsg
		JMP		loop
		
		% call command-function
3H		SET		$1,CMDS
		ORH		$1,#8000
		LDOU	$2,$1,$0
		ORH		$2,#8000
		PUSHGO	$0,$2,0
		JMP		loop


% void _cmdr(void)
_cmdr	GET		$0,rJ
		SET		$2,BUF
		ORH		$2,#8000
		ADDU	$2,$2,2
		SET		$3,16
		PUSHJ	$1,str:touint
		CMPU	$2,$1,#20			% invalid reg-no?
		PBNP	$2,2F
		SET		$2,ERROR
		PUSHJ	$1,putmsg
		JMP		1F
		% print special-register
2H		SET		$3,0
		SETL	$4,SPREGS
		ORH		$4,#8000
		MULU	$5,$1,8
		LDOU	$4,$4,$5			% SPREGS[$1*8]
		ORH		$4,#8000
		PUSHJ	$2,io:puts
		% print value
		SET		$3,0
		SET		$4,$1
		SET		$5,16
		PUSHJ	$2,putsp
		% print "\r\n"
		SETL	$2,CRLF
		PUSHJ	$1,putmsg
1H		PUT		rJ,$0
		POP		0,0

% void _cmdm(void)
_cmdm	GET		$0,rJ
		SET		$2,BUF
		ORH		$2,#8000
		ADDU	$2,$2,2
		SET		$3,16
		PUSHJ	$1,str:touint
		% print "m["
		SETL	$3,MEM1
		PUSHJ	$2,putmsg
		% print address
		SET		$3,0
		SET		$4,$1
		SET		$5,16
		PUSHJ	$2,io:putu
		% print "]="
		SETL	$3,MEM2
		PUSHJ	$2,putmsg
		% print value
		SET		$3,0
		SET		$4,$1
		SET		$5,16
		PUSHJ	$2,putm
		% print "\r\n"
		SETL	$2,CRLF
		PUSHJ	$1,putmsg
		PUT		rJ,$0
		POP		0,0

% void putmsg(char *msg)
putmsg	GET		$1,rJ
		SET		$3,0
		SET		$4,$0
		ORH		$4,#8000
		PUSHJ	$2,io:puts
		PUT		rJ,$1
		POP		0,0

% void putm(octa term,octa addr,octa base)
putm	GET		$3,rJ				% save rJ
		OR		$5,$0,$0
		ORH		$1,#8000
		LDOU	$6,$1,0				% load from that address
		OR		$7,$2,$2
		PUSHJ	$4,io:putu			% call putu(term,$6,base)
		PUT		rJ,$3				% restore rJ
		POP		0,0
		
% void putsp(octa term,octa rno,octa base)
putsp	GET		$3,rJ				% save rJ
		SETML	$6,#FE06			% fabricate instruction: GET $6,X
		OR		$6,$6,$1
		GETA	$7,@+16
		STTU	$6,$7,0				% store instr
		SYNCD	#3,$7,0				% flush DC to mem
		SYNCID	#3,$7,0				% remove from IC and DC
		XOR		$0,$0,$0			% will be replaced
		OR		$5,$0,$0
		OR		$7,$2,$2
		PUSHJ	$4,io:putu			% call putu(term,$6,base)
		PUT		rJ,$3				% restore rJ
		POP		0,0

#include "string.mmi"
#include "io.mmi"
