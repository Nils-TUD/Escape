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
#include <apps/apps.h>
#include <task/thread.h>
#include <vfs/vfs.h>
#include <vfs/real.h>
#include <vfs/listeners.h>
#include <vfs/rw.h>
#include <mem/kheap.h>
#include <util.h>
#include <video.h>
#include <sllist.h>
#include <app.h>
#include <string.h>
#include <errors.h>
#include <assert.h>

#define APP_MAP_SIZE  256

typedef struct {
	char *source;
	bool writable;
} sAppDB;

static bool apps_isInStrList(sSLList *list,const char *str);
static bool apps_isInRangeList(sSLList *list,u32 start,u32 count);
static s32 apps_add(sApp *app);
static void apps_rem(sApp *app);
static s32 apps_readHandler(tTid tid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count);
static void apps_readCallback(sVFSNode *node,u32 *dataSize,void **buffer);
static void apps_listener(sVFSNode *node,u32 event);

static bool enabled = true;
static sVFSNode *appsNode = NULL;
static sSLList *apps[APP_MAP_SIZE];

void apps_init(void) {
	tInodeNo nodeNo;
	sVFSNode *node;
	if(vfsn_resolvePath("/system",&nodeNo,NULL,VFS_READ) < 0)
		util_panic("Unable to resolve path '/system'");
	node = vfsn_getNode(nodeNo);
	appsNode = vfsn_createDir(node,(char*)"apps");
	if(vfsl_add(appsNode,VFS_EV_CREATE | VFS_EV_MODIFY | VFS_EV_DELETE,apps_listener) < 0)
		util_panic("Unable to create file '/system/apps'");
}

s32 apps_loadDBFromFile(const char *path) {
	const u32 bufIncSize = 16;
	s32 err,count;
	tFileNo file;
	char *buffer;
	u32 bufPos = 0;
	/* one char more for '\0' */
	u32 bufSize = bufIncSize + 1;
	sThread *t = thread_getRunning();

	/* read file into buffer */
	file = vfsr_openFile(t->tid,VFS_READ,path);
	if(file < 0)
		return file;
	buffer = (char*)kheap_alloc(bufSize);
	if(!buffer)
		return ERR_NOT_ENOUGH_MEM;
	while((count = vfs_readFile(t->tid,file,(u8*)buffer + bufPos,bufIncSize)) > 0) {
		bufPos += count;
		if(count != (s32)bufIncSize)
			break;

		bufSize += bufIncSize;
		buffer = (char*)kheap_realloc(buffer,bufSize);
		if(!buffer)
			return ERR_NOT_ENOUGH_MEM;
	}
	vfs_closeFile(t->tid,file);
	/* terminate */
	buffer[bufPos] = '\0';

	/* now read the apps */
	err = apps_loadDB(buffer);
	kheap_free(buffer);
	return err;
}

s32 apps_loadDB(const char *defs) {
	s32 err;
	char src[MAX_PATH_LEN];
	bool srcWritable;
	sApp *app;
	/* now read the apps */
	while(1) {
		/* parse app from it */
		app = (sApp*)kheap_alloc(sizeof(sApp));
		if(!app)
			return ERR_NOT_ENOUGH_MEM;
		defs = app_fromString(defs,app,src,&srcWritable);
		if(defs == NULL) {
			kheap_free(app);
			break;
		}

		/* add to db */
		if((err = apps_add(app)) < 0) {
			kheap_free(app);
			return err;
		}
	}
	return 0;
}

bool apps_isEnabled(void) {
	return enabled;
}

bool apps_canUseIOPorts(sApp *app,u16 start,u16 count) {
	if(!enabled)
		return true;
	assert(app != NULL);
	return apps_isInRangeList(app->ioports,start,count);
}

bool apps_canUseDriver(sApp *app,const char *name,u32 type,u16 ops) {
	sSLNode *n;
	sDriverPerm *drv;
	if(!enabled)
		return true;
	assert(app != NULL);
	if(app->driver == NULL)
		return false;
	for(n = sll_begin(app->driver); n != NULL; n = n->next) {
		drv = (sDriverPerm*)n->data;
		if(drv->group == type || (drv->group == DRV_GROUP_CUSTOM && strcmp(drv->name,name) == 0)) {
			if((ops & DRV_OP_READ) && !drv->read)
				return false;
			if((ops & DRV_OP_WRITE) && !drv->write)
				return false;
			if((ops & DRV_OP_IOCTL) && !drv->ioctrl)
				return false;
			return true;
		}
	}
	return false;
}

bool apps_canUsePhysMem(sApp *app,u32 start,u32 count) {
	if(!enabled)
		return true;
	assert(app != NULL);
	return apps_isInRangeList(app->physMem,start,count);
}

bool apps_canCreateShMem(sApp *app,const char *name) {
	if(!enabled)
		return true;
	assert(app != NULL);
	return apps_isInStrList(app->createShMem,name);
}

bool apps_canJoinShMem(sApp *app,const char *name) {
	if(!enabled)
		return true;
	assert(app != NULL);
	return apps_isInStrList(app->joinShMem,name);
}

bool apps_canUseFS(sApp *app,u16 ops) {
	if(!enabled)
		return true;
	assert(app != NULL);
	if((ops & FS_OP_READ) && !app->fs.read)
		return false;
	if((ops & FS_OP_WRITE) && !app->fs.write)
		return false;
	return true;
}

bool apps_canGetIntrpt(sApp *app,tSig signal) {
	sSLNode *n;
	if(!enabled)
		return true;
	assert(app != NULL);
	if(app->intrpts == NULL)
		return false;
	for(n = sll_begin(app->intrpts); n != NULL; n = n->next) {
		if((u32)n->data == signal)
			return true;
	}
	return false;
}

sApp *apps_get(const char *name) {
	sApp *a;
	sSLNode *n;
	sSLList *list = apps[*name % APP_MAP_SIZE];
	if(list == NULL)
		return NULL;
	for(n = sll_begin(list); n != NULL; n = n->next) {
		a = (sApp*)n->data;
		if(strcmp(a->name,name) == 0)
			return a;
	}
	return NULL;
}

static bool apps_isInStrList(sSLList *list,const char *str) {
	sSLNode *n;
	if(list == NULL)
		return false;
	for(n = sll_begin(list); n != NULL; n = n->next) {
		if(strcmp((char*)n->data,str) == 0)
			return true;
	}
	return false;
}

static bool apps_isInRangeList(sSLList *list,u32 start,u32 count) {
	sSLNode *n;
	sRange *r;
	if(list == NULL)
		return false;
	for(n = sll_begin(list); n != NULL; n = n->next) {
		r = (sRange*)n->data;
		if(r->start == start && r->end == start + count - 1)
			return true;
	}
	return false;
}

static s32 apps_add(sApp *app) {
	sSLList *list;
	char *nodeName;
	assert(app != NULL);

	list = apps[*app->name % APP_MAP_SIZE];
	if(list == NULL) {
		list = apps[*app->name % APP_MAP_SIZE] = sll_create();
		if(list == NULL)
			return ERR_NOT_ENOUGH_MEM;
	}
	if(!sll_append(list,app))
		return ERR_NOT_ENOUGH_MEM;

	nodeName = kheap_alloc(strlen(app->name) + 1);
	if(nodeName == NULL)
		return ERR_NOT_ENOUGH_MEM;
	strcpy(nodeName,app->name);
	if(vfsn_createInfo(KERNEL_TID,appsNode,nodeName,apps_readHandler) == NULL)
		return ERR_NOT_ENOUGH_MEM;
	return 0;
}

static void apps_rem(sApp *app) {
	sSLList *list;
	assert(app != NULL);

	list = apps[*app->name % APP_MAP_SIZE];
	if(list == NULL)
		return;
	sll_removeFirst(list,app);
}

static s32 apps_readHandler(tTid tid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	UNUSED(file);
	return vfsrw_readHelper(tid,node,buffer,offset,count,0,apps_readCallback);
}

static void apps_readCallback(sVFSNode *node,u32 *dataSize,void **buffer) {
	sApp *app;
	char source[MAX_PATH_LEN];
	sStringBuffer buf;
	buf.str = NULL;
	buf.size = 0;
	buf.len = 0;
	buf.dynamic = true;
	app = apps_get(node->name);
	if(app == NULL || !app_toString(&buf,app,source,false)) {
		*buffer = NULL;
		*dataSize = 0;
	}
	else {
		*buffer = buf.str;
		*dataSize = buf.len + 1;
	}
}

static void apps_listener(sVFSNode *node,u32 event) {
	vid_printf("Listener called for node %s and event 0x%x\n",node->name,event);
}
