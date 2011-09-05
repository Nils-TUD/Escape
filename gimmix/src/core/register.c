/**
 * $Id: register.c 227 2011-06-11 18:40:58Z nasmussen $
 */

#include <assert.h>
#include <string.h>

#include "common.h"
#include "../build/abstime.h"
#include "core/register.h"
#include "core/mmu.h"
#include "core/tc.h"
#include "core/cpu.h"
#include "mmix/io.h"
#include "mmix/error.h"
#include "config.h"
#include "exception.h"
#include "event.h"

static void reg_stackLoad(void);
static octa reg_stackLoadVal(int type,int index);
static void reg_stackStore(void);
static void reg_stackStoreVal(octa val,int type,int index);
static void reg_checkAccessibility(octa begin,octa end);

static octa local[LREG_NUM];
static octa global[256];	// always 256; GREG_NUM globals available starting at the top
static octa special[SPECIAL_NUM];
static int L;	// = special[rL]
static int G;	// = special[rG]
static octa S;	// = special[rS]/8: offset in register-ring of the next register to write to stack
static octa O;	// = special[rO]/8: offset of $0

void reg_init(void) {
	reg_reset();
}

void reg_reset(void) {
	for(size_t i = 0; i < ARRAY_SIZE(local); i++)
		local[i] = 0;
	for(size_t i = 0; i < ARRAY_SIZE(global); i++)
		global[i] = 0;
	for(size_t i = 0; i < ARRAY_SIZE(special); i++)
		special[i] = 0;
	special[rN] = ABSTIME | ((octa)VERSION << 56) | ((octa)SUBVERSION << 48) |
			((octa)SUBSUBVERSION << 40);
	special[rG] = G = 255;
	special[rL] = L = 0;
	special[rS] = S = 0;
	special[rO] = O = 0;
}

octa reg_getLocal(int rno) {
	return local[rno & LREG_MASK];
}

void reg_setLocal(int rno,octa value) {
	local[rno & LREG_MASK] = value;
}

octa reg_getGlobal(int rno) {
	assert(rno >= MAX(256 - GREG_NUM,MIN_GLOBAL));
	return global[rno & GREG_MASK];
}

void reg_setGlobal(int rno,octa value) {
	assert(rno >= MAX(256 - GREG_NUM,MIN_GLOBAL));
	global[rno & GREG_MASK] = value;
}

octa reg_getSpecial(int rno) {
	assert(rno < SPECIAL_NUM);
	return special[rno];
}

void reg_setSpecial(int rno,octa value) {
	assert(rno < SPECIAL_NUM);
	special[rno] = value;
	// update our shortcuts
	if(rno == rG)
		G = value;
	else if(rno == rL)
		L = value;
	else if(rno == rS)
		S = value / sizeof(octa);
	else if(rno == rO)
		O = value / sizeof(octa);
}

octa reg_get(int rno) {
	if(rno >= G)
		return global[rno & GREG_MASK];
	if(rno < L)
		return local[(O + rno) & LREG_MASK];
	return 0;
}

void reg_set(int rno,octa value) {
	if(rno >= G)
		global[rno & GREG_MASK] = value;
	else {
		// note: if reg_stackStore throws an exception, we might have changed our state but in
		// this case, its ok. Because after the exception-handling we will re-execute this
		// instruction. The first few stores may have already been done and will therefore not
		// be done again. So we'll continue at the point at which we interrupted.

		// increase L and zero registers, if necessary
		while(rno >= L) {
			local[(O + L) & LREG_MASK] = 0;
			// if all slots are in use, store one on the stack
			if(((S - O - (L + 1)) & LREG_MASK) == 0)
				reg_stackStore();
			// important: change L afterwards since reg_stackStore may throw an exception!
			special[rL] = L = L + 1;
		}
		local[(O + rno) & LREG_MASK] = value;
	}
}

void reg_push(int rno) {
	// don't change L because reg_stackStore may throw an exception
	int curL = L;

	// if we want to save all local regs we need to increase L and rno by one and we may have to
	// store a value on the stack
	if(rno >= G) {
		rno = curL++;
		if(((S - O - curL) & LREG_MASK) == 0)
			reg_stackStore();
		// do it here manually because L may be equal to G, so that reg_set would set a global reg
		local[(O + rno) & LREG_MASK] = rno;
	}
	// set register rno to rno (the "hole" that stores how many we've pushed down)
	else {
		reg_set(rno,rno);
		curL = L;
	}

	// push down
	O += rno + 1;
	special[rO] += (rno + 1) * sizeof(octa);
	// move L down (keep arguments)
	L = curL - (rno + 1);
	special[rL] = L;
}

void reg_pop(int rno) {
	// the last return-value goes into the hole
	octa holeData;
	if(rno != 0 && rno <= L)
		holeData = local[(O + rno - 1) & LREG_MASK];
	// if rno > L, we set the hole to 0 (this implies rno > 0)
	else
		holeData = 0;

	int numRets = rno <= L ? rno : L + 1;
	// get number of regs that the caller wanted to keep (load from stack, if necessary)
	if(special[rS] == special[rO]) {
		// if this load throws an exception, our state has not changed
		reg_stackLoad();
		// we have to decrease rL here to ensure a consistent state if reg_stackLoad() throws.
		// otherwise this value could be overwritten if, say, the OS decides not to resume the
		// ordinary execution after handling the exception, but call a signal-handler, i.e. use
		// the stack for other things before retrying the pop.
		// the following keeps the condition for alpha, beta and gamma valid
		if(((S - O - L) & LREG_MASK) == 0)
			special[rL] = --L;
	}
	int numRegs = local[(O - 1) & LREG_MASK] & 0xFF;

	// restore regs from stack, if necessary
	// this load may also throw an exception, because we'll continue at that point afterwards.
	while((tetra)(O - S) <= (tetra)numRegs) {
		reg_stackLoad();
		// we have to decrease rL here to ensure a consistent state if reg_stackLoad() throws
		if(((S - O - L) & LREG_MASK) == 0)
			special[rL] = --L;
	}

	// set new L (number of regs to restore + number of return-values)
	L = numRegs + numRets;
	if(L > G)
		L = G;

	// if we have any return-value, set the hole
	if(L > numRegs)
		local[(O - 1) & LREG_MASK] = holeData;

	// adjust stack-offset and sync rL
	O -= numRegs + 1;
	special[rO] -= (numRegs + 1) * sizeof(octa);
	special[rL] = L;
}

void reg_save(int dst,bool changeStack) {
	assert(dst >= G);
	octa oldrS = special[rS];
	octa oldrO = special[rO];

	// don't change the stack if its already in privileged space
	bool doChangeStack = changeStack && !(oldrS & MSB(64));

	// check if the memory range we're going to write to is accessible; don't do that when changing
	// stack because rSS is always in privileged space. thus, no mapping happens and errors like
	// unaccessible physical memory are ignored, because it would indicate a serious error in the OS
	if(!doChangeStack) {
		octa begin = special[rS];
		// locals, rL, 13 special and the globals
		octa end = (special[rO] + L * sizeof(octa)) + (1 + 13 + (256 - G)) * sizeof(octa);
		reg_checkAccessibility(begin,end);
	}

	// push all local registers down
	int oldL = L;
	O += L;
	special[rO] += L * sizeof(octa);

	// if desired, change stack to rSS
	if(doChangeStack) {
		// setup rO so that all not yet pushed-down and the local registers are on the stack
		octa newrO = special[rSS] + L * sizeof(octa) + (oldrO - oldrS);
		reg_setSpecial(rO,newrO);
		// don't set S here because it is used as index of local when storing
		// but of course, we have to set rS because we want to write to the new stack
		special[rS] = special[rSS];
	}

	special[rL] = L = 0;

	// store all registers on the stack that have been pushed down but not yet stored
	while(special[rO] != special[rS])
		reg_stackStore();
	// last but not least, store the old L
	reg_stackStoreVal(oldL,RSTACK_DEFAULT,S & LREG_MASK);

	// when changing stack, save rO and rS first
	if(doChangeStack) {
		reg_stackStoreVal(oldrO,RSTACK_SPECIAL,rO);
		reg_stackStoreVal(oldrO + (oldL + 1) * sizeof(octa),RSTACK_SPECIAL,rS);
	}

	// save g[rG] .. g[255], then save the special-registers rB, rD, rE, rH, rJ, rM, rR, rP,
	// rW, rX, rY and rZ. finally put rG and rA in one octa
	for(int k = G;;) {
		octa value;
		if(k == rZ + 1) {
			value = ((octa)G << 56) | (special[rA] & 0xFFFFFFFF);
			// tell UNSAVE whether we've changed the stack
			if(changeStack && doChangeStack)
				value |= (octa)1 << 32;
			reg_stackStoreVal(value,RSTACK_RGRA,0);
		}
		else if(k < SPECIAL_NUM)
			reg_stackStoreVal(special[k],RSTACK_SPECIAL,k);
		else
			reg_stackStoreVal(global[k],RSTACK_GREGS,k);

		if(k == 255)
			k = rB;
		else if(k == rR)
			k = rP;
		else if(k == rZ + 1)
			break;
		else
			k++;
	}

	// if we've changed the stack, set the new S
	if(doChangeStack)
		S = special[rS] / sizeof(octa);
	// finally set rO and return the address of the last value stored on the stack
	O = S;
	special[rO] = special[rS];
	reg_set(dst,special[rO] - sizeof(octa));
}

void reg_unsave(octa src,bool changeStack) {
	// octa-align it
	src &= ~(octa)(sizeof(octa) - 1);

	// first load the first value from stack and check if it fullfills the requirements of
	// rA and rG. this way we haven't changed anything if an exception is thrown
	octa rGrA = mmu_readOcta(src,MEM_SIDE_EFFECTS);
	// note that G may be less than the current L because L will be loaded from the stack
	if((rGrA >> 56) < MIN_GLOBAL)
		ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
	if(((rGrA) & 0xFFFFFF) & ~(octa)0x3FFFF)
		ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
	// don't change the stack back if it hasn't been changed by SAVE
	if(changeStack && !(rGrA & ((octa)1 << 32)))
		changeStack = false;

	// check if the memory range we're going to read from is accessible
	if(!changeStack) {
		int nG = (rGrA >> 56) & 0xFF;
		// this read may fail as well, but we haven't changed the state yet
		int nL = mmu_readOcta(src - (13 + 256 - nG) * sizeof(octa),MEM_SIDE_EFFECTS) & 0xFF;
		octa end = src + sizeof(octa);
		octa begin = end - (nL + 1 + 13 + (256 - nG)) * sizeof(octa);
		reg_checkAccessibility(begin,end);
	}

	// set new stackpointer
	reg_setSpecial(rS,src + sizeof(octa));

	// now restore all registers in reverse order: start with rA and rG, then rZ, rY, rX, rW, rP,
	// rR, rM, rJ, rH, rE, rD, rB. finally restore g[255] .. g[rG]
	for(int k = rZ + 1;;) {
		if(k == rZ + 1) {
			octa value = reg_stackLoadVal(RSTACK_RGRA,0);
			special[rG] = G = value >> 56;
			special[rA] = value & 0x3FFFF;
		}
		else if(k < SPECIAL_NUM)
			special[k] = reg_stackLoadVal(RSTACK_SPECIAL,k);
		else
			global[k] = reg_stackLoadVal(RSTACK_GREGS,k);

		if(k == rP)
			k = rR;
		else if(k == rB)
			k = 255;
		else if(k == G)
			break;
		else
			k--;
	}

	// when changing stack, restore rS and rO
	if(changeStack) {
		octa newrS = reg_stackLoadVal(RSTACK_SPECIAL,rS);
		octa newrO = reg_stackLoadVal(RSTACK_SPECIAL,rO);
		reg_setSpecial(rO,newrO);
		// now set the new S to put the locals into the correct slots, if we're changing the stack
		S = newrS / sizeof(octa);
	}

	// load rL (has been stored as last local register)
	reg_stackLoad();
	// load $0 .. $(L - 1)
	int k = local[S & LREG_MASK] & 0xFF;
	for(int j = 0; j < k; j++)
		reg_stackLoad();

	// finally restore rO, rL and rG
	if(changeStack) {
		// we have to load the pushed-down values back from the kernel-stack into the registers
		while(special[rS] != special[rSS])
			reg_stackLoad();
		special[rS] = S * sizeof(octa);
	}
	else {
		O = S;
		special[rO] = special[rS];
	}
	L = k > G ? G : k;
	special[rL] = L;
	special[rG] = G;
}

static void reg_stackLoad(void) {
	// note: we can't directly assign the return-value here because S is changed by the function!
	octa value = reg_stackLoadVal(RSTACK_DEFAULT,(S - 1) & LREG_MASK);
	local[S & LREG_MASK] = value;
}

static octa reg_stackLoadVal(int type,int index) {
	// note the order: mmu_readOcta may throw an exception!
	octa val = mmu_readOcta(special[rS] - sizeof(octa),MEM_SIDE_EFFECTS);
	S--;
	special[rS] -= sizeof(octa);
	ev_fire2(EV_STACKLOAD,type,index);
	return val;
}

static void reg_stackStore(void) {
	reg_stackStoreVal(local[S & LREG_MASK],RSTACK_DEFAULT,S & LREG_MASK);
}

static void reg_stackStoreVal(octa val,int type,int index) {
	mmu_writeOcta(special[rS],val,MEM_SIDE_EFFECTS);
	ev_fire2(EV_STACKSTORE,type,index);
	S++;
	special[rS] += sizeof(octa);
}

static void reg_checkAccessibility(octa begin,octa end) {
	// when in privileged space, its not necessary. if the physical memory is not accessible, we
	// are dead anyway.
	if(!(begin & MSB(64))) {
		octa pagesize = (octa)1 << ((special[rV] >> 40) & 0xFF);
		// if the pagesize is invalid, i.e 1, and rK is zero, mmu_readOcta won't trigger an exception
		// that means, we might loop for a long time here without any hope of success
		if((special[rK] & (TRAP_PROT_READ | TRAP_PROT_WRITE)) == 0 && (pagesize < 13 || pagesize > 48))
			return;
		for(begin &= -pagesize; begin < end; begin += pagesize) {
			octa val = mmu_readOcta(begin,MEM_SIDE_EFFECTS);
			mmu_writeOcta(begin,val,MEM_SIDE_EFFECTS);
		}
	}
}
