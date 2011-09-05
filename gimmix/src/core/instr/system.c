/**
 * $Id: system.c 239 2011-08-30 18:28:06Z nasmussen $
 */

#define _POSIX_C_SOURCE 199309L
#include <time.h>

#include "common.h"
#include "core/cpu.h"
#include "core/decoder.h"
#include "core/instr/system.h"
#include "core/mmu.h"
#include "core/cache.h"
#include "core/register.h"
#include "core/tc.h"
#include "exception.h"

#define SYNC_DRAIN_PIPELINE		0
#define SYNC_DRAIN_STORES		1
#define SYNC_DRAIN_LOADS		2
#define SYNC_DRAIN_LDST			3
#define SYNC_POWERSAVE			4
#define SYNC_FLUSH_CACHES		5
#define SYNC_CLEAR_TCS			6
#define SYNC_CLEAR_CACHES		7

static void syncRange(octa addr,int length,bool syncInstr,bool radical);
static void loadRange(octa addr,int length,int flags,bool instr);

void cpu_instr_ldunc(const sInstrArgs *iargs) {
	octa value = mmu_readOcta(iargs->y,MEM_SIDE_EFFECTS | MEM_UNCACHED);
	reg_set(iargs->x,value);
}

void cpu_instr_stunc(const sInstrArgs *iargs) {
	mmu_writeOcta(iargs->y,iargs->x,MEM_SIDE_EFFECTS | MEM_UNCACHED);
}

void cpu_instr_preld(const sInstrArgs *iargs) {
	// ensure that the specified range is in cache
	loadRange(iargs->z,iargs->x,MEM_SIDE_EFFECTS | MEM_IGNORE_PROT_EX,false);
}

void cpu_instr_prego(const sInstrArgs *iargs) {
	// ensure that the specified range is in cache
	loadRange(iargs->z,iargs->x,MEM_SIDE_EFFECTS | MEM_IGNORE_PROT_EX,true);
}

void cpu_instr_prest(const sInstrArgs *iargs) {
	// ensure that the specified range is in cache; don't initialize it because we can assume that
	// it will be stored before its read
	loadRange(iargs->z,iargs->x,MEM_SIDE_EFFECTS | MEM_IGNORE_PROT_EX | MEM_UNINITIALIZED,false);
}

void cpu_instr_syncd(const sInstrArgs *iargs) {
	syncRange(iargs->z,iargs->x,false,cpu_isPriv());
}

void cpu_instr_syncid(const sInstrArgs *iargs) {
	syncRange(iargs->z,iargs->x,true,cpu_isPriv());
}

void cpu_instr_sync(const sInstrArgs *iargs) {
	if(!cpu_isPriv() && iargs->y > 3)
		ex_throw(EX_DYNAMIC_TRAP,TRAP_PRIVILEGED_INSTR);

	switch(iargs->y) {
		case SYNC_DRAIN_PIPELINE:
		case SYNC_DRAIN_STORES:
		case SYNC_DRAIN_LOADS:
		case SYNC_DRAIN_LDST:
			// not implemented
			break;

		case SYNC_POWERSAVE: {
			/*struct timespec ts;
			int missing = INSTRS_PER_TICK - cpu_getInstrsSinceTick();
			ts.tv_sec = 0;
			ts.tv_nsec = (1000 / INSTRS_PER_TICK) * missing;
			nanosleep(&ts,NULL);
			cpu_setInstrsSinceTick(INSTRS_PER_TICK - 1);
			reg_setSpecial(rC,reg_getSpecial(rC) + missing);*/
		}
		break;

		case SYNC_CLEAR_CACHES:
			cache_removeAll(CACHE_INSTR);
			cache_removeAll(CACHE_DATA);
			break;

		case SYNC_FLUSH_CACHES:
			cache_flushAll(CACHE_INSTR);
			cache_flushAll(CACHE_DATA);
			break;

		case SYNC_CLEAR_TCS:
			tc_removeAll(TC_INSTR);
			tc_removeAll(TC_DATA);
			break;

		default:
			ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
			break;
	}
}

static void syncRange(octa addr,int length,bool syncInstr,bool radical) {
	// obviously this is very inefficient because we translate the virtual address for every octa
	// instead of taking care of the pagesize. but its simpler and therefore ok for now
	octa end = addr + length + 1;
	addr &= -(octa)sizeof(octa);
	while(addr < end) {
		if(syncInstr)
			mmu_syncidOcta(addr,radical);
		else
			mmu_syncdOcta(addr,radical);
		addr += sizeof(octa);
	}
}

static void loadRange(octa addr,int length,int flags,bool instr) {
	octa end = addr + length + 1;
	addr &= instr ? -(octa)sizeof(tetra) : -(octa)sizeof(octa);
	while(addr < end) {
		// exceptions are ignored
		if(instr)
			mmu_readInstr(addr,flags);
		else
			mmu_readOcta(addr,flags);
		addr += instr ? sizeof(tetra) : sizeof(octa);
	}
}
