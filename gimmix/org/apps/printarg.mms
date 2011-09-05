t	IS	$255
value	GREG	#1234567812345678

%	LOC	Data_Segment
%cbuf	BYTE	0,0

% main(int argc,char **argv);
	LOC	#100
Main	ADD	$1,$1,8		% skip argv[0]
	LDOU	$2,$1		% load argv[1]
	PUSHJ	$1,atoi		% $1 = atoi(argv[1])
	SET	$2,$1
	PUSHJ	$1,printn	% printn($1)
	SET	$2,#0A
	PUSHJ	$1,printc	% printc('\n')
	TRAP	0,Halt,0

% octa atoi(char *str);
atoi	GET	$5,rJ		% save rJ
	SET	$2,$0		% $2 = str
	SET	$0,0		% $0 = return-val
	SET	$1,0		% not negative by default
	LDB	$3,$2		% $3 = *str
	CMP	$4,$3,'-'
	PBNZ	$4,1F		% if $3 != '-', goto 1H (we don't need to load the byte again)
	SET	$1,1		% negative
	ADD	$2,$2,1		% str++
_loop	LDB	$3,$2		% $3 = *str
1H	BZ	$3,_done	% if $3 == 0, goto _done
	MUL	$0,$0,10	% $0 *= 10
	SUB	$3,$3,'0'	% $3 -= '0'
	ADD	$0,$0,$3	% $0 += $3
	ADD	$2,$2,1		% str++
	JMP	_loop		% goto _loop
_done	PBZ	$1,_end		% if not negative, goto _end
	NEG	$0,0,$0		% $0 = -$0
_end	PUT	rJ,$5		% restore rJ
	POP	1,0		% return octa

% void printc(octa c);
cbuf	BYTE	0,0
printc	STB	$0,cbuf		% store byte in cbuf
	LDA	t,cbuf		% get address
	TRAP	0,Fputs,StdOut	% write to stdout
	POP	0

% void printn(octa n);
printn	GET	$3,rJ		% save rJ
	PBNN	$0,1F		% n negative?
	SET	$5,'-'		% print '-'
	PUSHJ	$4,printc
	NEG	$0,0,$0		% n = -n
1H	CMP	$1,$0,10
	DIV	$0,$0,10	% n /= 10
	GET	$2,rR		% $2 = remainder
	BN	$1,2F		% oldn < 10?
	SET	$5,$0
	PUSHJ	$4,printn	% printn(n)
2H	ADD	$5,$2,'0'
	PUSHJ	$4,printc	% printc(remainder + '0')
	PUT	rJ,$3		% restore rJ
	POP	0

