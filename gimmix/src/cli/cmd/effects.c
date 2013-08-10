/**
 * $Id: effects.c 239 2011-08-30 18:28:06Z nasmussen $
 */

#include <string.h>

#include "common.h"
#include "cli/cmds.h"
#include "cli/cmd/effects.h"
#include "cli/console.h"
#include "cli/lang/specreg.h"
#include "cli/lang/idesc.h"
#include "cli/lang/symmap.h"
#include "core/cpu.h"
#include "core/register.h"
#include "core/mmu.h"
#include "core/tc.h"
#include "core/decoder.h"
#include "core/bus.h"
#include "core/cache.h"
#include "event.h"
#include "exception.h"
#include "mmix/io.h"
#include "mmix/error.h"

// the basic idea here is to backup all possibly changing data before an instruction is executed
// and print the changed stuff after the execution. this works in a really simple way for all
// simple instructions. the RESUME-actions are traced "manually" and all other kind of effects are
// traced via events. these are device-reads/-writes, stack-reads/-stores, virtual address
// translations and cache-fills/-flushes.

#define MAX_FORMAT_LEN		255
#define FL_HEX				1
#define FL_PADZEROS			2
#define FL_BASE				4
#define FL_SPECIAL			8
#define FL_NOREG			16
#define FL_WYDE				32
#define FL_FLOAT			64
#define FL_EXPLROUND		128

#define RESUME_AGAIN		0	// repeat the command in rX as if in location rW - 4
#define RESUME_CONT			1	// same, but substitute rY and rZ for operands
#define RESUME_SET			2	// set r[X] to rZ
#define RESUME_TRANS		3	// install (rY, rZ) into IT-cache or DT-cache, then RESUME_AGAIN

#define INSTREV_DEV			0
#define INSTREV_STACK		1
#define INSTREV_VAT			2
#define INSTREV_CACHE		3
#define INSTREV_DEVCB		4
#define MAX_EVENTS			4096

#define SHOW_DEV			1
#define SHOW_STACK			2
#define SHOW_VAT			4
#define SHOW_CACHE			8
#define SHOW_FETCH_EVENTS	16

typedef struct {
	octa dst;
	octa y;
	octa z;
} sFmtArgs;

typedef struct {
	int type;
	bool fetch;
	union {
		struct {
			octa addr;
			int mode;
			octa phys;
			bool usedTC;
			int exception;
			octa exBits;
		} vat;
		struct {
			bool load;
			octa addr;
			octa value;
			int type;
			int index;
		} stack;
		struct {
			bool load;
			octa addr;
			octa value;
		} dev;
		struct {
			const sDevice *device;
			int param;
		} devCB;
		struct {
			bool fill;
			int cache;
			octa addr;
		} cache;
	} d;
} sInstrEvent;

static void registerEvents(void);
static void unregisterEvents(void);
static void beforeExec(const sEvArgs *args);
static void beforeFetch(const sEvArgs *args);
static void exception(const sEvArgs *args);
static void stackLoadStore(const sEvArgs *args);
static void devReadWrite(const sEvArgs *args);
static void devCallback(const sEvArgs *args);
static void cacheFillFlush(const sEvArgs *args);
static void virtAddrTrans(const sEvArgs *args);
static void saveData(const sInstrArgs *args,const sInstr *instr,sFmtArgs *fargs);
static void traceResume(const sInstrArgs *args,int enabledEvents);
static void printEvents(int enabledEvents);
static void printCacheEvent(sInstrEvent *ev);
static void printDevEvent(sInstrEvent *ev);
static void printVatEvent(sInstrEvent *ev);
static void printStackEvent(sInstrEvent *ev);
static void printRegStackLoc(int type,int index);
static void printEffects(const char *fmt,const sFmtArgs *args);
static void printVal(octa val,int flags);

static octa oldrL = 0;
static octa oldrA = 0;
static octa oldrS = 0;
static octa oldrO = 0;
static octa oldrG = 0;
static octa oldrX = 0;
static octa oldrXX = 0;
static octa oldrY = 0;
static octa oldrYY = 0;
static octa oldrZ = 0;
static octa oldrZZ = 0;
static octa oldrQ = 0;
static octa oldPC = 0;
static bool oldPriv = false;
static sFmtArgs instrArgs;
static sFmtArgs resumeArgs;
static int exNo = 0;
static octa exBits = 0;
static octa exPC = 0;
static bool beforeExecCalled = false;

static bool eventsEnabled = false;
static size_t eventCount = 0;
static sInstrEvent events[MAX_EVENTS];

void cli_cmd_effectsInit(void) {
	// by default, events are enabled
	registerEvents();
}

void cli_cmd_effects(size_t argc,const sASTNode **argv) {
	if(argc > 1)
		cmds_throwEx(NULL);

	int enabledEvents = SHOW_STACK;
	if(argc == 1) {
		bool finished;
		sCmdArg evs = cmds_evalArg(argv[0],ARGVAL_STR,0,&finished);
		const char *evStr = evs.d.str;
		if(strcmp(evStr,"on") == 0) {
			registerEvents();
			return;
		}
		else if(strcmp(evStr,"off") == 0) {
			unregisterEvents();
			return;
		}
		else {
			enabledEvents = 0;
			while(*evStr) {
				switch(*evStr) {
					case 'd':
						enabledEvents |= SHOW_DEV;
						break;
					case 's':
						enabledEvents |= SHOW_STACK;
						break;
					case 'v':
						enabledEvents |= SHOW_VAT;
						break;
					case 'c':
						enabledEvents |= SHOW_CACHE;
						break;
					case 'f':
						enabledEvents |= SHOW_FETCH_EVENTS;
						break;
					default:
						cmds_throwEx("Invalid event: %c\n",*evStr);
				}
				evStr++;
			}
		}
	}

	if(!eventsEnabled)
		cmds_throwEx("Please re-enable the effects first via 'e on'\n");

	tetra raw = cpu_getCurInstrRaw();
	const sInstrArgs *args = cpu_getCurInstrArgs();
	const sInstrDesc *desc = idesc_get(OPCODE(raw));

	octa funcStart = 0;
	const char *name = symmap_getNearestFuncName(oldPC,&funcStart);
	if(beforeExecCalled) {
		mprintf("%016OX <%-16.16s>: %-23s: ",oldPC,name,idesc_disasm(oldPC,raw,false));
		if(OPCODE(raw) == RESUME) {
			// RESUME itself can never affect the register-stack or set bits in rA
			printEffects(desc->effect,&instrArgs);
			traceResume(args,enabledEvents);
		}
		else {
			printEvents(enabledEvents);
			printEffects(desc->effect,&instrArgs);
			if(reg_getSpecial(rA) != oldrA)
				mprintf(", rA=#%OX=%s",reg_getSpecial(rA),sreg_explain(rA,reg_getSpecial(rA)));
		}
	}
	// if beforeExec has not been called we got an exception before that. so we don't know anything
	// about the current instruction
	else
		mprintf("%016OX <%-16.16s>: %-23s: ???",oldPC,name,"???");

	if(reg_getSpecial(rQ) != oldrQ)
		mprintf(", rQ=#%OX=%s",reg_getSpecial(rQ),sreg_explain(rQ,reg_getSpecial(rQ)));
	if(exNo != EX_NONE) {
		int trap = exNo == EX_DYNAMIC_TRAP || exNo == EX_FORCED_TRAP;
		mprintf("\n%62s%s -> #%OX","",ex_toString(exNo,exBits),cpu_getPC());
		mprintf("\n%62srW=#%OX, rX=#%OX, rY=#%OX, rZ=#%OX","",
			reg_getSpecial(trap ? rWW : rW),reg_getSpecial(trap ? rXX : rX),
			reg_getSpecial(trap ? rYY : rY),reg_getSpecial(trap ? rZZ : rZ));
		mprintf("\n%62srB=#%OX, $255=#%OX","",
			reg_getSpecial(trap ? rBB : rB),reg_get(255));
	}
	else if(beforeExecCalled && OPCODE(raw) == TRAP && args->x == 0 && args->y == 0 && args->z == 0)
		mprintf(" -> CPU halted");
	mprintf("\n");
}

static void registerEvents(void) {
	if(eventsEnabled)
		return;

	ev_register(EV_BEFORE_EXEC,beforeExec);
	ev_register(EV_BEFORE_FETCH,beforeFetch);
	ev_register(EV_EXCEPTION,exception);
	ev_register(EV_STACKLOAD,stackLoadStore);
	ev_register(EV_STACKSTORE,stackLoadStore);
	ev_register(EV_DEV_READ,devReadWrite);
	ev_register(EV_DEV_WRITE,devReadWrite);
	ev_register(EV_DEV_CALLBACK,devCallback);
	ev_register(EV_CACHE_FILL,cacheFillFlush);
	ev_register(EV_CACHE_FLUSH,cacheFillFlush);
	ev_register(EV_VAT,virtAddrTrans);
	eventsEnabled = true;
}

static void unregisterEvents(void) {
	if(!eventsEnabled)
		return;

	ev_unregister(EV_BEFORE_EXEC,beforeExec);
	ev_unregister(EV_BEFORE_FETCH,beforeFetch);
	ev_unregister(EV_EXCEPTION,exception);
	ev_unregister(EV_STACKLOAD,stackLoadStore);
	ev_unregister(EV_STACKSTORE,stackLoadStore);
	ev_unregister(EV_DEV_READ,devReadWrite);
	ev_unregister(EV_DEV_WRITE,devReadWrite);
	ev_unregister(EV_DEV_CALLBACK,devCallback);
	ev_unregister(EV_CACHE_FILL,cacheFillFlush);
	ev_unregister(EV_CACHE_FLUSH,cacheFillFlush);
	ev_unregister(EV_VAT,virtAddrTrans);
	eventsEnabled = false;
}

static void beforeFetch(const sEvArgs *args) {
	UNUSED(args);
	eventCount = 0;
	exNo = 0;
	exBits = 0;
	exPC = 0;
	// store pc to know it for the next instruction in the case that beforeExec is not called
	beforeExecCalled = false;
	oldPC = cpu_getPC();
	oldPriv = cpu_isPriv();
}

static void beforeExec(const sEvArgs *args) {
	UNUSED(args);
	tetra raw = cpu_getCurInstrRaw();
	const sInstrArgs *iargs = cpu_getCurInstrArgs();
	const sInstr *instr = cpu_getCurInstr();

	beforeExecCalled = true;
	oldrL = reg_getSpecial(rL);
	oldrA = reg_getSpecial(rA);
	oldrS = reg_getSpecial(rS);
	oldrO = reg_getSpecial(rO);
	oldrG = reg_getSpecial(rG);
	oldrX = reg_getSpecial(rX);
	oldrXX = reg_getSpecial(rXX);
	oldrY = reg_getSpecial(rY);
	oldrYY = reg_getSpecial(rYY);
	oldrZ = reg_getSpecial(rZ);
	oldrZZ = reg_getSpecial(rZZ);
	oldrQ = reg_getSpecial(rQ);
	oldPC = cpu_getPC();
	oldPriv = cpu_isPriv();
	saveData(iargs,instr,&instrArgs);
	// for RESUME_AGAIN: save also the args for the instruction in rX/rXX
	if(OPCODE(raw) == RESUME) {
		octa rx = (iargs->y == 1 && (oldPC & MSB(64))) ? oldrXX : oldrX;
		if(rx >> 56 == RESUME_AGAIN || rx >> 56 == RESUME_TRANS) {
			const sInstr *ninstr = dec_getInstr(OPCODE(rx));
			sInstrArgs nargs;
			dec_decode((tetra)rx,&nargs);
			saveData(&nargs,ninstr,&resumeArgs);
		}
	}
}

static void exception(const sEvArgs *args) {
	exNo = args->arg1;
	exBits = args->arg2;
	exPC = cpu_getPC();
}

static void stackLoadStore(const sEvArgs *args) {
	if(eventCount >= MAX_EVENTS)
		return;
	sInstrEvent *ev = events + eventCount;
	eventCount++;
	ev->type = INSTREV_STACK;
	ev->fetch = !beforeExecCalled;
	ev->d.stack.load = args->event == EV_STACKLOAD;
	ev->d.stack.type = args->arg1;
	ev->d.stack.index = args->arg2;
	ev->d.stack.addr = reg_getSpecial(rS);

	// better catch the exception here; just to be sure
	jmp_buf env;
	int ex = setjmp(env);
	if(ex != EX_NONE)
		ev->d.stack.value = 0;
	else {
		ex_push(&env);
		ev->d.stack.value = mmu_readOcta(ev->d.stack.addr,0);
	}
	ex_pop();
}

static void devReadWrite(const sEvArgs *args) {
	if(eventCount >= MAX_EVENTS)
		return;
	sInstrEvent *ev = events + eventCount;
	ev->type = INSTREV_DEV;
	ev->fetch = !beforeExecCalled;
	ev->d.dev.load = args->event == EV_DEV_READ;
	ev->d.dev.addr = args->arg1;
	ev->d.dev.value = args->arg2;
	eventCount++;
}

static void devCallback(const sEvArgs *args) {
	if(eventCount >= MAX_EVENTS)
		return;
	sInstrEvent *ev = events + eventCount;
	ev->type = INSTREV_DEVCB;
	ev->fetch = !beforeExecCalled;
	ev->d.devCB.device = (const sDevice*)args->arg1;
	ev->d.devCB.param = args->arg2;
	eventCount++;
}

static void cacheFillFlush(const sEvArgs *args) {
	if(eventCount >= MAX_EVENTS)
		return;
	sInstrEvent *ev = events + eventCount;
	ev->type = INSTREV_CACHE;
	ev->fetch = !beforeExecCalled;
	ev->d.cache.fill = args->event == EV_CACHE_FILL;
	ev->d.cache.cache = args->arg1;
	ev->d.cache.addr = args->arg2;
	eventCount++;
}

static void virtAddrTrans(const sEvArgs *args) {
	if(eventCount >= MAX_EVENTS)
		return;

	sVirtTransReg vtr = mmu_unpackrV(reg_getSpecial(rV));
	sInstrEvent *ev = events + eventCount;
	eventCount++;
	ev->type = INSTREV_VAT;
	ev->fetch = !beforeExecCalled;
	int tc = args->arg2 == MEM_EXEC ? TC_INSTR : TC_DATA;
	ev->d.vat.addr = args->arg1;
	ev->d.vat.mode = args->arg2;
	octa key = tc_addrToKey(args->arg1,vtr.s,vtr.n);
	ev->d.vat.usedTC = !(args->arg1 & MSB(64)) && tc_getByKey(tc,key) != NULL;
	ev->d.vat.exception = EX_NONE;
	ev->d.vat.exBits = 0;

	// try translation and catch exception
	jmp_buf env;
	int ex = setjmp(env);
	if(ex != EX_NONE) {
		ev->d.vat.exception = ex;
		ev->d.vat.exBits = ex_getBits();
	}
	else {
		ex_push(&env);
		ev->d.vat.phys = mmu_translate(args->arg1,args->arg2,args->arg2,false);
	}
	ex_pop();
}

static void saveData(const sInstrArgs *args,const sInstr *instr,sFmtArgs *fargs) {
	fargs->dst = args->x;
	// special case for SET*,INC*,OR*,ANDN* to be able to show the old value
	if(instr->args == A_DS && instr->format == I_RI16)
		fargs->y = reg_get(args->x);
	else
		fargs->y = args->y;
	fargs->z = args->z;
}

static void traceResume(const sInstrArgs *args,int enabledEvents) {
	static char buffer[24];
	static const char *ropcodes[] = {"AGAIN","CONT","SET","TRANS"};
	octa rx,ry,rz,rw;
	if(args->y == 1 && oldPriv) {
		rx = oldrXX;
		ry = oldrYY;
		rz = oldrZZ;
		rw = reg_getSpecial(rWW);
	}
	else {
		rx = oldrX;
		ry = oldrY;
		rz = oldrZ;
		rw = reg_getSpecial(rW);
	}

	int ropcode = rx >> 56;
	mprintf(", %s",ropcode == 0x80 ? "SKIP" : (ropcode < 4 ? ropcodes[ropcode] : "???"));
	if(rx & MSB(64)) {
		if(!exPC)
			mprintf(" -> #%OX",rw);
		return;
	}
	// if use resume is false, RESUME itself has triggered an exception.
	// in this case, don't display the inserted instruction
	if(!cpu_useResume())
		return;

	mprintf(" -> #%OX",rw);

	// determine some common properties
	int dst = 0,rno = 0;
	const sInstr *ninstr = NULL;
	const sInstrDesc *ndesc = NULL;
	const char *name = "???";
	const char *disasm = "???";
	if(ropcode == RESUME_SET || ropcode == RESUME_CONT) {
		dst = (rx >> 16) & 0xFF;
		rno = dst >= (int)oldrG ? dst : (int)((oldrO + dst) & LREG_MASK);
		name = sreg_get(args->y == 1 && oldPriv ? rZZ : rZ)->name;
		msnprintf(buffer,sizeof(buffer),"%-7s $%d,%s","SET",rno,name);
		disasm = buffer;
		name = "SET";
	}
	if(ropcode == RESUME_CONT || ropcode == RESUME_AGAIN || ropcode == RESUME_TRANS) {
		ninstr = dec_getInstr(OPCODE(rx));
		ndesc = idesc_get(OPCODE(rx));
		disasm = idesc_disasm(rw - sizeof(tetra),rx,false);
		name = ndesc->name;
	}
	// display translation-effects
	if(ropcode == RESUME_TRANS) {
		sVirtTransReg vtr = mmu_unpackrV(reg_getSpecial(rV));
		mprintf("\n%62s%cTC[#%OX] = #%OX","",OPCODE(rx) == SWYM ? 'I' : 'D',
				tc_addrToKey(oldrYY,vtr.s,vtr.n),tc_pteToTrans(oldrZZ,vtr.s));
	}

	// display the effects of the instruction as if it has been inserted into the instruction-stream
	if(ropcode != RESUME_TRANS || OPCODE(rx) != SWYM) {
	    octa funcStart = 0;
	    const char *fname = symmap_getNearestFuncName(rw - sizeof(tetra),&funcStart);
		mprintf("\n");
		mprintf("%016OX <%-16.16s>: %-23s: ",rw - sizeof(tetra),fname,disasm);
		switch(ropcode) {
			case RESUME_SET:
				mprintf("$%d=%c[%d] = #%OX",dst,dst >= (int)oldrG ? 'g' : 'l',rno,rz);
				break;

			case RESUME_CONT:
				if(ninstr->args != A_III)
					mprintf("$%d=%c[%d] = %s ",dst,dst >= (int)oldrG ? 'g' : 'l',rno,name);
				if(ninstr->args == A_DSS)
					mprintf("#%OX #%OX = #%OX",ry,rz,reg_get(dst));
				else if(ninstr->args == A_DS)
					mprintf("#%OX = #%OX",rz,reg_get(dst));
				// A_III for trap
				else {
					mprintf("%d %d %d",(int)((rx >> 16) & 0xFF),(int)((rx >> 8) & 0xFF),
							(int)(rx & 0xFF),reg_get(dst));
				}
				break;

			case RESUME_AGAIN:
			case RESUME_TRANS:
				printEvents(enabledEvents);
				printEffects(ndesc->effect,&resumeArgs);
				break;
		}
		if(reg_getSpecial(rA) != oldrA)
			mprintf(", rA=#%OX",reg_getSpecial(rA));
	}
}

static void printEvents(int enabledEvents) {
	for(size_t i = 0; i < eventCount; i++) {
		sInstrEvent *ev = events + i;
		if(ev->fetch && !(enabledEvents & SHOW_FETCH_EVENTS))
			continue;
		switch(ev->type) {
			case INSTREV_DEV:
				if(enabledEvents & SHOW_DEV)
					printDevEvent(ev);
				break;
			case INSTREV_DEVCB:
				if(enabledEvents & SHOW_DEV) {
					mprintf("Device[%s]: Callback called with parameter %d\n%38s",
							ev->d.devCB.device->name,ev->d.devCB.param,"");
				}
				break;
			case INSTREV_STACK:
				if(enabledEvents & SHOW_STACK)
					printStackEvent(ev);
				break;
			case INSTREV_VAT:
				if(enabledEvents & SHOW_VAT)
					printVatEvent(ev);
				break;
			case INSTREV_CACHE:
				if(enabledEvents & SHOW_CACHE)
					printCacheEvent(ev);
				break;
		}
	}
}

static void printCacheEvent(sInstrEvent *ev) {
	const sCache *c = cache_getCache(ev->d.cache.cache);
	if(ev->d.cache.fill) {
		mprintf("%s: Filling m[#%OX .. #%OX]\n%62s",c->name,ev->d.cache.addr,
				ev->d.cache.addr + c->blockSize - 1,"");
	}
	else {
		mprintf("%s: Flushing m[#%OX .. #%OX]\n%62s",c->name,ev->d.cache.addr,
				ev->d.cache.addr + c->blockSize - 1,"");
	}
}

static void printDevEvent(sInstrEvent *ev) {
	const sDevice *dev = bus_getDevByAddr(ev->d.dev.addr);
	if(ev->d.dev.load) {
		mprintf("Device[%s]: m8[#%016OX] -> #%016OX\n%62s",dev->name,ev->d.dev.addr,
				ev->d.dev.value,"");
	}
	else {
		mprintf("Device[%s]: #%016OX -> m8[#%016OX]\n%62s",dev->name,ev->d.dev.value,
				ev->d.dev.addr,"");
	}
}

static void printVatEvent(sInstrEvent *ev) {
	mprintf("VAT: %c #%OX",
			(ev->d.vat.mode & MEM_READ) ? 'r' :
			(ev->d.vat.mode & MEM_WRITE) ? 'w' :
			(ev->d.vat.mode & MEM_EXEC) ? 'x' : '?',ev->d.vat.addr);
	if(ev->d.vat.exception != EX_NONE)
		mprintf(" -> %s",ex_toString(ev->d.vat.exception,ev->d.vat.exBits));
	else
		mprintf(" -> #%OX",ev->d.vat.phys);
	if(ev->d.vat.usedTC)
		mprintf(" (used %s)",ev->d.vat.mode == MEM_EXEC ? "ITC" : "DTC");
	mprintf("\n%62s","");
}

static void printStackEvent(sInstrEvent *ev) {
	if(ev->d.stack.load) {
		mprintf("rS -= %d, ",sizeof(octa));
		printRegStackLoc(ev->d.stack.type,ev->d.stack.index);
		mprintf(" = M8[#%016OX] = #%OX\n%62s",ev->d.stack.addr,ev->d.stack.value,"");
	}
	else {
		mprintf("M8[#%016OX] = ",ev->d.stack.addr);
		printRegStackLoc(ev->d.stack.type,ev->d.stack.index);
		mprintf(" = #%OX, rS += %d\n%62s",ev->d.stack.value,sizeof(octa),"");
	}
}

static void printRegStackLoc(int type,int index) {
	switch(type) {
		case RSTACK_DEFAULT:
			mprintf("l[%d]",index);
			break;
		case RSTACK_SPECIAL:
			mprintf("%s",sreg_get(index)->name);
			break;
		case RSTACK_GREGS:
			mprintf("g[%d]",index);
			break;
		case RSTACK_RGRA:
			mprintf("rG|rA");
			break;
	}
}

static void printEffects(const char *fmt,const sFmtArgs *fargs) {
	static char roundLeft[] =	{0,'[','^','_','('};
	static char roundRight[] =	{0,']','^','_',')'};
	while(true) {
		while(*fmt != '%') {
			if(*fmt == '\0')
				return;
			mprintf("%c",*fmt);
			fmt++;
		}

		int flags = 0;
		bool stop = false;
		while(!stop) {
			switch(*++fmt) {
				case '#':
					flags |= FL_HEX | FL_BASE;
					break;
				case 'h':
					flags |= FL_HEX;
					break;
				case 's':
					flags |= FL_SPECIAL;
					break;
				case 'n':
					flags |= FL_NOREG;
					break;
				case 'w':
					flags |= FL_WYDE;
					break;
				case '0':
					flags |= FL_PADZEROS;
					break;
				case 'e':
					flags |= FL_EXPLROUND;
					break;
				case '.':
					flags |= FL_FLOAT;
					break;
				default:
					stop = true;
					break;
			}
		}

		if(*fmt >= 'A' && *fmt <= 'Z') {
			const char name[] = {'r',*fmt,'\0'};
			printVal(reg_getSpecial(sreg_getByName(name)),flags);
		}
		else {
			switch(*fmt) {
				case '(':
				case ')': {
					int mode = 0;
					if(flags & FL_EXPLROUND)
						mode = fargs->y;
					if(mode == 0) {
						mode = (oldrA >> 16) & 0x3;
						mode = mode == 0 ? 4 : mode;
					}
					else if(mode > 4)
						mode = 4;
					mprintf("%c",*fmt == '(' ? roundLeft[mode] : roundRight[mode]);
				}
				break;

				case 'l': {
					octa grO = reg_getSpecial(rO) / sizeof(octa);
					int grG = (int)reg_getSpecial(rG);
					int dst = (int)fargs->dst;
					int rno = dst >= grG ? dst : (int)((grO + dst) & LREG_MASK);
					if(reg_getSpecial(rL) != oldrL)
						mprintf("rL=%d,",(int)reg_getSpecial(rL));
					mprintf("$%d=%c[%d]",dst,dst >= grG ? 'g' : 'l',rno);
				}
				break;
				case 'x': {
					octa val = (flags & (FL_SPECIAL | FL_NOREG)) ? fargs->dst : reg_get(fargs->dst);
					printVal(val,flags);
				}
				break;
				case 'r':
					printVal(cpu_getPC(),flags);
					break;
				case 'y':
					printVal(fargs->y,flags);
					break;
				case 'z':
					printVal(fargs->z,flags);
					break;
				case 'b':
					if(cpu_hasPCChanged())
						mprintf("yes -> #%OX",fargs->z);
					else
						mprintf("no");
					break;
				case 'p': {
					octa addr = fargs->y + fargs->z;
					if(reg_get(fargs->dst) == 1)
						mprintf("M8[#%OX]=#%OX",addr,mmu_readOcta(addr,0));
					else
						mprintf("rP=#%OX",mmu_readOcta(addr,0));
				}
				break;
				case '*':
					if(fargs->dst < SPECIAL_NUM)
						printVal(reg_getSpecial(fargs->dst),flags);
					else
						mprintf("??");
					break;
			}
		}
		fmt++;
	}
}

static void printVal(octa val,int flags) {
	if(flags & FL_SPECIAL) {
		const sSpecialReg *sr = sreg_get(val);
		mprintf("%s",sr ? sr->name : "?");
	}
	else if(flags & FL_FLOAT) {
		uDouble dval;
		dval.o = val;
		mprintf("%le",dval.d);
	}
	else {
		if((flags & FL_BASE) && (flags & FL_HEX))
			mprintf("#");
		if((flags & FL_HEX) && (flags & FL_PADZEROS)) {
			if(flags & FL_WYDE)
				mprintf("%04OX",val);
			else
				mprintf("%016OX",val);
		}
		else if(flags & FL_HEX)
			mprintf("%OX",val);
		else
			mprintf("%Od",val);
	}
}
