/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <common.h>
#include <mem/pmem.h>
#include <mem/kheap.h>
#include <mem/region.h>
#include <string.h>
#include <assert.h>
#include <video.h>

/**
 * The region-module implements the abstraction 'region' which is simply a group of pages that
 * have common properties. So for example 'text' is a group, 'read-only-data', 'data', 'stack'
 * and so on. A region has no address and can be shared (if RF_SHAREABLE is set) by multiple
 * processes (and of course the virtual address to which it is mapped may be different).
 * A region has some flags that specify what operations are allowed and it has an array of
 * page-flags (flags for each page). So that each page can have different flags like copy-on-write,
 * swapped, demand-load or demand-zero.
 */

sRegion *reg_create(sBinDesc *bin,u32 binOffset,u32 bCount,u8 pgFlags,u32 flags) {
	u32 i,pageCount;
	sRegion *reg;
	assert(pgFlags == PF_DEMANDLOAD || pgFlags == PF_DEMANDZERO || pgFlags == 0);
	assert((flags & ~(RF_GROWABLE | RF_SHAREABLE | RF_WRITABLE | RF_STACK | RF_NOFREE)) == 0);

	reg = (sRegion*)kheap_alloc(sizeof(sRegion));
	if(reg == NULL)
		return NULL;
	reg->procs = sll_create();
	if(reg->procs == NULL)
		goto errReg;
	if(bin != NULL && bin->path) {
		reg->binary.path = strdup(bin->path);
		if(reg->binary.path == NULL)
			goto errPDirs;
		reg->binary.modifytime = bin->modifytime;
		reg->binOffset = binOffset;
	}
	else {
		reg->binary.path = NULL;
		reg->binary.modifytime = 0;
		reg->binOffset = 0;
	}
	reg->flags = flags;
	reg->byteCount = bCount;
	pageCount = BYTES_2_PAGES(bCount);
	reg->pfSize = pageCount;
	reg->pageFlags = (u8*)kheap_alloc(pageCount);
	if(reg->pageFlags == NULL)
		goto errPath;
	for(i = 0; i < pageCount; i++)
		reg->pageFlags[i] = pgFlags;
	return reg;

errPath:
	kheap_free((void*)reg->binary.path);
errPDirs:
	sll_destroy(reg->procs,false);
errReg:
	kheap_free(reg);
	return NULL;
}

void reg_destroy(sRegion *reg) {
	assert(reg != NULL);
	if(reg->binary.path)
		kheap_free((void*)reg->binary.path);
	kheap_free(reg->pageFlags);
	sll_destroy(reg->procs,false);
	kheap_free(reg);
}

u32 reg_presentPageCount(sRegion *reg) {
	u32 i,c = 0;
	assert(reg != NULL);
	for(i = 0; i < reg->pfSize; i++) {
		if((reg->pageFlags[i] & (PF_DEMANDLOAD | PF_DEMANDZERO | PF_LOADINPROGRESS)) == 0)
			c++;
	}
	return c;
}

u32 reg_refCount(sRegion *reg) {
	assert(reg != NULL);
	return sll_length(reg->procs);
}

bool reg_addTo(sRegion *reg,const void *p) {
	assert(reg != NULL);
	assert(sll_length(reg->procs) == 0 || (reg->flags & RF_SHAREABLE));
	return sll_append(reg->procs,(void*)p);
}

bool reg_remFrom(sRegion *reg,const void *p) {
	assert(reg != NULL);
	return sll_removeFirst(reg->procs,(void*)p);
}

bool reg_grow(sRegion *reg,s32 amount) {
	u32 count = BYTES_2_PAGES(reg->byteCount);
	assert(reg != NULL && (reg->flags & RF_GROWABLE));
	if(amount > 0) {
		s32 i;
		u8 *pf = (u8*)kheap_realloc(reg->pageFlags,reg->pfSize + amount);
		if(pf == NULL)
			return false;
		reg->pfSize += amount;
		/* stack grows downwards */
		if(reg->flags & RF_STACK) {
			memmove(pf + amount,pf,count);
			for(i = 0; i < amount; i++)
				pf[i] = 0;
		}
		else {
			for(i = 0; i < amount; i++)
				pf[i + count] = 0;
		}
		reg->pageFlags = pf;
		reg->byteCount += amount * PAGE_SIZE;
	}
	else {
		if(reg->byteCount < (u32)-amount * PAGE_SIZE)
			return false;
		if(reg->flags & RF_STACK)
			memmove(reg->pageFlags,reg->pageFlags + -amount,count + amount);
		reg->byteCount -= -amount * PAGE_SIZE;
	}
	return true;
}

sRegion *reg_clone(const void *p,sRegion *reg) {
	sRegion *clone;
	assert(reg != NULL && !(reg->flags & RF_SHAREABLE));
	clone = reg_create(&reg->binary,reg->binOffset,reg->byteCount,0,reg->flags);
	if(clone) {
		memcpy(clone->pageFlags,reg->pageFlags,reg->pfSize);
		reg_addTo(clone,p);
	}
	return clone;
}

void reg_sprintf(sStringBuffer *buf,sRegion *reg,u32 virt) {
	u32 i,x;
	sSLNode *n;
	struct {
		const char *name;
		u32 no;
	} flagNames[] = {
		{"Growable",RF_GROWABLE},
		{"Shareable",RF_SHAREABLE},
		{"Writable",RF_WRITABLE},
		{"Stack",RF_STACK},
		{"NoFree",RF_NOFREE}
	};
	prf_sprintf(buf,"\tSize: %u bytes\n",reg->byteCount);
	prf_sprintf(buf,"\tflags: ");
	for(i = 0; i < ARRAY_SIZE(flagNames); i++) {
		if(reg->flags & flagNames[i].no)
			prf_sprintf(buf,"%s ",flagNames[i].name);
	}
	prf_sprintf(buf,"\n");
	if(reg->binary.path) {
		prf_sprintf(buf,"\tbinary: path=%s modified=%u offset=%#x\n",
				reg->binary.path ? reg->binary.path : "NULL",reg->binary.modifytime,reg->binOffset);
	}
	prf_sprintf(buf,"\tProcesses: ");
	for(n = sll_begin(reg->procs); n != NULL; n = n->next)
		prf_sprintf(buf,"%d ",((sProc*)n->data)->pid);
	prf_sprintf(buf,"\n");
	prf_sprintf(buf,"\tPages (%d):\n",reg->pfSize);
	for(i = 0, x = BYTES_2_PAGES(reg->byteCount); i < x; i++) {
		prf_sprintf(buf,"\t\t%d: (%#08x) %c%c%c\n",i,virt + i * PAGE_SIZE,
				reg->pageFlags[i] & PF_COPYONWRITE ? 'c' : '-',
				reg->pageFlags[i] & PF_DEMANDLOAD ? 'l' : '-',
				reg->pageFlags[i] & PF_DEMANDZERO ? 'z' : '-',
				reg->pageFlags[i] & PF_SWAPPED ? 's' : '-');
	}
}


#if DEBUGGING

void reg_dbg_print(sRegion *reg,u32 virt) {
	sStringBuffer buf;
	buf.dynamic = true;
	buf.len = 0;
	buf.size = 0;
	buf.str = NULL;
	reg_sprintf(&buf,reg,virt);
	vid_printf("%s",buf.str);
	kheap_free(buf.str);
}

#endif
