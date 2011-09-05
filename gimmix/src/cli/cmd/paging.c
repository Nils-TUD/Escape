/**
 * $Id: paging.c 239 2011-08-30 18:28:06Z nasmussen $
 */

#include <string.h>

#include "common.h"
#include "cli/cmds.h"
#include "cli/cmd/paging.h"
#include "cli/console.h"
#include "core/register.h"
#include "core/cpu.h"
#include "core/mmu.h"
#include "core/bus.h"
#include "core/tc.h"
#include "core/cache.h"
#include "event.h"
#include "exception.h"
#include "mmix/io.h"

/* parts of the code here are inspired from the VAT-tool of GIMMIX 0.0.2 */

static void translateAddr(octa addr);
static void showTCEntriesOf(int tc,const sASTNode *addrs);
static void showTCEntries(int tc);
static void printTCEntry(int i,sTCEntry *e);
static int getTCIndex(int tc,sTCEntry *entry);

static const char *tcNames[] = {
	/* TC_INSTR */	"Instruction TC",
	/* TC_DATA */	"Data TC"
};

void cli_cmd_v2p(size_t argc,const sASTNode **argv) {
	if(argc < 1)
		cmds_throwEx(NULL);
	sCmdArg addr = cmds_evalArg(argv[0],ARGVAL_INT,0,NULL);
	translateAddr(addr.d.integer);
}

void cli_cmd_itc(size_t argc,const sASTNode **argv) {
	if(argc > 1)
		cmds_throwEx(NULL);
	if(argc == 1)
		showTCEntriesOf(TC_INSTR,argv[0]);
	else
		showTCEntries(TC_INSTR);
}

void cli_cmd_dtc(size_t argc,const sASTNode **argv) {
	if(argc > 1)
		cmds_throwEx(NULL);
	if(argc == 1)
		showTCEntriesOf(TC_DATA,argv[0]);
	else
		showTCEntries(TC_DATA);
}

static void translateAddr(octa addr) {
	octa rv = reg_getSpecial(rV);
	const sVirtTransReg vtr = mmu_unpackrV(rv);

	mprintf("The virtual address #%016OX is translated as follows:\n",addr);
	if(addr & MSB(64)) {
		mprintf("MSB of virtual address set: address uses the directly mapped space\n");
		mprintf("ACCESS IS PRIVILEGED\n");
		addr &= ~MSB(64);
		mprintf("Physical address = #%016OX\n",addr);
		if(addr & 0x7fff000000000000)
			mprintf("This physical address accesses an I/O device\n");
		else
			mprintf("This physical address accesses main memory\n");
	}
	else {
		int a[5];
		int segment = (addr >> 61) & 0x3;
		mprintf("The segment number is %d\n",segment);
		addr &= 0x1fffffffffffffff;
		mprintf("The segment-relative virtual address is #%016OX\n",addr);
		octa mask = ((octa)1 << vtr.s) - 1;
		mprintf("The page mask is #%OX\n",mask);
		octa page = addr >> vtr.s;
		octa offset = addr & mask;
		mprintf("This leads to page #%OX, offset #%OX\n",page,offset);
		for(int i = 0; i < 5; i++)
			a[i] = (page >> (10 * i)) & 0x3ff;
		mprintf("Thus the page number in radix 1024 is ");
		mprintf("(#%03x #%03x #%03x #%03x #%03x)\n",a[4],a[3],a[2],a[1],a[0]);
		// check whether this address is in the bounds of the segment
		octa limit = (octa)1 << 10 * (vtr.b[segment + 1] - vtr.b[segment]);
		if(vtr.b[segment + 1] < vtr.b[segment] || page >= limit) {
			mprintf("EXCEPTION: page is out of segment-bounds: ");
			if(vtr.b[segment + 1] < vtr.b[segment])
				mprintf("-empty-");
			else {
				mprintf("#%016OX .. #%016OX",(octa)segment << 61,
						((octa)segment << 61) | ((limit << vtr.s) - 1));
			}
			mprintf("\n");
		}
		mprintf("Translation chain: VA --> ");
		int level = 0;
		for(int i = 4; i >= 1; i--) {
			if(a[i] != 0 || level > 0) {
				mprintf("PTP(%d) --> ",i);
				if(level == 0)
					level = i;
			}
		}
		mprintf("PTE\n");

		// interpret auxiliary PTP values
		octa base = (vtr.r >> 13) + vtr.b[segment] + level;
		for(; level > 0; level--) {
			octa ptp = cache_read(CACHE_DATA,(base << 13) + (a[level] << 3),0);
			mprintf("Fetching PTP%d from m[(#%08OX << 13) + (#%03X << 3)] = m[#%OX] = #%OX\n",
					level,base,a[level],(base << 13) + (a[level] << 3),ptp);
			if((int)((ptp >> 3) & 0x3FF) != vtr.n) {
				mprintf("EXCEPTION: n field of PTP%d (%d) does not match n field in rV (%d)\n",
						level,(int)(ptp >> 3) & 0x3FF,vtr.n);
			}
			base = (ptp & ~MSB(64)) >> 13;
		}

		// interpret PTE value
		octa pte = cache_read(CACHE_DATA,(base << 13) + (a[level] << 3),0);
		mprintf("Fetching PTE from m[(#%08OX << 13) + (#%03X << 3)] = m[#%OX] = #%OX\n",
				base,a[level],(base << 13) + (a[level] << 3),pte);
		if((int)((pte >> 3) & 0x3FF) != vtr.n) {
			mprintf("EXCEPTION: n field of PTE (%d) does not match n field in rV (%d)\n",
					(int)(pte >> 3) & 0x3FF,vtr.n);
		}
		mprintf("PTE has permissions %c%c%c\n",
				(pte & MEM_READ) ? 'r' : '-',
				(pte & MEM_WRITE) ? 'w' : '-',
				(pte & MEM_EXEC) ? 'x' : '-');
		// use 56-bit physical-address to include the I/O space
		octa pageOffset = (pte & 0xFFFFFFFFFFFFF8) & ~(((octa)1 << vtr.s) - 1);
		octa phys = pageOffset + (addr & (((octa)1 << vtr.s) - 1));
		mprintf("Physical page base address is #%OX\n",pageOffset);
		mprintf("Then, finally, the physical address is #%OX\n",phys);
	}
}

static void showTCEntriesOf(int tc,const sASTNode *addrs) {
	bool finished;
	octa rv = reg_getSpecial(rV);
	const sVirtTransReg vtr = mmu_unpackrV(rv);
	octa pageSize = 1 << vtr.s;
	for(octa offset = 0; ; offset += pageSize) {
		sCmdArg addr = cmds_evalArg(addrs,ARGVAL_INT,offset,&finished);
		if(finished)
			break;

		sTCEntry *e = tc_getByKey(tc,tc_addrToKey(addr.d.integer,vtr.s,vtr.n));
		if(e == NULL)
			mprintf("No entry found for address #%016OX in %s\n",addr.d.integer,tcNames[tc]);
		else {
			mprintf("%-2s %-16s %-3s %-3s %-16s %-14s %-3s\n",
					"no","key","seg","n","virtual","physical","prwx");
			printTCEntry(getTCIndex(tc,e),e);
		}
	}
}

static void showTCEntries(int tc) {
	mprintf("%-2s %-16s %-3s %-3s %-16s %-14s %-3s\n",
			"no","key","seg","n","virtual","physical","prwx");
	for(size_t i = 0; ; i++) {
		sTCEntry *e = tc_getByIndex(tc,i);
		if(e == NULL)
			break;
		printTCEntry(i,e);
	}
}

static void printTCEntry(int i,sTCEntry *e) {
	mprintf("%2d %016OX %3d %03OX %016OX %014OX %c%c%c%c\n",
			i,e->key,(int)((e->key >> 61) & 0x3),
			(e->key >> 3) & 0x3FF,e->key & 0x0FFFFFFFFFFFE000,
			(e->translation & ~(octa)0x7) << 10,
			(e->key & MSB(64)) ? '-' : 'p',
			(e->translation & MEM_READ) ? 'r' : '-',
			(e->translation & MEM_WRITE) ? 'w' : '-',
			(e->translation & MEM_EXEC) ? 'x' : '-');
}

static int getTCIndex(int tc,sTCEntry *entry) {
	for(size_t i = 0; ; i++) {
		sTCEntry *e = tc_getByIndex(tc,i);
		if(e == entry)
			return i;
	}
	return -1;
}
