%
% signal.mms -- shows how a signal could be handled
%

		% syscalls
ESIG	IS		1
PHELLO	IS		2

		% kernel stack
		LOC		#7000
KSTACK	OCTA	0


		% kernel data-area
		LOC		#6000
		
		% segmentsizes: 1,0,0,0; pageSize=2^13; r=0x2000; n=0
RV		OCTA	#10000D0000020000

		LOC		#00020000
		OCTA	#0000000000000007	% PTE  0    (#0000000000000000 .. #0000000000001FFF)
		LOC		#00020008
		OCTA	#0000000000002007	% PTE  1    (#0000000000002000 .. #0000000000003FFF)
		LOC		#00020010
		OCTA	#0000000000004007	% PTE  2    (#0000000000004000 .. #0000000000005FFF)
		
SREGS	OCTA	0
		OCTA	0
		OCTA	0
		OCTA	0
		OCTA	0
	
HELLO	BYTE	"Hello World!",#d,#a,0


		% startup
		LOC		#1000
Main	GETA	$0,RV
		LDOU	$0,$0,0
		PUT		rV,$0
		
		GETA	$0,FTRAP
		PUT		rT,$0
		GETA	$0,DTRAP
		PUT		rTT,$0
		
		GETA	$0,KSTACK
		PUT		rSS,$0
		
		% enable terminal-interrupts
		SETH	$0,#8002
		STCO	#3,$0,0
		
		% go to USER
		GETA	$0,USER
		ANDNH	$0,#8000
		PUT		rWW,$0
		SETH	$0,#8000
		PUT		rXX,$0
		NEG		$255,0,1		% rK
		RESUME	1
		
		
		% dynamic trap handler
DTRAP	SAVE	$255,1
		GET		$0,rQ
		PUT		rQ,0
		SETH	$0,#8002
		LDOU	$0,$0,8			% load char to stop device from sending interrupts
		GETA	$0,SREGS
		LDOU	$1,$0,32
		BNZ		$1,1F			% blocked? then continue
		GET		$1,rWW
		STOU	$1,$0,0			% SREGS[0] = rWW
		GET		$1,rXX
		STOU	$1,$0,8			% SREGS[1] = rXX
		GET		$1,rYY
		STOU	$1,$0,16		% SREGS[2] = rYY
		GET		$1,rZZ
		STOU	$1,$0,24		% SREGS[3] = rZZ
		STCO	#1,$0,32		% block signal
		% set rWW to address of sighandler-entry
		GETA	$0,SIGHE
		ANDNH	$0,#8000
		PUT		rWW,$0
1H		UNSAVE	1,$255
		NEG		$255,0,1		% rK
		RESUME	1
		
		
		% trap handler
FTRAP	SAVE	$255,1
		GET		$1,rXX
		AND		$1,$1,#FF		% Z-field specifies the syscall
		CMPU	$2,$1,ESIG
		BZ		$2,1F			% end-signal?
		CMPU	$2,$1,PHELLO
		BZ		$2,2F			% print hello?
		
		% restore special-register
1H		GETA	$0,SREGS
		LDOU	$1,$0,0
		PUT		rWW,$1			% rWW = SREGS[0]
		LDOU	$1,$0,8
		PUT		rXX,$1			% rXX = SREGS[1]
		LDOU	$1,$0,16
		PUT		rYY,$1			% rYY = SREGS[2]
		LDOU	$1,$0,24
		PUT		rZZ,$1			% rZZ = SREGS[3]
		STCO	#0,$0,32		% unblock signal
		JMP		3F

		% just for testing
2H		SET		$1,0
		GETA	$2,HELLO
		PUSHJ	$0,io:puts		% io:puts(0,HELLO)
		
3H		UNSAVE	1,$255
		NEG		$255,0,1		% rK
		RESUME	1

#include "io.mmi"


		% user code
		LOC		#2000
USER	JMP		USER

		% signal-handler entry
SIGHE	SAVE	$255,0
		PUSHJ	$255,SIGH
		UNSAVE	0,$255
		TRAP	0,0,ESIG
	
		% the real signal-handler
SIGH	TRAP	0,0,PHELLO
		POP		0,0
