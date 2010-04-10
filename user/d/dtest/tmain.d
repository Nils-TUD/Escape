import std.c.stdio;
import core.thread;

shared static this() {
	printf("MUH!?\n");
}
shared static ~this() {
	printf("Aus die Maus!!\n");
}

static this() {
	printf("WAS??\n");
}
static ~this() {
	printf("bye\n");
}

int t1;
int t2;
int t3 = 123;
auto t4 = "bla und mehr";
auto t5 = "blub oder?";

extern (C) int gettid();

void testThread(void *arg) {
	char[] blub = cast(char[])"mythread";
	for(int i = 0; i < gettid() % 4; i++)
		blub ~= "+";
	Thread c = Thread.getThis();
	c.name(cast(string)blub);
	printf("I am a thread (%s)!!!\n",c.name().ptr);
	fflush(stdout);
	Thread.sleep(5_000_000);
	printf("nu aber");
	fflush(stdout);
}

void main(string[] args) {
	Thread t = new Thread(&testThread);
	t.start();
	Thread t2 = new Thread(&testThread);
	t2.start();
	try {
		printf("hier\n");
		printf("t1=%d, t2=%d, t3=%d, t4=%s, t5=%s\n",t1,t2,t3,t4.ptr,t5.ptr);
		try {
			myexfunc();
		}
		catch(Object x) {
			printf("Caught nested exception: %p\n",x);
			throw x;
		}
	}
	catch(Object a) {
		printf("Caught exception: %p\n",a);
	}
	finally {
		printf("Finally ...\n");
	}
}

unittest {
	assert(1);
}

void myexfunc() {
	throw new Object();
}

/+import std.c.stdio;
import std.c.stdarg;
import std.stdio;
import std.date;

extern (C) void sleep(uint);

static this() {
	printf("MUH macht die Kuh!!\n");
}
static this() {
	printf("noch eins\n");
}
static ~this() {
	printf("Terminating....\n");
}

class A {
	public:
		this() {
			printf("Constructor\n");
		}
		~this() {
			printf("Destructor\n");
		}
		
		void doit() {
			printf("Doit!!!\n");
		}
}

void f1(int a,int b) {
	void f2(int c) {
		void f3(int d) {
			printf("a=%d, b=%d, c=%d, d=%d\n",a,b,c,d);
		}
		f3(4);
	}
	f2(3);
}

void main(string[] args) {
	int[6] x = [1,2,3,4,84,123];
	char[] str = "abc" ~ "test";
	x[0] = 4;
	x[5] = 5;
	printf("Str=%s\n",str.ptr);
	printf("Args: (%d)",args.length);
	foreach(string a; args)
		printf("\t%s\n",a.ptr);
	foreach(int i, j; x)
		printf("%d: %d\n",i,j);
	varargs("abc",1235,12.4,'f');
	dvarargs(1,2,34,5);
	
	auto a = new A();
	a.doit();
	
	f1(1,2);
	
	d_time time = getUTCtime();
	//writefln("Date=%s",toUTCString(time));
	
	/*try {
		throw new Object();
	}
	catch(Object a) {
		printf("Caught exception\n");
	}*/
	
	int bla;
	printf("Enter a number: ");
	scanf("%d",&bla);
	printf("Entered: %d\n",bla);
}

void dvarargs(int[] args ...) {
	foreach(int a; args)
		printf("arg: %d\n",a);
}

void varargs(string a,...) {
	va_list ap;
	va_start!(string)(ap,a);
	int a1 = va_arg!(int)(ap);
	float a2 = va_arg!(float)(ap);
	char a3 = va_arg!(char)(ap);
	printf("Args=%d %f %c\n",a1,a2,a3);
	va_end(ap);
}+/
