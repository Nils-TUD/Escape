% expects lringsize = 8!

% main(int argc,char **argv);
	LOC	#100
Main	SET	$0,#00
	SET	$1,#01
	SET	$2,#02
	SET	$4,#04
	SET	$5,#05
	PUSHJ	$3,func
	TRAP	0,Halt,0

% void func(int a,int b);
func	SET	$0,#10
	SET	$1,#11
	SET	$2,#12
	SET	$3,#13
	SET	$4,#14
	POP	4,0

