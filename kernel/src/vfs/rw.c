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

#include <sys/common.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/rw.h>
#include <sys/vfs/request.h>
#include <sys/mem/kheap.h>
#include <sys/task/thread.h>
#include <sys/task/signals.h>
#include <esc/messages.h>
#include <string.h>
#include <errors.h>
#include <assert.h>

/* the initial size of the write-cache for driver-usage-nodes */
#define VFS_INITIAL_WRITECACHE		128

/* a message (for communicating with drivers) */
typedef struct {
	tMsgId id;
	u32 length;
} sMessage;

/* a data-"message" in a pipe */
typedef struct {
	u32 length;
	u32 offset;
	u8 data[];
} sPipeData;

s32 vfsrw_readDef(tTid tid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	s32 byteCount;
	UNUSED(tid);
	UNUSED(file);
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

s32 vfsrw_readHelper(tTid tid,sVFSNode *node,u8 *buffer,u32 offset,u32 count,u32 dataSize,
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
	if(mem == NULL)
		return 0;

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

s32 vfsrw_readPipe(tTid tid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	s32 byteCount,total;
	sPipeData *data;
	sSLList *list;
	UNUSED(file);
	volatile sVFSNode *n = node;
	/* wait until data is available */
	/* don't cache the list here, because the pointer changes if the list is NULL */
	while(sll_length(n->data.pipe.list) == 0) {
		thread_wait(tid,node,EV_PIPE_FULL);
		thread_switch();
		if(sig_hasSignalFor(tid))
			return ERR_INTERRUPTED;
	}

	list = node->data.pipe.list;
	data = sll_get(list,0);
	/* empty message indicates EOF */
	if(data->length == 0)
		return 0;

	total = 0;
	while(1) {
		/* copy */
		vassert(offset >= data->offset,"Illegal offset");
		vassert(data->length >= (offset - data->offset),"Illegal offset");
		byteCount = MIN(data->length - (offset - data->offset),count);
		memcpy(buffer + total,data->data + (offset - data->offset),byteCount);
		count -= byteCount;
		total += byteCount;
		/* remove if read completely */
		if(byteCount + (offset - data->offset) == data->length) {
			kheap_free(data);
			sll_removeIndex(list,0);
		}
		node->data.pipe.total -= byteCount;
		offset += byteCount;
		if(count == 0)
			break;
		/* wait until data is available */
		while(sll_length(n->data.pipe.list) == 0) {
			/* before we go to sleep we have to notify others that we've read data. otherwise
			 * we may cause a deadlock here */
			thread_wakeupAll(node,EV_PIPE_EMPTY);
			thread_wait(tid,node,EV_PIPE_FULL);
			/* TODO we can't accept signals here, right? since we've already read something, which
			 * we have to deliver to the user. the only way I can imagine would be to put it back..
			 */
			thread_switchNoSigs();
		}
		data = sll_get(list,0);
		/* keep the empty one for the next transfer */
		if(data->length == 0)
			break;
	}
	/* wakeup all threads that wait for writing in this node */
	thread_wakeupAll(node,EV_PIPE_EMPTY);
	return total;
}

s32 vfsrw_readDrvUse(tTid tid,tFileNo file,sVFSNode *node,tMsgId *id,u8 *data,u32 size) {
	sSLList **list;
	sMessage *msg;
	u16 event;
	s32 res;
	UNUSED(file);

	/* wait until a message arrives */
	if(node->parent->owner == tid) {
		event = EV_CLIENT;
		list = &node->data.drvuse.sendList;
	}
	else {
		event = EV_RECEIVED_MSG;
		list = &node->data.drvuse.recvList;
	}
	while(sll_length(*list) == 0) {
		thread_wait(tid,0,event);
		thread_switch();
		if(sig_hasSignalFor(tid))
			return ERR_INTERRUPTED;
		/* if we waked up and the node is not our, the node has been destroyed (driver died, ...) */
		if(event == EV_RECEIVED_MSG && node->owner != tid)
			return ERR_INVALID_FILE;
	}

	/* get first element and copy data to buffer */
	msg = (sMessage*)sll_get(*list,0);
	if(data && msg->length > size) {
		kheap_free(msg);
		sll_removeIndex(*list,0);
		return ERR_INVALID_ARGS;
	}

	/* the data is behind the message */
	if(data)
		memcpy(data,(u8*)(msg + 1),msg->length);

	/*vid_printf("%s received msg %d from %s\n",thread_getById(tid)->proc->command,
					msg->id,node->parent->name);*/

	/* set id, return size and free msg */
	if(id)
		*id = msg->id;
	res = msg->length;
	kheap_free(msg);
	sll_removeIndex(*list,0);
	return res;
}

s32 vfsrw_writeDef(tTid tid,tFileNo file,sVFSNode *n,const u8 *buffer,u32 offset,u32 count) {
	void *cache,*oldCache;
	u32 newSize = 0;

	UNUSED(tid);
	UNUSED(file);

	cache = n->data.def.cache;
	oldCache = cache;

	/* need to create cache? */
	if(cache == NULL) {
		newSize = MAX(count,VFS_INITIAL_WRITECACHE);
		/* check for overflow */
		if(newSize > MAX_VFS_FILE_SIZE)
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
		if(newSize > MAX_VFS_FILE_SIZE)
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
	/* don't throw the data away, use the old version */
	n->data.def.cache = oldCache;
	return ERR_NOT_ENOUGH_MEM;
}

s32 vfsrw_writePipe(tTid tid,tFileNo file,sVFSNode *node,const u8 *buffer,u32 offset,u32 count) {
	sPipeData *data;
	UNUSED(file);
	volatile sVFSNode *n = node;
	/* wait while our node is full */
	if(count) {
		while((n->data.pipe.total + count) >= MAX_VFS_FILE_SIZE) {
			thread_wait(tid,node,EV_PIPE_EMPTY);
			thread_switchNoSigs();
		}
	}

	/* build pipe-data */
	data = (sPipeData*)kheap_alloc(sizeof(sPipeData) + count);
	if(data == NULL)
		return ERR_NOT_ENOUGH_MEM;
	data->offset = offset;
	data->length = count;
	if(count)
		memcpy(data->data,buffer,count);

	/* create list, if necessary */
	if(node->data.pipe.list == NULL) {
		node->data.pipe.list = sll_create();
		if(node->data.pipe.list == NULL) {
			kheap_free(data);
			return ERR_NOT_ENOUGH_MEM;
		}
	}

	/* append */
	if(!sll_append(node->data.pipe.list,data)) {
		kheap_free(data);
		return ERR_NOT_ENOUGH_MEM;
	}
	node->data.pipe.total += count;
	thread_wakeupAll(node,EV_PIPE_FULL);
	return count;
}

s32 vfsrw_writeDrvUse(tTid tid,tFileNo file,sVFSNode *n,tMsgId id,const u8 *data,u32 size) {
	sSLList **list;
	sMessage *msg;

	UNUSED(file);

	/*vid_printf("%s sent msg %d with %d bytes to %s\n",
			thread_getById(tid)->proc->command,id,size,n->parent->name);*/

	/* drivers write to the receive-list (which will be read by other processes) */
	if(n->parent->owner == tid) {
		/* if it is from a driver or fs, don't enqueue it but pass it directly to
		 * the corresponding handler */
		if(DRV_IS_FS(n->parent->data.driver.funcs) ||
			(id == MSG_DRV_OPEN_RESP || id == MSG_DRV_READ_RESP || id == MSG_DRV_WRITE_RESP)) {
			vfsreq_sendMsg(id,n->parent,n->owner,data,size);
			return 0;
		}

		list = &(n->data.drvuse.recvList);
	}
	/* other processes write to the send-list (which will be read by the driver) */
	else
		list = &(n->data.drvuse.sendList);

	if(*list == NULL)
		*list = sll_create();

	/* create message and copy data to it */
	msg = (sMessage*)kheap_alloc(sizeof(sMessage) + size * sizeof(u8));
	if(msg == NULL)
		return ERR_NOT_ENOUGH_MEM;

	msg->length = size;
	msg->id = id;
	if(data)
		memcpy(msg + 1,data,size);

	/* append to list */
	if(!sll_append(*list,msg)) {
		kheap_free(msg);
		return ERR_NOT_ENOUGH_MEM;
	}

	/* notify the driver */
	if(list == &(n->data.drvuse.sendList))
		thread_wakeup(n->parent->owner,EV_CLIENT);
	/* notify the process that there is a message */
	else
		thread_wakeup(n->owner,EV_RECEIVED_MSG);
	return 0;
}
