
#define DATE "Thu Aug 26 20:33:25 1999"
#define INSTRUCTIONS "32"

#define DOTEST(x,y,t) \
if (t) {							\
	extern int x(void);					\
	x(); a=x(); printf("%13s=%4d", #y, a-ohead);		\
	pos=pos+1;						\
	if(pos==4) {						\
		printf("\n");					\
		pos=0;						\
	} else {						\
		printf(" ");					\
	}							\
}

#define NL()							\
	if(pos!=0) {						\
		printf("\n");					\
		pos=0;						\
	}							\
	printf("\n");

int main(void) {
	int ohead, a, pos=0;
	int level0=1,
		level1=0,/*!system("grep -q mmx /proc/cpuinfo > /dev/null 2>&1"),*/
		level2=0;/*!system("grep -q 3dnow /proc/cpuinfo > /dev/null 2>&1");*/
	extern int oheadohead();
	oheadohead();
	ohead = oheadohead();
	printf("x86mark generated "DATE" --"
		" cycle counts per "INSTRUCTIONS" instructions\n");

DOTEST(addadd, ADD, level0)
DOTEST(mulmul, MUL, level0)
DOTEST(shlshl, SHL, level0)
DOTEST(addmul, ADD-MUL, level0)
DOTEST(addshl, ADD-SHL, level0)
DOTEST(jmpjmp, JMP, level0)
DOTEST(ujmpujmp, UJMP, level0)
DOTEST(addjmp, ADD-JMP, level0)
NL()
DOTEST(faddfadd, FADD, level0)
DOTEST(fmulfmul, FMUL, level0)
DOTEST(fmulfadd, FMUL-FADD, level0)
DOTEST(fdivfdiv, FDIV, level0)
DOTEST(fmulfdiv, FMUL-FDIV, level0)
DOTEST(faddfdiv, FADD-FDIV, level0)
DOTEST(fsqrtfsqrt, FSQRT, level0)
DOTEST(fsinfsin, FSIN, level0)
NL()
DOTEST(maddmadd, MADD, level0)
DOTEST(maddadd, MADD-ADD, level0)
DOTEST(amaddadd, AMADD-ADD, level0)
DOTEST(addstore, ADD-STORE, level0)
DOTEST(addload, ADD-LOAD, level0)
DOTEST(loadstore, LOAD-STORE, level0)
DOTEST(loadload, LOAD, level0)
DOTEST(storestore, STORE, level0)
NL()
DOTEST(addfmul, ADD-FMUL, level0)
DOTEST(mulfmul, MUL-FMUL, level0)
DOTEST(shlfmul, SHL-FMUL, level0)
DOTEST(addfadd, ADD-FADD, level0)
DOTEST(mulfadd, MUL-FADD, level0)
DOTEST(shlfadd, SHL-FADD, level0)
DOTEST(addfdiv, ADD-FDIV, level0)
DOTEST(oheadohead, OHEAD, level0)
NL()
DOTEST(ifaddifadd, IFADD, level0)
DOTEST(f7addf7add, F7ADD, level0)
DOTEST(f2addf2add, F2ADD, level0)
DOTEST(ifmulifmul, IFMUL, level0)
NL()
DOTEST(dfadddfadd, DFADD, level0)
DOTEST(dfadd1dfadd1, DFADD1, level0)
DOTEST(dfmuldfmul, DFMUL, level0)
DOTEST(dfdivdfdiv, DFDIV, level0)
DOTEST(dadddadd, DADD, level0)
DOTEST(dmuleax, DMUL-EAX, level0)
DOTEST(dmuledx, DMUL-EDX, level0)
NL()
DOTEST(pfmulpfmul, PFMUL, level2)
DOTEST(pfaddpfadd, PFADD, level2)
DOTEST(pfaddpfmul, PFADD-PFMUL, level2)
DOTEST(dpfmuldpfmul, DPFMUL, level2)
DOTEST(dpfadddpfadd, DPFADD, level2)
DOTEST(dpfadddpfmul, DPFADD-DPFMUL, level2)
NL()
DOTEST(pxorpxor, PXOR, level1)
DOTEST(paddwpaddw, PADDW, level1)
DOTEST(pmullwpmullw, PMULLW, level1)
DOTEST(dpxordpxor, DPXOR, level1)
DOTEST(dpaddwdpaddw, DPADDW, level1)
DOTEST(dpmullwdpmullw, DPMULLW, level1)
DOTEST(pxorpaddw, PXOR-PADDW, level1)
DOTEST(pxorpmullw, PXOR-PMULLW, level1)
DOTEST(paddwpmullw, PADDW-PMULLW, level1)
if(pos) printf("");return 0;
}
