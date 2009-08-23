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
#include <vfs.h>
#include <vfsnode.h>
#include <vfsrw.h>
#include <vfsreq.h>
#include <kheap.h>
#include <thread.h>
#include <messages.h>
#include <string.h>
#include <errors.h>
#include <assert.h>

/* the initial size of the write-cache for service-usage-nodes */
#define VFS_INITIAL_WRITECACHE		128

/* a message (for communicating with services) */
typedef struct {
	tMsgId id;
	u32 length;
} sMessage;

s32 vfs_defReadHandler(tTid tid,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	s32 byteCount;
	UNUSED(tid);
	/* no data available? */
	if(node->data.def.cache == NULL)
		return 0;

	if(offset > node->data.def.pos)
		offset = node->data.def.pos;
	byteCount = MIN(node->data.def.pos - offset,count);
	if(byteCount > 0) {
		/* simply copy the data to the buffer */
		memcpy(buffer,(u8*)node->data.def.cache + offset,byteCount);
	}
	return byteCount;
}

s32 vfs_readHelper(tTid tid,sVFSNode *node,u8 *buffer,u32 offset,u32 count,u32 dataSize,
		fReadCallBack callback) {
	void *mem = NULL;

	UNUSED(tid);
	vassert(node != NULL,"node == NULL");
	vassert(buffer != NULL,"buffer == NULL");

	/* just if the datasize is known in advance */
	if(dataSize > 0) {
		/* can we copy it directly? */
		if(offset == 0 && count == dataSize)
			mem = buffer;
		/* don't waste time in this case */
		else if(offset >= dataSize)
			return 0;
		/* ok, use the heap as temporary storage */
		else {
			mem = kheap_alloc(dataSize);
			if(mem == NULL)
				return 0;
		}
	}

	/* copy values to public struct */
	callback(node,&dataSize,&mem);

	/* stored on kheap? */
	if((u32)mem != (u32)buffer) {
		/* correct vars */
		if(offset > dataSize)
			offset = dataSize;
		count = MIN(dataSize - offset,count);
		/* copy */
		if(count > 0)
			memcpy(buffer,(u8*)mem + offset,count);
		/* free temp storage */
		kheap_free(mem);
	}

	return count;
}

s32 vfs_serviceUseReadHandler(tTid tid,sVFSNode *node,tMsgId *id,u8 *data,u32 size) {
	sSLList *list;
	sMessage *msg;
	s32 res;

	/* services reads from the send-list */
	if(node->parent->owner == tid) {
		list = node->data.servuse.sendList;
		/* don't block service-reads */
		if(sll_length(list) == 0)
			return 0;
	}
	/* other processes read from the receive-list */
	else {
		/* don't block the kernel ;) */
		if(tid != KERNEL_TID) {
			/* wait until a message arrives */
			/* don't cache the list here, because the pointer changes if the list is NULL */
			while(sll_length(node->data.servuse.recvList) == 0) {
				thread_wait(tid,EV_RECEIVED_MSG);
				thread_switchInKernel();
			}
		}
		else if(sll_length(node->data.servuse.recvList) == 0)
			return 0;

		list = node->data.servuse.recvList;
	}

	/* get first element and copy data to buffer */
	msg = (sMessage*)sll_get(list,0);
	if(msg->length > size)
		return ERR_INVALID_SYSC_ARGS;

	/* the data is behind the message */
	memcpy(data,(u8*)(msg + 1),msg->length);

	/*vid_printf("%s received msg %d from %s\n",
			tid != KERNEL_TID ? thread_getById(tid)->proc->command : "KERNEL",
					msg->id,node->parent->name);*/

	/* set id, return size and free msg */
	*id = msg->id;
	res = msg->length;
	kheap_free(msg);
	sll_removeIndex(list,0);
	return res;
}

s32 vfs_defWriteHandler(tTid tid,sVFSNode *n,const u8 *buffer,u32 offset,u32 count) {
	void *cache;
	void *oldCache;
	u32 newSize = 0;

	UNUSED(tid);

	cache = n->data.def.cache;
	oldCache = cache;

	/* need to create cache? */
	if(cache == NULL) {
		newSize = MAX(count,VFS_INITIAL_WRITECACHE);
		/* check for overflow */
		if(newSize > 0xFFFF)
			newSize = 0xFFFF;
		if(newSize < count)
			return ERR_NOT_ENOUGH_MEM;

		n->data.def.cache = kheap_alloc(newSize);
		cache = n->data.def.cache;
		/* reset position */
		n->data.def.pos = 0;
	}
	/* need to increase cache-size? */
	else if(n->data.def.size < offset + count) {
		/* ensure that we allocate enough memory */
		newSize = MAX(offset + count,(u32)n->data.def.size * 2);
		/* check for overflow */
		if(newSize > 0xFFFF)
			newSize = 0xFFFF;
		if(newSize < offset + count)
			return ERR_NOT_ENOUGH_MEM;

		n->data.def.cache = kheap_realloc(cache,newSize);
		cache = n->data.def.cache;
	}

	/* all ok? */
	if(cache != NULL) {
		/* copy the data into the cache */
		memcpy((u8*)cache + offset,buffer,count);
		/* set total size and number of used bytes */
		if(newSize)
			n->data.def.size = newSize;
		/* we have checked size for overflow. so it is ok here */
		n->data.def.pos = MAX(n->data.def.pos,offset + count);

		return count;
	}

	/* restore cache */
	n->data.def.cache = oldCache;
	return ERR_NOT_ENOUGH_MEM;
}

s32 vfs_serviceUseWriteHandler(tTid tid,sVFSNode *n,tMsgId id,const u8 *data,u32 size) {
	sSLList **list;
	sMessage *msg;

	/* services write to the receive-list (which will be read by other processes) */
	/* special-case: pid == KERNEL_PID: the kernel wants to write to a service */
	if(tid != KERNEL_TID && n->parent->owner == tid) {
		/* if the message is intended for the kernel, don't enqueue it but pass it directly to the
		 * corresponding handler */
		if(n->owner == KERNEL_TID) {
			vfsreq_sendMsg(id,data,size);
			return 0;
		}

		list = &(n->data.servuse.recvList);
	}
	/* other processes write to the send-list (which will be read by the service) */
	else
		list = &(n->data.servuse.sendList);

	if(*list == NULL)
		*list = sll_create();

	/* create message and copy data to it */
	msg = (sMessage*)kheap_alloc(sizeof(sMessage) + size * sizeof(u8));
	if(msg == NULL)
		return ERR_NOT_ENOUGH_MEM;

	msg->length = size;
	msg->id = id;
	memcpy(msg + 1,data,size);

	/*vid_printf("%s sent msg %d to %s\n",
			tid != KERNEL_TID ? thread_getById(tid)->proc->command : "KERNEL",
					msg->id,n->parent->name);*/

	/* append to list */
	if(!sll_append(*list,msg)) {
		kheap_free(msg);
		return ERR_NOT_ENOUGH_MEM;
	}

	/* notify the service */
	if(list == &(n->data.servuse.sendList)) {
		if(n->parent->owner != KERNEL_TID)
			thread_wakeup(n->parent->owner,EV_CLIENT);
	}
	/* notify the process that there is a message */
	else
		thread_wakeup(n->owner,EV_RECEIVED_MSG);
	return 0;
}
