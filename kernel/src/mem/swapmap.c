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
#include <mem/swapmap.h>
#include <mem/paging.h>
#include <task/proc.h>
#include <video.h>
#include <assert.h>

/* The idea is:
 * Keeping track of all blocks of the swap-device would cost A LOT of memory (consider a 4GiB
 * swap-partition. that would be 1M blocks and we would probably need several bytes for each block).
 * Therefore we use a heap-like map that splits the space into areas. Each area consists of
 * contiguous pages of one process, i.e. we try to group multiple pages together if possible and
 * put these in contiguous blocks.
 * We provide a static number of areas and use a freelist to be able to allocate and free areas
 * quickly. Additionally we have a list of "used" areas, i.e. areas that have an address in the
 * swap-space which may be free or not.
 * So if we have for example 1024 running processes and each process is split into 8 parts, we
 * need 8192 areas for it. That means it may be possible (and likely, too) that we can't use
 * the whole swap-space due to a limited number of areas. But it means also that our memory-usage
 * for it is independent of the swap-space-size and that we don't need much memory if the swap-
 * space is not used at all (to be precise, the memory-usage is constant: 8192 areas, with 20
 * bytes each -> 160 KiB).
 *
 * TODO an improvement of this could be to allocate the areas dynamically. i.e. for example we
 * allocate one frame at the beginning and as soon as we have no areas anymore we allocate a new
 * frame and would get (4096 / 20) new ones. A problem could be that - of course - we use this
 * stuff here if we have not enough memory. So its not really good to aquire memory here...
 */

#define SWMAP_SIZE		8192

typedef struct sSwMapArea sSwMapArea;
struct sSwMapArea {
	sSwMapArea *next;
	/* starting block in swap-device */
	u32 block;
	/* the number of blocks of this area */
	u32 size;
	/* position in virtual memory of the process */
	u32 virt;
	tPid pid;
	/* wether this area is free or not (16bit to fill the whole) */
	u16 free;
};

static sSwMapArea *swmap_getNewArea(u32 block,u32 size,tPid pid,u32 virt,bool free);
static void swmap_remUsedArea(sSwMapArea *a,sSwMapArea *p,sSwMapArea *pp);
static void swmap_remFreeArea(sSwMapArea *area,sSwMapArea *prev);

static sSwMapArea entries[SWMAP_SIZE];
static sSwMapArea *freelist;
static sSwMapArea *used;

void swmap_init(u32 swapSize) {
	u32 i;
	/* put all areas on the freelist */
	freelist = entries;
	freelist->next = NULL;
	for(i = 1; i < SWMAP_SIZE; i++) {
		entries[i].next = freelist;
		freelist = entries + i;
	}

	/* create one block with all free space */
	used = swmap_getNewArea(0,swapSize / PAGE_SIZE,INVALID_PID,0,true);
	assert(used != NULL);
	used->next = NULL;
}

void swmap_remProc(tPid pid) {

}

u32 swmap_alloc(tPid pid,u32 virt,u32 count) {
	sSwMapArea *p = NULL;
	sSwMapArea *pp = NULL;
	sSwMapArea *a = used;
	while(a != NULL) {
		/* if our requested stuff is directly behind this area and the next area has enough
		 * room, we can add it to this area */
		if(a->pid == pid && !a->free && a->virt + a->size * PAGE_SIZE == virt && a->next &&
				a->next->free && a->next->size >= count) {
			u32 block = a->block + a->size;
			/* prevent that the last region will be removed */
			if(!a->next->next && a->next->size == count)
				return INVALID_BLOCK;
			a->size += count;
			a->next->block += count;
			a->next->size -= count;
			/* if the next is empty now, remove it */
			if(a->next->size == 0)
				swmap_remFreeArea(a->next,a);
			return block;
		}

		/* if our requested stuff is directly before the next area and this area is free and
		 * has enough room, we can put our stuff before the next one */
		if(a->free && a->size >= count && a->next && a->next->pid == pid &&
				virt + count * PAGE_SIZE == a->next->virt) {
			u32 block = a->next->block - count;
			a->next->virt = virt;
			a->next->size += count;
			a->next->block -= count;
			a->size -= count;
			/* area empty? remove it */
			if(a->size == 0)
				swmap_remFreeArea(a,p);
			return block;
		}

		pp = p;
		p = a;
		a = a->next;
	}

	/* if were here we have found no area to that we can add our stuff. there we use the last
	 * area which holds all of the remaining free space */

	/* don't remove the last area */
	if(p->size <= count)
		return INVALID_BLOCK;
	a = swmap_getNewArea(p->block,count,pid,virt,false);
	if(!a)
		return INVALID_BLOCK;
	/* insert in used-list */
	if(pp)
		pp->next = a;
	else
		used = a;
	a->next = p;
	/* change p appropriately */
	p->block += count;
	p->size -= count;
	return a->block;
}

u32 swmap_find(tPid pid,u32 virt) {
	sSwMapArea *a = used;
	while(a != NULL) {
		if(!a->free && a->pid == pid && virt >= a->virt && virt < a->virt + a->size * PAGE_SIZE)
			return a->block + (virt - a->virt) / PAGE_SIZE;
		a = a->next;
	}
	return INVALID_BLOCK;
}

u32 swmap_freeSpace(void) {
	u32 total = 0;
	sSwMapArea *a = used;
	while(a != NULL) {
		if(a->free)
			total += a->size;
		a = a->next;
	}
	return total * PAGE_SIZE;
}

void swmap_free(tPid pid,u32 block,u32 count) {
	sSwMapArea *p = NULL;
	sSwMapArea *pp = NULL;
	sSwMapArea *a = used;
	while(a != NULL) {
		if(!a->free && a->pid == pid && block >= a->block && block + count <= a->block + a->size) {
			/* if same size, we can remove the area */
			if(a->size == count) {
				swmap_remUsedArea(a,p,pp);
				return;
			}

			/* space on both sides */
			if(block > a->block && block + count < a->block + a->size) {
				/* we need 2 new areas */
				sSwMapArea *pre = swmap_getNewArea(
						a->block,block - a->block,a->pid,a->virt,false);
				sSwMapArea *post = swmap_getNewArea(
						block + count,a->block + a->size - (block + count),a->pid,
						a->virt + ((block - a->block) + count) * PAGE_SIZE,false);
				assert(pre != NULL && post != NULL);
				if(p)
					p->next = pre;
				else
					used = pre;
				pre->next = a;
				post->next = a->next;
				a->next = post;
				a->block = block;
			}
			/* space on the left */
			else if(block > a->block) {
				sSwMapArea *pre = swmap_getNewArea(
						a->block,block - a->block,a->pid,a->virt,false);
				assert(pre != NULL);
				if(p)
					p->next = pre;
				else
					used = pre;
				/* if the next is free too, we have to connect them */
				if(a->next->free) {
					a->next->block = block;
					a->next->size += count;
					pre->next = a->next;
					a->next = freelist;
					freelist = a;
					return;
				}
				/* otherwise just change a */
				pre->next = a;
				a->block += block - a->block;
			}
			/* space on the right */
			else {
				sSwMapArea *post = swmap_getNewArea(
					block + count,a->block + a->size - (block + count),a->pid,
					a->virt + count * PAGE_SIZE,false);
				assert(post != NULL);
				post->next = a->next;
				/* if the previous one is free, connect them */
				if(p && p->free) {
					p->size += count;
					p->next = post;
					a->next = freelist;
					freelist = a;
					return;
				}
				/* otherwise just change a */
				a->next = post;
				a->block = block;
			}
			a->size = count;
			a->free = true;
			return;
		}

		pp = p;
		p = a;
		a = a->next;
	}
	vassert(false,"Area with pid=%u, blocks=%u..%u not found",pid,block,block + count - 1);
}

static sSwMapArea *swmap_getNewArea(u32 block,u32 size,tPid pid,u32 virt,bool free) {
	sSwMapArea *a = freelist;
	if(a == NULL)
		return NULL;
	freelist = freelist->next;

	a->block = block;
	a->size = size;
	a->pid = pid;
	a->virt = virt;
	a->free = free;
	return a;
}

static void swmap_remUsedArea(sSwMapArea *a,sSwMapArea *p,sSwMapArea *pp) {
	/* both existent (next always) and both free, i.e. merge all into the prev one */
	if(p && p->free && a->next->free) {
		/* put everything into the next one (to prefer the last area of the used-list) */
		a->next->size += p->size + a->size;
		a->next->block = p->block;
		/* remove both */
		if(pp)
			pp->next = a->next;
		else
			used = a->next;
		/* put them on the freelist */
		a->next = freelist;
		freelist = p;
	}
	/* prev is free */
	else if(p && p->free) {
		p->size += a->size;
		p->next = a->next;
		a->next = freelist;
		freelist = a;
	}
	/* next is free */
	else if(a->next->free) {
		a->next->size += a->size;
		a->next->block = a->block;
		if(p)
			p->next = a->next;
		else
			used = a->next;
		a->next = freelist;
		freelist = a;
	}
	/* no free areas around */
	else
		a->free = true;
}

static void swmap_remFreeArea(sSwMapArea *area,sSwMapArea *prev) {
	assert(area->free);
	/* out of used list */
	if(prev) {
		/* if the previous and the next one fit together, connect them */
		if(area->next && prev->pid == area->next->pid &&
			prev->virt + prev->size * PAGE_SIZE == area->next->virt) {
			/* connect */
			prev->size += area->next->size;
			/* remove and put on free list */
			prev->next = area->next->next;
			area->next->next = freelist;
			freelist = area->next;
		}
		else
			prev->next = area->next;
	}
	else
		used = area->next;
	/* on free list */
	area->next = freelist;
	freelist = area;
}


#if DEBUGGING

void swmap_dbg_print(void) {
	sSwMapArea *a = used;
	vid_printf("SwapMap:\n");
	while(a != NULL) {
		if(!a->free) {
			vid_printf("\t%04u-%04u: p%02u (%p - %p)\n",a->block,a->block + a->size - 1,
					a->pid,a->virt,a->virt + a->size * PAGE_SIZE - 1);
		}
		else
			vid_printf("\t%04u-%04u: free\n",a->block,a->block + a->size - 1);
		a = a->next;
	}
}

#endif
