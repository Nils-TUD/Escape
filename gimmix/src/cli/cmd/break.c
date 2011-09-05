/**
 * $Id: break.c 239 2011-08-30 18:28:06Z nasmussen $
 */

#include <string.h>

#include "common.h"
#include "cli/cmds.h"
#include "cli/cmd/break.h"
#include "cli/lang/symmap.h"
#include "cli/console.h"
#include "core/cpu.h"
#include "core/mmu.h"
#include "mmix/io.h"
#include "mmix/mem.h"
#include "exception.h"
#include "event.h"

#define MAX_BREAKS		16
#define MAX_EXPR_LEN	64

#define BREAK_VIRTUAL	'v'
#define BREAK_PHYSICAL	'p'
#define BREAK_EXPR		'e'

typedef struct {
	bool enabled;
	char type;
	union {
		struct {
			int mode;
			octa addr;
		} a;
		struct {
			bool hasValue;
			bool waitForChange;
			sASTNode *expr;
			octa *values;
			size_t size;
		} e;
	} d;
} sBreakpoint;

static void addAddrBreakpoint(sBreakpoint *bp,const sASTNode *nmode,const sASTNode *naddr,
		bool virtual);
static void addExprBreakpoint(sBreakpoint *bp,const sASTNode *expr,const sASTNode *val);
static void translationCallback(const sEvArgs *args);
static void afterExecCallback(const sEvArgs *args);
static int decodeMode(const char *mode);
static void resetBreakpoint(sBreakpoint *bp);
static void printBreakpoints(void);
static void printBreakpoint(size_t index,sBreakpoint *bp);
static void printMode(int mode);
static sBreakpoint *newBreakpoint(void);

static bool delayedBreak = false;
static bool ignoreInstr = false;
static int addrBreakpoints = 0;
static int exprBreakpoints = 0;
static sBreakpoint breakpoints[MAX_BREAKS];
static char exprStr[MAX_EXPR_LEN];

void cli_cmd_breakInit(void) {
	ev_register(EV_AFTER_EXEC,afterExecCallback);
	ev_register(EV_VAT,translationCallback);
}

void cli_cmd_break(size_t argc,const sASTNode **argv) {
	if(argc != 0 && argc != 1 && argc != 2 && argc != 3)
		cmds_throwEx(NULL);

	if(argc == 0)
		printBreakpoints();
	else {
		sBreakpoint *bp = newBreakpoint();
		if(bp == NULL)
			cmds_throwEx("Max. number of breakpoints reached!\n");

		if(argc == 1)
			addAddrBreakpoint(bp,NULL,argv[0],true);
		else {
			bool finished = false;
			sCmdArg cmd = cmds_evalArg(argv[0],ARGVAL_STR,0,&finished);
			if(strcmp(cmd.d.str,"v") == 0 || strcmp(cmd.d.str,"p") == 0) {
				if(argc != 3)
					cmds_throwEx(NULL);
				addAddrBreakpoint(bp,argv[1],argv[2],strcmp(cmd.d.str,"v") == 0);
			}
			else if(strcmp(cmd.d.str,"e") == 0) {
				if(argc != 2 && argc != 3)
					cmds_throwEx(NULL);
				addExprBreakpoint(bp,argv[1],argc == 3 ? argv[2] : NULL);
			}
			else
				cmds_throwEx(NULL);
		}

		mprintf("Added breakpoint: ");
		printBreakpoint(bp - breakpoints,bp);
	}
}

void cli_cmd_delBreak(size_t argc,const sASTNode **argv) {
	if(argc != 1)
		cmds_throwEx(NULL);

	for(octa offset = 0; ; offset++) {
		bool finished = false;
		sCmdArg no = cmds_evalArg(argv[0],ARGVAL_INT,offset,&finished);
		if(finished)
			break;
		if(no.d.integer >= MAX_BREAKS || !breakpoints[no.d.integer].enabled)
			cmds_throwEx("Invalid breakpoint-number!\n");

		mprintf("Removed breakpoint: ");
		printBreakpoint(no.d.integer,breakpoints + no.d.integer);
		resetBreakpoint(breakpoints + no.d.integer);
	}
}

static void addAddrBreakpoint(sBreakpoint *bp,const sASTNode *nmode,const sASTNode *naddr,
		bool virtual) {
	bool finished = false;
	if(nmode != NULL) {
		sCmdArg mode = cmds_evalArg(nmode,ARGVAL_STR,0,&finished);
		bp->d.a.mode = decodeMode(mode.d.str);
	}
	else
		bp->d.a.mode = MEM_EXEC;
	bp->type = virtual ? BREAK_VIRTUAL : BREAK_PHYSICAL;
	sCmdArg addr = cmds_evalArg(naddr,ARGVAL_INT | ARGVAL_STR,0,&finished);
	if(addr.type == ARGVAL_STR) {
		bp->d.a.addr = symmap_getAddress(addr.d.str);
		if(bp->d.a.addr == 0)
			cmds_throwEx("Unable to find symbol '%s'\n",addr.d.str);
	}
	else
		bp->d.a.addr = addr.d.integer;
	bp->enabled = true;
	addrBreakpoints++;
}

static void addExprBreakpoint(sBreakpoint *bp,const sASTNode *expr,const sASTNode *val) {
	bp->type = BREAK_EXPR;
	bp->enabled = true;
	// clone expr because the argument will be free'd after the command
	bp->d.e.expr = ast_clone(expr);
	bp->d.e.hasValue = val != NULL;
	bp->d.e.waitForChange = false;
	// evaluate the expression and store the result (may be a range)
	bp->d.e.size = 8;
	bp->d.e.values = (octa*)mem_alloc(bp->d.e.size * sizeof(octa));
	for(octa offset = 0; ; offset++) {
		bool finished = true;
		sCmdArg eval = eval_get(val ? val : bp->d.e.expr,offset,&finished);
		if(finished)
			break;
		if(offset >= bp->d.e.size) {
			bp->d.e.size *= 2;
			bp->d.e.values = (octa*)mem_realloc(bp->d.e.values,bp->d.e.size * sizeof(octa));
		}
		// ignore expressions that have no int-value
		octa ival = eval.type == ARGVAL_INT ? eval.d.integer : 0;
		bp->d.e.values[offset] = ival;
	}
	exprBreakpoints++;
}

static void translationCallback(const sEvArgs *args) {
	if(addrBreakpoints == 0 || ignoreInstr)
		return;

	octa addr = args->arg1;
	int mode = args->arg2;
	char type = 'M';
	octa breakAddr = addr;
	bool doBreak = false;
	for(size_t i = 0; i < MAX_BREAKS; i++) {
		sBreakpoint *bp = breakpoints + i;
		if(!bp->enabled)
			continue;

		if(bp->type == BREAK_VIRTUAL) {
			if(bp->d.a.addr == addr && (bp->d.a.mode & mode)) {
				doBreak = true;
				break;
			}
		}
		else if(bp->type == BREAK_PHYSICAL) {
			if(bp->d.a.mode & mode) {
				octa phys = mmu_translate(addr,mode,mode,false);
				if(bp->d.a.addr == phys) {
					doBreak = true;
					breakAddr = phys;
					type = 'm';
					break;
				}
			}
		}
	}

	if(doBreak) {
		// when repeating this instruction, don't break again
		ignoreInstr = true;
		mprintf("Breakpoint reached @ #%016OX: %c[#%016OX], access for ",cpu_getPC(),type,breakAddr);
		printMode(mode);
		mprintf("\n");
		if(mode & (MEM_WRITE | MEM_READ)) {
			tetra curInstr = cpu_getCurInstrRaw();
			if(OPCODE(curInstr) == SAVE || OPCODE(curInstr) == UNSAVE) {
				/* FIXME: currently, SAVE and UNSAVE are not really interruptible, but check in
				 * advance whether they will succeed. This works well for the simulator itself,
				 * but if the CLI comes into play and sets read or write-breakpoints we have a
				 * problem. Because these might, by default, interrupt these instructions, which
				 * is not possible, in general. Therefore, we use this temporary hack to break
				 * after the execution of the instruction instead of inbetween. this is not really
				 * nice from the user-perspective, because its violates the philosophy that all
				 * address-breakpoints break _before_ the execution and all expression-breakpoints
				 * break _after_ the execution. (the reason for that is that everybody is used to
				 * the behaviour that ordinary breakpoints cause the machine to break _before_
				 * the instruction at the breakpoint-address has been executed) */
				delayedBreak = true;
				return;
			}
		}
		ex_throw(EX_BREAKPOINT,0);
	}
}

static void afterExecCallback(const sEvArgs *args) {
	UNUSED(args);
	ignoreInstr = false;
	if(delayedBreak) {
		delayedBreak = false;
		cpu_pause();
	}
	if(exprBreakpoints == 0)
		return;

	for(size_t i = 0; i < MAX_BREAKS; i++) {
		sBreakpoint *bp = breakpoints + i;
		if(!bp->enabled || bp->type != BREAK_EXPR)
			continue;

		bool changed = bp->d.e.hasValue;
		for(octa offset = 0; ; offset++) {
			bool finished = true;
			sCmdArg val = eval_get(bp->d.e.expr,offset,&finished);
			if(finished)
				break;
			octa ival = val.type == ARGVAL_INT ? val.d.integer : 0;
			if(!bp->d.e.hasValue) {
				changed |= ival != bp->d.e.values[offset];
				bp->d.e.values[offset] = ival;
			}
			else if(bp->d.e.values[offset] != ival) {
				changed = false;
				break;
			}
		}

		if(changed) {
			if(!bp->d.e.hasValue || !bp->d.e.waitForChange) {
				ast_toString(bp->d.e.expr,exprStr,MAX_EXPR_LEN);
				if(bp->d.e.hasValue) {
					bp->d.e.waitForChange = true;
					mprintf("Breakpoint reached @ #%016OX: %s has desired value\n",cpu_getPC(),exprStr);
				}
				else
					mprintf("Breakpoint reached @ #%016OX: value of %s changed\n",cpu_getPC(),exprStr);
				cpu_pause();
				break;
			}
		}
		else if(bp->d.e.waitForChange)
			bp->d.e.waitForChange = false;
	}
}

static int decodeMode(const char *mode) {
	int imode = 0;
	while(*mode) {
		switch(*mode) {
			case 'r':
				imode |= MEM_READ;
				break;
			case 'w':
				imode |= MEM_WRITE;
				break;
			case 'x':
				imode |= MEM_EXEC;
				break;
			default:
				cmds_throwEx("Invalid mode-flag %c\n",*mode);
				break;
		}
		mode++;
	}
	return imode;
}

static void resetBreakpoint(sBreakpoint *bp) {
	bp->enabled = false;
	if(bp->type == BREAK_EXPR) {
		ast_destroy(bp->d.e.expr);
		mem_free(bp->d.e.values);
		exprBreakpoints--;
	}
	else
		addrBreakpoints--;
}

static void printBreakpoints(void) {
	for(size_t i = 0; i < MAX_BREAKS; i++)
		printBreakpoint(i,breakpoints + i);
}

static void printBreakpoint(size_t index,sBreakpoint *bp) {
	if(bp->enabled) {
		mprintf("%02d:    %c",index,bp->type);
		switch(bp->type) {
			case BREAK_VIRTUAL:
			case BREAK_PHYSICAL:
				mprintf(" ");
				printMode(bp->d.a.mode);
				mprintf(" #%016OX\n",bp->d.a.addr);
				break;
			case BREAK_EXPR:
				ast_toString(bp->d.e.expr,exprStr,MAX_EXPR_LEN);
				if(!bp->d.e.hasValue)
					mprintf(" %s\n",exprStr);
				else {
					mprintf(" %s = #%OX",exprStr,bp->d.e.values[0]);
					if(bp->d.e.size > 8)
						mprintf("..");
					mprintf("\n");
				}
				break;
		}
	}
}

static void printMode(int mode) {
	mprintf("%c%c%c",
		(mode & MEM_READ) ? 'r' : '-',
		(mode & MEM_WRITE) ? 'w' : '-',
		(mode & MEM_EXEC) ? 'x' : '-');
}

static sBreakpoint *newBreakpoint(void) {
	for(size_t i = 0; i < MAX_BREAKS; i++) {
		if(!breakpoints[i].enabled)
			return breakpoints + i;
	}
	return NULL;
}
