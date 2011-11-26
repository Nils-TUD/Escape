/**
 * $Id: mmu.c 235 2011-06-16 08:37:02Z nasmussen $
 */

#include <stdlib.h>

#include "common.h"
#include "core/mmu.h"
#include "core/bus.h"
#include "core/cache.h"
#include "core/tc.h"
#include "core/register.h"
#include "core/cpu.h"
#include "exception.h"
#include "event.h"

#define MMU_EX_OFFSET	37

static octa oldrV = -1;
static sVirtTransReg vtr;

static octa mmu_doRead(octa addr,int mode,int flags);
static void mmu_doWrite(octa addr,octa value,int flags);
static void mmu_handleMemEx(int ex,octa addr,octa value,int flags);

byte mmu_readByte(octa addr,int flags) {
	int off = (addr & (sizeof(octa) - 1)) >> 0;
	octa data = mmu_doRead(addr,MEM_READ,flags);
	return (data >> (8 * (7 - off))) & 0xFF;
}

wyde mmu_readWyde(octa addr,int flags) {
	// note: the shift aligns it to a multiple of 2
	int off = (addr & (sizeof(octa) - 1)) >> 1;
	octa data = mmu_doRead(addr,MEM_READ,flags);
	return (data >> (16 * (3 - off))) & 0xFFFF;
}

tetra mmu_readTetra(octa addr,int flags) {
	int off = (addr & (sizeof(octa) - 1)) >> 2;
	octa data = mmu_doRead(addr,MEM_READ,flags);
	return (data >> (32 * (1 - off))) & 0xFFFFFFFF;
}

tetra mmu_readInstr(octa addr,int flags) {
	int off = (addr & (sizeof(octa) - 1)) >> 2;
	octa data = mmu_doRead(addr,MEM_EXEC,flags);
	return (data >> (32 * (1 - off))) & 0xFFFFFFFF;
}

octa mmu_readOcta(octa addr,int flags) {
	return mmu_doRead(addr,MEM_READ,flags);
}

static octa mmu_doRead(octa addr,int mode,int flags) {
	octa res;
	jmp_buf env;
	int ex = setjmp(env);
	if(ex != EX_NONE) {
		mmu_handleMemEx(ex,addr,0,flags);
		// loads that cause an exception, load zero
		res = 0;
	}
	else {
		ex_push(&env);
		int exp = (mode & MEM_READ) ? MEM_READ : MEM_EXEC;
		int cache = (mode & MEM_READ) ? CACHE_DATA : CACHE_INSTR;
		octa phys = mmu_translate(addr,mode,exp,flags & MEM_SIDE_EFFECTS);
		res = cache_read(cache,phys,flags);
	}
	ex_pop();
	return res;
}

void mmu_writeByte(octa addr,byte value,int flags) {
	// if the read throws an exception, mmmix triggers a read- and write-exception
	octa current = mmu_doRead(addr,MEM_READ | MEM_WRITE,flags);
	int off = (addr & (sizeof(octa) - 1)) >> 0;
	int shift = 8 * (7 - off);
	octa newVal = (current & ~((octa)0xFF << shift)) | ((octa)value << shift);
	mmu_doWrite(addr,newVal,flags);
}

void mmu_writeWyde(octa addr,wyde value,int flags) {
	octa current = mmu_doRead(addr,MEM_READ | MEM_WRITE,flags);
	int off = (addr & (sizeof(octa) - 1)) >> 1;
	int shift = 16 * (3 - off);
	octa newVal = (current & ~((octa)0xFFFF << shift)) | ((octa)value << shift);
	mmu_doWrite(addr,newVal,flags);
}

void mmu_writeTetra(octa addr,tetra value,int flags) {
	octa current = mmu_doRead(addr,MEM_READ | MEM_WRITE,flags);
	int off = (addr & (sizeof(octa) - 1)) >> 2;
	int shift = 32 * (1 - off);
	octa newVal = (current & ~((octa)0xFFFFFFFF << shift)) | ((octa)value << shift);
	mmu_doWrite(addr,newVal,flags);
}

void mmu_writeOcta(octa addr,octa value,int flags) {
	mmu_doWrite(addr,value,flags);
}

static void mmu_doWrite(octa addr,octa value,int flags) {
	jmp_buf env;
	int ex = setjmp(env);
	if(ex != EX_NONE) {
		mmu_handleMemEx(ex,addr,value,flags);
		// stores that cause an exception, store nothing
	}
	else {
		ex_push(&env);
		octa phys = mmu_translate(addr,MEM_WRITE,MEM_WRITE,flags & MEM_SIDE_EFFECTS);
		cache_write(CACHE_DATA,phys,value,flags);
	}
	ex_pop();
}

void mmu_syncdOcta(octa addr,bool radical) {
	jmp_buf env;
	int ex = setjmp(env);
	// protection-exceptions are ignored here
	if(ex != EX_NONE) {
		if(ex == EX_BREAKPOINT) {
			ex_pop();
			ex_rethrow();
		}
		mmu_handleMemEx(ex,addr,0,MEM_SIDE_EFFECTS | MEM_IGNORE_PROT_EX);
	}
	else {
		ex_push(&env);
		octa phys = mmu_translate(addr,MEM_WRITE,MEM_WRITE,true);
		cache_flush(CACHE_DATA,phys);
		if(radical)
			cache_remove(CACHE_DATA,phys);
	}
	ex_pop();
}

void mmu_syncidOcta(octa addr,bool radical) {
	jmp_buf env;
	int ex = setjmp(env);
	// protection-exceptions are ignored here
	if(ex != EX_NONE) {
		if(ex == EX_BREAKPOINT) {
			ex_pop();
			ex_rethrow();
		}
		mmu_handleMemEx(ex,addr,0,MEM_SIDE_EFFECTS | MEM_IGNORE_PROT_EX);
	}
	else {
		ex_push(&env);
		// note that we can assume that the mapping in ITC is the same as in the DTC because
		// if not, it would mean that at least one of them is not up-to-date, which is considered
		// an error
		octa phys = mmu_translate(addr,MEM_WRITE,MEM_WRITE,true);
		cache_remove(CACHE_INSTR,phys);
		if(radical)
			cache_remove(CACHE_DATA,phys);
		else
			cache_flush(CACHE_DATA,phys);
	}
	ex_pop();
}

sVirtTransReg mmu_unpackrV(octa rv) {
	sVirtTransReg v;
	v.b[0] = 0;
	for(int i = 0; i < 4; i++)
		v.b[i + 1] = (rv >> (60 - i * 4)) & 0xF;
	v.s = (rv >> 40) & 0xFF;
	v.n = (rv >> 3) & 0x3FF;
	v.r = rv & 0xFFFFFFE000;
	v.f = rv & 0x7;
	v.invalid = v.s < 13 || v.s > 48 || v.f > 1;
	return v;
}

octa mmu_translate(octa addr,int mode,int expected,bool sideEffects) {
	int tc = (mode & MEM_EXEC) ? TC_INSTR : TC_DATA;
	if(sideEffects)
		ev_fire2(EV_VAT,addr,mode);

	if(addr & MSB(64)) {
		if((addr & MSB(64)) && !cpu_isPriv())
			ex_throw(EX_DYNAMIC_TRAP,TRAP_PRIVILEGED_ACCESS);
		return addr & ~MSB(64);
	}

	// check values in rV
	octa rv = reg_getSpecial(rV);
	if(rv != oldrV) {
		vtr = mmu_unpackrV(rv);
		oldrV = rv;
	}
	if(vtr.invalid)
		ex_throw(EX_DYNAMIC_TRAP,(octa)mode << MMU_EX_OFFSET);

	// first look in TC
	octa key = 0;
	sTCEntry *tce = NULL;
	octa pte = 0;
	int segment = addr >> 61;
	if(sideEffects) {
		key = tc_addrToKey(addr,vtr.s,vtr.n);
		tce = tc_getByKey(tc,key);
	}

	addr &= 0x1FFFFFFFFFFFFFFF;
	if(tce == NULL) {
		// throw an exception if software-translation is desired
		// don't do that when no side-effects are desired
		if(sideEffects && vtr.f == 1)
			ex_throw(EX_FORCED_TRAP,TRAP_SOFT_TRANS | ((octa)mode << MMU_EX_OFFSET));

		// translate and store in TC
		octa pageNo = addr >> vtr.s;
		// check whether this address is in the bounds of the segment
		octa limit = (octa)1 << 10 * (vtr.b[segment + 1] - vtr.b[segment]);
		if(vtr.b[segment + 1] < vtr.b[segment] || pageNo >= limit)
			ex_throw(EX_DYNAMIC_TRAP,TRAP_REPEAT | (octa)mode << MMU_EX_OFFSET);

		// determine level
		int j = 0;
		while(pageNo >= 1024) {
			j++;
			pageNo /= 1024;
		}
		// resolve PTPs
		pageNo = addr >> vtr.s;
		octa c = (vtr.r >> 13) + vtr.b[segment] + j;
		int memflags = sideEffects ? MEM_SIDE_EFFECTS : 0;
		for(; j > 0; j--) {
			octa ax = (pageNo >> (10 * j)) & 0x3FF;
			octa ptp = cache_read(CACHE_DATA,(c << 13) + (ax << 3),memflags);
			if(!(ptp & MSB(64)) || (int)((ptp >> 3) & 0x3FF) != vtr.n)
				ex_throw(EX_DYNAMIC_TRAP,TRAP_REPEAT | (octa)mode << MMU_EX_OFFSET);
			c = (ptp & ~MSB(64)) >> 13;
		}
		// fetch PTE
		octa a0 = pageNo & 0x3FF;
		pte = cache_read(CACHE_DATA,(c << 13) + (a0 << 3),memflags);
		if((int)((pte >> 3) & 0x3FF) != vtr.n)
			ex_throw(EX_DYNAMIC_TRAP,TRAP_REPEAT | (octa)mode << MMU_EX_OFFSET);
		pte = tc_pteToTrans(pte,vtr.s);
		// store into TC
		if(sideEffects)
			tc_set(tc,key,pte);
	}
	else
		pte = tce->translation;

	// check access permissions
	if(!(pte & expected))
		ex_throw(EX_DYNAMIC_TRAP,TRAP_REPEAT | ((octa)mode & ~pte) << MMU_EX_OFFSET);

	// PTE to physical address
	octa trans = pte & ~(octa)0x7;
	octa phys = (trans << 10) + (addr & (((octa)1 << vtr.s) - 1));
	return phys;
}

static void mmu_handleMemEx(int ex,octa addr,octa value,int flags) {
	octa bits = ex_getBits();
	if(!(flags & MEM_IGNORE_PROT_EX) || (bits & ~(TRAP_RWX_BITS | TRAP_REPEAT))) {
		if(cpu_setMemEx(ex,bits,addr,value,flags & MEM_SIDE_EFFECTS)) {
			ex_pop();
			ex_rethrow();
		}
	}
}
