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
#include <sys/mem/kheap.h>
#include <sys/task/thread.h>
#include <sys/task/event.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/request.h>
#include <sys/vfs/channel.h>
#include <sys/vfs/server.h>
#include <sys/vfs/driver.h>
#include <sys/video.h>
#include <esc/messages.h>
#include <esc/sllist.h>
#include <string.h>
#include <errors.h>

typedef struct {
	/* a list for sending messages to the driver */
	sSLList *sendList;
	/* a list for reading messages from the driver */
	sSLList *recvList;
} sChannel;

typedef struct {
	tMsgId id;
	u32 length;
} sMessage;

static void vfs_chan_destroy(sVFSNode *n);
static s32 vfs_chan_seek(tPid pid,sVFSNode *node,s32 position,s32 offset,u32 whence);
static void vfs_chan_close(tPid pid,tFileNo file,sVFSNode *node);

sVFSNode *vfs_chan_create(tPid pid,sVFSNode *parent) {
	sChannel *chan;
	sVFSNode *node;
	char *name = vfs_node_getId(pid);
	if(!name)
		return NULL;
	node = vfs_node_create(parent,name);
	if(node == NULL) {
		kheap_free(name);
		return NULL;
	}

	node->owner = pid;
	node->mode = MODE_TYPE_DRVUSE | MODE_OWNER_READ | MODE_OWNER_WRITE |
			MODE_OTHER_READ | MODE_OTHER_WRITE;
	node->readHandler = (fRead)vfsdrv_read;
	node->writeHandler = (fWrite)vfsdrv_write;
	node->seek = vfs_chan_seek;
	node->close = vfs_chan_close;
	node->destroy = vfs_chan_destroy;
	node->data = NULL;
	chan = (sChannel*)kheap_alloc(sizeof(sChannel));
	if(!chan) {
		vfs_node_destroy(node);
		return NULL;
	}
	chan->recvList = NULL;
	chan->sendList = NULL;
	node->data = chan;
	return node;
}

static void vfs_chan_destroy(sVFSNode *n) {
	sChannel *chan = (sChannel*)n->data;
	if(chan) {
		/* we have to reset the last client for the driver here */
		vfs_server_clientRemoved(n->parent,n);
		/* free send and receive list */
		if(chan->recvList)
			sll_destroy(chan->recvList,true);
		if(chan->sendList)
			sll_destroy(chan->sendList,true);
		kheap_free(chan);
		n->data = NULL;
	}
}

static s32 vfs_chan_seek(tPid pid,sVFSNode *node,s32 position,s32 offset,u32 whence) {
	UNUSED(pid);
	UNUSED(node);
	switch(whence) {
		case SEEK_SET:
			return offset;
		case SEEK_CUR:
			return position + offset;
		default:
		case SEEK_END:
			/* not supported for drivers */
			return ERR_INVALID_ARGS;
	}
}

static void vfs_chan_close(tPid pid,tFileNo file,sVFSNode *node) {
	sChannel *chan = (sChannel*)node->data;
	if(node->refCount == 0) {
		/* notify the driver, if it is one */
		vfsdrv_close(pid,file,node);

		/* if there are message for the driver we don't want to throw them away */
		/* if there are any in the receivelist (and no references of the node) we
		 * can throw them away because no one will read them anymore
		 * (it means that the client has already closed the file) */
		/* note also that we can assume that the driver is still running since we
		 * would have deleted the whole driver-node otherwise */
		if(sll_length(chan->sendList) == 0)
			vfs_node_destroy(node);
	}
}

bool vfs_chan_hasReply(sVFSNode *node) {
	/* TODO temporary */
	sChannel *chan = (sChannel*)node->data;
	return sll_length(chan->recvList) > 0;
}

bool vfs_chan_hasWork(sVFSNode *node) {
	sChannel *chan = (sChannel*)node->data;
	return sll_length(chan->sendList) > 0;
}

s32 vfs_chan_send(tPid pid,tFileNo file,sVFSNode *n,tMsgId id,const u8 *data,u32 size) {
	sSLList **list;
	sMessage *msg;
	sChannel *chan = (sChannel*)n->data;

	UNUSED(file);

	/*vid_printf("%d:%s sent msg %d with %d bytes to %s\n",pid,proc_getByPid(pid)->command,id,size,
			n->parent->owner == pid ? n->name : n->parent->name);*/

	/* drivers write to the receive-list (which will be read by other processes) */
	if(n->parent->owner == pid) {
		/* if it is from a driver or fs, don't enqueue it but pass it directly to
		 * the corresponding handler */
		if(vfs_server_accepts(n->parent,id)) {
			vfs_req_sendMsg(id,n,data,size);
			return 0;
		}

		list = &(chan->recvList);
	}
	/* other processes write to the send-list (which will be read by the driver) */
	else
		list = &(chan->sendList);

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
	if(list == &(chan->sendList))
		proc_wakeup(n->parent->owner,n->parent,EV_CLIENT);
	/* notify all threads that wait on this node for a msg */
	else
		ev_wakeup(EVI_RECEIVED_MSG,(tEvObj)n);
	return 0;
}

s32 vfs_chan_receive(tPid pid,tFileNo file,sVFSNode *node,tMsgId *id,u8 *data,u32 size) {
	sSLList **list;
	sThread *t = thread_getRunning();
	sChannel *chan = (sChannel*)node->data;
	sMessage *msg;
	u32 event;
	s32 res;
	UNUSED(file);

	/* wait until a message arrives */
	if(node->parent->owner == pid) {
		event = EVI_CLIENT;
		list = &chan->sendList;
	}
	else {
		event = EVI_RECEIVED_MSG;
		list = &chan->recvList;
	}
	while(sll_length(*list) == 0) {
		if(!vfs_shouldBlock(file))
			return ERR_WOULD_BLOCK;
		ev_wait(t->tid,event,(tEvObj)node);
		thread_switch();
		if(sig_hasSignalFor(t->tid))
			return ERR_INTERRUPTED;
		/* if we waked up and there is no message, the driver probably died */
		if(event == EVI_RECEIVED_MSG && sll_length(*list) == 0)
			return ERR_DRIVER_DIED;
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

#if DEBUGGING

static void vfs_chan_dbg_printMessage(void *m) {
	sMessage *msg = (sMessage*)m;
	vid_printf("\t\t\tid=%u len=%u\n",msg->id,msg->length);
}

void vfs_chan_dbg_print(sVFSNode *n) {
	u32 i;
	sChannel *chan = (sChannel*)n->data;
	sSLList *lists[] = {chan->sendList,chan->recvList};
	for(i = 0; i < ARRAY_SIZE(lists); i++) {
		u32 j,count = sll_length(lists[i]);
		vid_printf("\t\tChannel %s %s: (%d)\n",n->name,i ? "recvs" : "sends",count);
		for(j = 0; j < count; j++)
			vfs_chan_dbg_printMessage(sll_get(lists[i],j));
	}
}

#endif
