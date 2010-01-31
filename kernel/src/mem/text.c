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
#include <mem/text.h>
#include <mem/pmem.h>
#include <mem/paging.h>
#include <mem/kheap.h>
#include <vfs/vfs.h>
#include <vfs/real.h>
#include <task/elf.h>
#include <fsinterface.h>
#include <video.h>
#include <assert.h>
#include <errors.h>

#define BUF_SIZE 1024

/**
 * Loads the text for the given (always current) process from file
 *
 * @param p the current process
 * @param file the file
 * @param position the position of the text in the file
 * @param textSize the size of the text
 * @return 0 on success
 */
static s32 text_load(sProc *p,tFileNo file,u32 position,u32 textSize);

/**
 * Searches for a text-usages with given file-info (inode and modifytime)
 *
 * @param info the file-info
 * @return the text to use or NULL
 */
static sTextUsage *text_get(sFileInfo *info);

/* a list with all known text-usages */
static sSLList *textUsages = NULL;

s32 text_alloc(const char *path,tFileNo file,u32 position,u32 textSize,sTextUsage **text) {
	sFileInfo info;
	sTextUsage *usage;
	sProc *p = proc_getRunning();
	sThread *t = thread_getRunning();
	s32 res;
	if((res = vfsr_stat(t->tid,path,&info)) < 0)
		return res;

	usage = text_get(&info);
	if(usage == NULL) {
		/* ok, so we've to create a new usage */
		usage = (sTextUsage*)kheap_alloc(sizeof(sTextUsage));
		if(usage == NULL)
			return ERR_NOT_ENOUGH_MEM;
		usage->inodeNo = info.inodeNo;
		usage->modifytime = info.modifytime;
		usage->procs = sll_create();
		if(usage->procs == NULL) {
			kheap_free(usage);
			return ERR_NOT_ENOUGH_MEM;
		}

		/* create list if not already done */
		if(textUsages == NULL) {
			textUsages = sll_create();
			if(textUsages == NULL) {
				sll_destroy(usage->procs,false);
				kheap_free(usage);
				return ERR_NOT_ENOUGH_MEM;
			}
		}

		/* append process */
		if(!sll_append(usage->procs,(void*)p)) {
			sll_destroy(usage->procs,false);
			kheap_free(usage);
			return ERR_NOT_ENOUGH_MEM;
		}

		/* load the text from file */
		res = text_load(p,file,position,textSize);
		if(res < 0) {
			sll_destroy(usage->procs,false);
			kheap_free(usage);
			return res;
		}

		/* append to usages */
		if(!sll_append(textUsages,usage)) {
			/* we don't need to undo text_load() here. the only side-effect of it is paging_map().
			 * the process will be destroyed in this case anyway (and the mem free'd). */
			sll_destroy(usage->procs,false);
			kheap_free(usage);
			return ERR_NOT_ENOUGH_MEM;
		}
	}
	else {
		/* ok, there is another process that uses the requested program */
		/* so we can share the pages */
		sProc *fp;
		vassert(sll_length(usage->procs) > 0,"text-usage with no processes!?");
		fp = (sProc*)sll_get(usage->procs,0);
		vassert(fp->pid != INVALID_PID,"text-usage of unused process!?");

		/* append process */
		if(!sll_append(usage->procs,(void*)p))
			return ERR_NOT_ENOUGH_MEM;

		/* copy pages */
		paging_getPagesOf(fp,0,0,fp->textPages,0);
		p->textPages = fp->textPages;
	}

	*text = usage;
	return 0;
}

bool text_clone(sTextUsage *u,tPid pid) {
	sProc *p;
	/* ignore it when there is no text-usage */
	if(u == NULL)
		return true;

	p = proc_getByPid(pid);
	return sll_append(u->procs,(void*)p);
}

bool text_free(sTextUsage *u,tPid pid) {
	sProc *p;
	/* if there is no text-usage the process has no text, so ignore it */
	if(u == NULL)
		return false;

	/* remove process */
	p = proc_getByPid(pid);
	vassert(sll_removeFirst(u->procs,p),"Unable to find process %x",p);

	/* if there are no more references we have to free the frames and
	 * delete the usage-node */
	if(sll_length(u->procs) == 0) {
		sll_destroy(u->procs,false);
		sll_removeFirst(textUsages,u);
		kheap_free(u);
		/* let the caller free the frames */
		return true;
	}
	return false;
}

static s32 text_load(sProc *p,tFileNo file,u32 position,u32 textSize) {
	u8 *target;
	s32 rem,res;
	u32 pages;
	sThread *t = thread_getRunning();

	/* at first we have to create the pages for the process */
	pages = BYTES_2_PAGES(textSize);
	if(mm_getFreeFrmCount(MM_DEF) < pages)
		return ERR_NOT_ENOUGH_MEM;
	paging_map(0,NULL,pages,0,true);
	p->textPages = pages;

	/* load text from disk */
	vfs_seek(t->tid,file,position,SEEK_SET);
	target = 0;
	rem = textSize;
	while(rem > 0) {
		res = vfs_readFile(t->tid,file,target,MIN(BUF_SIZE,rem));
		if(res < 0)
			return res;
		rem -= res;
		target += res;
	}
	/* ensure that the specified size was correct */
	if(rem != 0)
		return ERR_INVALID_ELF_BIN;
	return 0;
}

static sTextUsage *text_get(sFileInfo *info) {
	sSLNode *n,*p;
	sTextUsage *u;
	if(textUsages == NULL)
		return NULL;
	p = NULL;
	for(n = sll_begin(textUsages); n != NULL; p = n, n = n->next) {
		u = (sTextUsage*)n->data;
		/* if the file has been modified, we don't want to use it again */
		if(info->inodeNo == u->inodeNo && info->modifytime == u->modifytime)
			return u;
	}
	return NULL;
}


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void text_dbg_print(void) {
	sSLNode *n,*pn;
	sTextUsage *u;
	vid_printf("Text-Usages:\n");
	if(textUsages == NULL)
		return;
	for(n = sll_begin(textUsages); n != NULL; n = n->next) {
		u = (sTextUsage*)n->data;
		vid_printf("\tinode=%d, modified=%d, procs=",u->inodeNo,u->modifytime);
		for(pn = sll_begin(u->procs); pn != NULL; pn = pn->next) {
			vid_printf("%d",((sProc*)pn->data)->pid);
			if(pn->next != NULL)
				vid_printf(",");
		}
		vid_printf("\n");
	}
}

#endif
