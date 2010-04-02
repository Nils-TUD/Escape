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

sRegion *reg_create(sBinDesc *bin,u32 binOffset,u32 bCount,u8 pgFlags,u32 flags) {
	u32 i,pageCount;
	sRegion *reg;
	assert(pgFlags == PF_DEMANDLOAD || pgFlags == PF_DEMANDZERO || pgFlags == 0);
	assert((flags & ~(RF_GROWABLE | RF_SHAREABLE | RF_WRITABLE | RF_STACK)) == 0);

	reg = (sRegion*)kheap_alloc(sizeof(sRegion));
	if(reg == NULL)
		return NULL;
	reg->pdirs = sll_create();
	if(reg->pdirs == NULL)
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
	sll_destroy(reg->pdirs,false);
errReg:
	kheap_free(reg);
	return NULL;
}

void reg_destroy(sRegion *reg) {
	assert(reg != NULL);
	if(reg->binary.path)
		kheap_free((void*)reg->binary.path);
	kheap_free(reg->pageFlags);
	sll_destroy(reg->pdirs,false);
	kheap_free(reg);
}

u32 reg_refCount(sRegion *reg) {
	assert(reg != NULL);
	return sll_length(reg->pdirs);
}

bool reg_addTo(sRegion *reg,tPageDir pdir) {
	assert(reg != NULL);
	assert(sll_length(reg->pdirs) == 0 || (reg->flags & RF_SHAREABLE));
	return sll_append(reg->pdirs,(void*)pdir);
}

bool reg_remFrom(sRegion *reg,tPageDir pdir) {
	assert(reg != NULL);
	return sll_removeFirst(reg->pdirs,(void*)pdir);
}

bool reg_grow(sRegion *reg,s32 amount) {
	u32 count = BYTES_2_PAGES(reg->byteCount);
	assert(reg != NULL && (reg->flags & RF_GROWABLE));
	if(amount > 0) {
		s32 i;
		/* TODO perhaps *= 2? */
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

sRegion *reg_clone(tPageDir pdir,sRegion *reg) {
	sRegion *clone;
	assert(reg != NULL && !(reg->flags & RF_SHAREABLE));
	clone = reg_create(&reg->binary,reg->binOffset,reg->byteCount,0,reg->flags);
	if(clone) {
		memcpy(clone->pageFlags,reg->pageFlags,reg->pfSize);
		reg_addTo(clone,pdir);
	}
	return clone;
}


#if DEBUGGING

void reg_dbg_print(sRegion *reg) {
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
	};
	vid_printf("\t\tSize: %u bytes\n",reg->byteCount);
	vid_printf("\t\tflags: ");
	for(i = 0; i < ARRAY_SIZE(flagNames); i++) {
		if(reg->flags & flagNames[i].no)
			vid_printf("%s ",flagNames[i].name);
	}
	vid_printf("\n");
	vid_printf("\t\tbinary: path=%s modified=%u\n",
			reg->binary.path ? reg->binary.path : "NULL",reg->binary.modifytime);
	vid_printf("\t\tbinOffset: %#x\n",reg->binOffset);
	vid_printf("\t\tPDirs: ");
	for(n = sll_begin(reg->pdirs); n != NULL; n = n->next)
		vid_printf("%#x ",(u32)n->data);
	vid_printf("\n");
	vid_printf("\t\tPages (%d):\n",reg->pfSize);
	for(i = 0, x = BYTES_2_PAGES(reg->byteCount); i < x; i++)
		vid_printf("\t\t\t[%d] %c%c%c\n",i,
				reg->pageFlags[i] & PF_COPYONWRITE ? 'c' : '-',
				reg->pageFlags[i] & PF_DEMANDLOAD ? 'l' : '-',
				reg->pageFlags[i] & PF_DEMANDZERO ? 'z' : '-',
				reg->pageFlags[i] & PF_SWAPPED ? 's' : '-');
}

#endif
