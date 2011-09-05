/**
 * $Id: stat.c 235 2011-06-16 08:37:02Z nasmussen $
 */

#include <string.h>

#include "common.h"
#include "cli/cmds.h"
#include "cli/cmd/stat.h"
#include "cli/lang/idesc.h"
#include "core/decoder.h"
#include "core/bus.h"
#include "core/cache.h"
#include "core/cpu.h"
#include "core/tc.h"
#include "core/mmu.h"
#include "core/register.h"
#include "mmix/io.h"
#include "exception.h"
#include "event.h"

static void enableEvents(void);
static void disableEvents(void);
static void afterExec(const sEvArgs *args);
static void virtAddrTrans(const sEvArgs *args);
static void cacheLookup(const sEvArgs *args);
static void exception(const sEvArgs *args);
static void devRead(const sEvArgs *args);
static void devWrite(const sEvArgs *args);

static int exNo = 0;

static long instrs = 0;
static long oops = 0;
static long mems = 0;
static long goodPred = 0;
static long badPred = 0;
static long icHits = 0;
static long icMisses = 0;
static long dcHits = 0;
static long dcMisses = 0;
static long itcHits = 0;
static long itcMisses = 0;
static long dtcHits = 0;
static long dtcMisses = 0;
static long memReads = 0;
static long memWrites = 0;
static long ioReads = 0;
static long ioWrites = 0;
static bool enabled = false;

void cli_cmd_statInit(void) {
	// enabled by default
	enableEvents();
}

void cli_cmd_statReset(void) {
	// only reset it if its enabled; don't enable it again if the user has disabled it
	if(enabled)
		enableEvents();
}

void cli_cmd_stat(size_t argc,const sASTNode **argv) {
	if(argc != 0 && argc != 1)
		cmds_throwEx(NULL);

	if(argc == 1) {
		bool finished;
		sCmdArg arg = cmds_evalArg(argv[0],ARGVAL_STR,0,&finished);
		if(strcmp(arg.d.str,"on") == 0)
			enableEvents();
		else if(strcmp(arg.d.str,"off") == 0)
			disableEvents();
		else
			cmds_throwEx(NULL);
	}
	else {
		mprintf("%ld instructions, %ld oops, %ld mems, %ld good guesses, %ld bad guesses\n",
				instrs,oops,mems,goodPred,badPred);
		mprintf("%ld memory reads, %ld memory writes\n",memReads,memWrites);
		mprintf("%ld IO reads, %ld IO writes\n",ioReads,ioWrites);
		double itcHitrate = itcHits + itcMisses == 0 ? 0 : 100 * ((double)itcHits / (itcHits + itcMisses));
		mprintf("%ld ITC hits, %ld ITC misses (%.2lf%% hits)\n",itcHits,itcMisses,itcHitrate);
		double dtcHitrate = dtcHits + dtcMisses == 0 ? 0 : 100 * ((double)dtcHits / (dtcHits + dtcMisses));
		mprintf("%ld DTC hits, %ld DTC misses (%.2lf%% hits)\n",dtcHits,dtcMisses,dtcHitrate);
		double icHitrate = icHits + icMisses == 0 ? 0 : 100 * ((double)icHits / (icHits + icMisses));
		mprintf("%ld IC hits, %ld IC misses (%.2lf%% hits)\n",icHits,icMisses,icHitrate);
		double dcHitrate = dcHits + dcMisses == 0 ? 0 : 100 * ((double)dcHits / (dcHits + dcMisses));
		mprintf("%ld DC hits, %ld DC misses (%.2lf%% hits)\n",dcHits,dcMisses,dcHitrate);
	}
}

static void enableEvents(void) {
	if(!enabled) {
		ev_register(EV_AFTER_EXEC,afterExec);
		ev_register(EV_VAT,virtAddrTrans);
		ev_register(EV_CACHE_LOOKUP,cacheLookup);
		ev_register(EV_EXCEPTION,exception);
		ev_register(EV_DEV_READ,devRead);
		ev_register(EV_DEV_WRITE,devWrite);
		enabled = true;
	}

	// reset
	instrs = 0;
	oops = 0;
	mems = 0;
	icHits = 0;
	icMisses = 0;
	dcHits = 0;
	dcMisses = 0;
	itcHits = 0;
	itcMisses = 0;
	dtcHits = 0;
	dtcMisses = 0;
	goodPred = 0;
	badPred = 0;
	memReads = 0;
	memWrites = 0;
	ioReads = 0;
	ioWrites = 0;
}

static void disableEvents(void) {
	if(!enabled)
		return;

	ev_unregister(EV_AFTER_EXEC,afterExec);
	ev_unregister(EV_VAT,virtAddrTrans);
	ev_unregister(EV_CACHE_LOOKUP,cacheLookup);
	ev_unregister(EV_EXCEPTION,exception);
	ev_unregister(EV_DEV_READ,devRead);
	ev_unregister(EV_DEV_WRITE,devWrite);
	enabled = false;
}

static void afterExec(const sEvArgs *args) {
	UNUSED(args);
	// if an exception occurred, we have not executed the instruction
	if(exNo != 0) {
		exNo = 0;
		return;
	}

	tetra raw = cpu_getCurInstrRaw();
	const sInstrDesc *idesc = idesc_get(OPCODE(raw));
	instrs++;
	oops += idesc->oops;
	mems += idesc->mems;

	if(OPCODE(raw) >= BN && OPCODE(raw) <= BEVB) {
		if(cpu_hasPCChanged())
			badPred++;
		else
			goodPred++;
	}
	else if(OPCODE(raw) >= PBN && OPCODE(raw) <= PBEVB) {
		if(cpu_hasPCChanged())
			goodPred++;
		else
			badPred++;
	}
}

static void virtAddrTrans(const sEvArgs *args) {
	int tc = args->arg2 == MEM_EXEC ? TC_INSTR : TC_DATA;
	if(!(args->arg1 & MSB(64))) {
		sVirtTransReg vtr = mmu_unpackrV(reg_getSpecial(rV));
		if(tc_getByKey(tc,tc_addrToKey(args->arg1,vtr.s,vtr.n)) != NULL) {
			if(tc == TC_INSTR)
				itcHits++;
			else
				dtcHits++;
		}
		else {
			if(tc == TC_INSTR)
				itcMisses++;
			else
				dtcMisses++;
		}
	}
}

static void cacheLookup(const sEvArgs *args) {
	int cache = args->arg1;
	if(cache_find(cache_getCache(cache),args->arg2) != NULL) {
		if(cache == CACHE_INSTR)
			icHits++;
		else
			dcHits++;
	}
	else {
		if(cache == CACHE_INSTR)
			icMisses++;
		else
			dcMisses++;
	}
}

static void exception(const sEvArgs *args) {
	UNUSED(args);
	exNo = ex_getEx();
}

static void devRead(const sEvArgs *args) {
	octa addr = args->arg1;
	if(addr >= DEV_START_ADDR)
		ioReads++;
	else
		memReads++;
}

static void devWrite(const sEvArgs *args) {
	octa addr = args->arg1;
	if(addr >= DEV_START_ADDR)
		ioWrites++;
	else
		memWrites++;
}
