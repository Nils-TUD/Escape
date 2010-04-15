import tango.text.Arguments;
import tango.core.Runtime;
import tango.io.Stdout;
import tango.stdc.stdlib;
import Integer = tango.text.convert.Integer;

int main(char[][] args) {
	Arguments a = new Arguments("");
	uint bs = 4096;
	uint count = 0;
	char[] inFile;
	char[] outFile;

	a.get("if=").bind(delegate void (char[] value) {
		inFile = value[0..$];
	});
	a.get("of=").bind(delegate void (char[] value) {
		outFile = value[0..$];
	});
	a.get("bs=").bind(delegate void (char[] value) {
		bs = Integer.toInt(value);
	});
	a.get("count=").bind(delegate void (char[] value) {
		count = Integer.toInt(value);
	});
	if(!a.parse(args,false))
		Stderr(a.errors(&Stderr.layout.sprint));
	
	Stdout.format("inFile={}, outFile={}, bs={}, count={}",inFile,outFile,bs,count).newline;

	/+parser.bind("","if=",delegate void(char[] value) {
		inFile = value[0..$];
	});
	parser.bind("","of=",delegate void(char[] value) {
		outFile = value[0..$];
	});
	parser.bind("","bs=",delegate void(char[] value) {
		bs = Integer.parse(value);
	});
	parser.bind("","count=",delegate void(char[] value) {
		count = Integer.parse(value);
	});
	parser.bindDefault(delegate void(char[] value, uint ordinal) {
		Stderr("Usage: {0} [if=<file>] [of=<file>] [bs=N] [count=N]",args[0]).newline;
		Stderr("	You can use the suffixes K, M and G to specify N").newline;
		exit(EXIT_FAILURE);
	});+/

	return 0;
}

/*
for(i = 1; i < argc; i++) {
	if(strncmp(argv[i],"if=",3) == 0)
		inFile = argv[i] + 3;
	else if(strncmp(argv[i],"of=",3) == 0)
		outFile = argv[i] + 3;
	else if(strncmp(argv[i],"bs=",3) == 0)
		bs = scanNumber(argv[i] + 3);
	else if(strncmp(argv[i],"count=",6) == 0)
		count = scanNumber(argv[i] + 6);
	else
		usage(argv[0]);
}
*/
