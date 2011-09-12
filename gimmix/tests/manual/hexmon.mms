
	%******************************************************%
	%*                                                    *%
	%*           B o o t M o n  -  G I M M I X            *%
	%*                                                    *%
	%*       Bootstrap Monitor for GIMMIX Processor       *%
	%*                                                    *%
	%*                     Version 1                      *%
	%*                                                    *%
	%*                10/2004 by H.Geisse                 *%
	%*                                                    *%
	%******************************************************%


%***********************************************************************
%			vectors
%***********************************************************************

	LOC	#FFFF00000000

Main	JMP		start		% bootmon cold start
cin		JMP		kbi			% console input
cout	JMP		dpo			% console output


%***********************************************************************
%			main program
%***********************************************************************

start	GETA	$1,signon	% show sign-on message
		PUSHJ	$0,msgout

		GETA	$1,gimmix	% show serial number of CPU
		PUSHJ	$0,msgout
		GET		$1,rN
		PUSHJ	$0,hexout
		GETA	$1,crlf
		PUSHJ	$0,msgout
		
		% TODO temporary: simply boot without asking
		PUSHJ	$0,bcmd

cmdloop	GETA	$1,prompt	% show prompt
		PUSHJ	$0,msgout
	
		PUSHJ	$0,echo		% get and echo a character
	
		GETA	$1,cmdtbl	% search for a command in table
cmdlp1	LDBU	$2,$1,0
		BZ		$2,error	% not found
		CMPU	$2,$2,$0
		BNZ		$2,cmdlp2	% not this one, try next one
		LDOU	$1,$1,8		% jump to command interpreting routine
		ORH		$1,#8000
		GO		$1,$1,0
cmdlp2	ADDU	$1,$1,16
		JMP		cmdlp1

error	GETA	$1,errmsg	% say that we did not understand
		PUSHJ	$0,msgout
		JMP		cmdloop

%***********************************************************************
%			empty command
%***********************************************************************

crcmd	GETA	$1,crlf		% terminate line
		PUSHJ	$0,msgout
		JMP		cmdloop


%***********************************************************************
%			bootstrap command
%***********************************************************************

bcmd	GETA	$1,crlf		% terminate line
		PUSHJ	$0,msgout
	
		SETH	$0,#8003
1H		LDOU	$1,$0,0		% wait until disk is ready
		BZ		$1,1B
		LDOU	$1,$0,24	% check if disk present
		CMP		$2,$1,0
		BZ		$2,nodsk

		GETA	$3,dfmsg	% say that we found a disk
		PUSHJ	$2,msgout
		SET		$3,$1		% and how many sectors it has
		PUSHJ	$2,hexout
		GETA	$3,sctmsg
		PUSHJ	$2,msgout
	
		SETL	$1,#0001	% sector count
		STOU	$1,$0,8
		SETL	$1,#0000	% first sector
		STOU	$1,$0,16
		SETL	$1,#0001	% start command
		STOU	$1,$0,0
dskwait	LDOU	$1,$0,0
		AND		$1,$1,#10	% ready?
		PBZ		$1,dskwait
		LDOU	$1,$0,0
		AND		$1,$1,#08	% error?
		BNZ		$1,dskerr
		
		SETH	$0,#8003	% load from disk-buffer into memory
		ORML	$0,#0008
		SETH	$1,#8000
		ORL		$1,#0000
		SET		$2,0
		SET		$4,512
cpyloop	LDOU	$3,$0,$2
		STOU	$3,$1,$2
		ADDU	$2,$2,8
		CMPU	$3,$2,$4
		PBN		$3,cpyloop
		
		% we have to sync the data- with the instruction-cache
		OR		$2,$1,$1
		SYNCD	#FF,$2,0	% first flush to memory; SYNCID will simply remove it
		SYNCID	#FF,$2,0
		ADDU	$2,$2,#FF
		ADDU	$2,$2,1	
		SYNCD	#FF,$2,0
		SYNCID	#FF,$2,0
		
		GO		$0,$1,0		% finally...lift off

nodsk	GETA	$1,dnfmsg	% there is no disk
		PUSHJ	$0,msgout
		JMP		cmdloop

dskerr	GETA	$1,demsg	% disk error
		PUSHJ	$0,msgout
		JMP		cmdloop

nosig	GETA	$1,nsgmsg	% no signature
		PUSHJ	$0,msgout
		JMP		cmdloop


%***********************************************************************
%			utilities
%***********************************************************************

	% output null-terminated message
	% in:	$0	pointer to message
	% out:	--
msgout	GET		$1,rJ
msgout1	LDBU	$3,$0
		CMP		$2,$3,0
		PBZ		$2,msgout2
		PUSHJ	$2,cout
		ADDU	$0,$0,1
		JMP		msgout1
msgout2	PUT		rJ,$1
		POP		0,0

	% output octabyte in hex
	% in:	$0	octabyte
	% out:	--
hexout	GET		$1,rJ
		SETL	$2,#000F
hex1	SLU		$3,$2,2
		SRU		$4,$0,$3
		PUSHJ	$3,nybout
		SUBU	$2,$2,1
		PBNN	$2,hex1
		PUT		rJ,$1
		POP		0,0

	% output nybble in hex
	% in:	$0	nybble
	% out:	--
nybout	AND		$0,$0,#0F
		ADDU	$0,$0,#30
		CMP		$1,$0,#3A
		PBN		$1,nyb1
		ADDU	$0,$0,7
nyb1	GET		$1,rJ
		SET		$3,$0
		PUSHJ	$2,cout
		PUT		rJ,$1
		POP		0,0

	% console echo
	% in:	--
	% out:	$X	character
echo	GET		$0,rJ
		PUSHJ	$1,cin
		SET		$3,$1
		PUSHJ	$2,cout
		PUT		rJ,$0
		POP		2,0


%***********************************************************************
%			I/O
%***********************************************************************

	% keyboard input
	% in:	--
	% out:	$X	character
kbi		SETH	$1,#8002
kbi1	LDOU	$2,$1,0
		AND		$2,$2,1
		PBZ		$2,kbi1
		LDOU	$0,$1,8
		POP		1,0

	% display output
	% in:	$0	character
	% out:	--
dpo		SETH	$1,#8002
dpo1	LDOU	$2,$1,16
		AND		$2,$2,1
		PBZ		$2,dpo1
		STOU	$0,$1,24
		POP		0,0


%***********************************************************************
%			data
%***********************************************************************

		LOC	@+(8-@)&7
cmdtbl	BYTE	#0D,0,0,0,0,0,0,0
		OCTA	crcmd
		BYTE	"b",0,0,0,0,0,0,0
		OCTA	bcmd
		BYTE	0,0,0,0,0,0,0,0
		OCTA	0

		LOC	@+(4-@)&3
signon	BYTE	"BootMon Version 1",#0D,#0A,0

		LOC	@+(4-@)&3
gimmix	BYTE	"GIMMIX S/N #",0

		LOC	@+(4-@)&3
crlf	BYTE	#0D,#0A,0

		LOC	@+(4-@)&3
prompt	BYTE	"#",0

		LOC	@+(4-@)&3
errmsg	BYTE	"?",#0D,#0A,0

		LOC	@+(4-@)&3
dnfmsg	BYTE	"Disk not found!",#0D,#0A,0

		LOC	@+(4-@)&3
dfmsg	BYTE	"Disk with #",0

		LOC	@+(4-@)&3
sctmsg	BYTE	" sectors found, booting...",#0D,#0A,0

		LOC	@+(4-@)&3
demsg	BYTE	"Disk error!",#0D,#0A,0

		LOC	@+(4-@)&3
nsgmsg	BYTE	"MBR signature missing!",#0D,#A,0
