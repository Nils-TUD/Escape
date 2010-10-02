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
#include <sys/task/event.h>
#include <sys/task/signals.h>
#include <sys/video.h>
#include <esc/messages.h>
#include <string.h>
#include <errors.h>
#include <assert.h>

s32 vfsrw_readHelper(tPid pid,sVFSNode *node,u8 *buffer,u32 offset,u32 count,u32 dataSize,
		fReadCallBack callback) {
	void *mem = NULL;

	UNUSED(pid);
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
