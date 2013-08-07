/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <sys/mem/cache.h>
#include <sys/mem/virtmem.h>
#include <sys/task/thread.h>
#include <sys/task/signals.h>
#include <sys/task/event.h>
#include <sys/task/proc.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/pipe.h>
#include <sys/vfs/openfile.h>
#include <sys/spinlock.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

VFSPipe::VFSPipe(pid_t pid,VFSNode *p,bool &success)
		: VFSNode(pid,generateId(pid),MODE_TYPE_PIPE | S_IRUSR | S_IWUSR,success), noReader(false),
		  total(0), list() {
	if(!success)
		return;

	/* auto-destroy on the last close() */
	refCount--;
	append(p);
}

VFSPipe::~VFSPipe() {
	for(auto it = list.begin(); it != list.end(); ) {
		auto old = it++;
		delete &*old;
	}
}

size_t VFSPipe::getSize(A_UNUSED pid_t pid) const {
	return total;
}

void VFSPipe::close(pid_t pid,OpenFile *file) {
	/* if there are still more than 1 user, notify the other */
	/* TODO actually, this doesn't work (we need a lock for refCount) */
	if(name != NULL && refCount > 1) {
		/* if thats the read-end, save that there is no reader anymore and wakeup the writers */
		if(file->fcntl(pid,F_GETACCESS,0) == VFS_READ) {
			noReader = true;
			Event::wakeup(EVI_PIPE_EMPTY,(evobj_t)this);
		}
		/* otherwise write EOF in the pipe */
		else
			file->writeFile(pid,NULL,0);
	}
	/* in any case, destroy the node, i.e. decrease references */
	unref();
}

ssize_t VFSPipe::read(A_UNUSED tid_t pid,A_UNUSED OpenFile *file,USER void *buffer,
                      off_t offset,size_t count) {
	Thread *t = Thread::getRunning();

	/* wait until data is available */
	if(!isAlive())
		return -EDESTROYED;
	SpinLock::acquire(&lock);
	while(list.length() == 0) {
		Event::wait(t,EVI_PIPE_FULL,(evobj_t)this);
		SpinLock::release(&lock);

		Thread::switchAway();

		if(Signals::hasSignalFor(t->getTid()))
			return -EINTR;
		SpinLock::acquire(&lock);
		if(!isAlive()) {
			SpinLock::release(&lock);
			return -EDESTROYED;
		}
	}

	PipeData *data = &*list.begin();
	/* empty message indicates EOF */
	if(data->length == 0) {
		SpinLock::release(&lock);
		return 0;
	}

	size_t totalBytes = 0;
	while(true) {
		/* copy */
		vassert(offset >= data->offset,"Illegal offset");
		vassert((off_t)data->length >= (offset - data->offset),"Illegal offset");
		size_t byteCount = MIN(data->length - (offset - data->offset),count);
		Thread::addLock(&lock);
		memcpy((uint8_t*)buffer + totalBytes,data->data + (offset - data->offset),byteCount);
		Thread::remLock(&lock);
		/* remove if read completely */
		if(byteCount + (offset - data->offset) == data->length) {
			list.removeAt(NULL,data);
			Cache::free(data);
		}
		count -= byteCount;
		totalBytes += byteCount;
		total -= byteCount;
		offset += byteCount;
		if(count == 0)
			break;
		/* wait until data is available */
		while(list.length() == 0) {
			/* before we go to sleep we have to notify others that we've read data. otherwise
			 * we may cause a deadlock here */
			Event::wakeup(EVI_PIPE_EMPTY,(evobj_t)this);
			Event::wait(t,EVI_PIPE_FULL,(evobj_t)this);
			SpinLock::release(&lock);

			/* TODO we can't accept signals here, right? since we've already read something, which
			 * we have to deliver to the user. the only way I can imagine would be to put it back.. */
			Thread::switchNoSigs();

			SpinLock::acquire(&lock);
			if(!isAlive()) {
				SpinLock::release(&lock);
				return -EDESTROYED;
			}
		}
		data = &*list.begin();
		/* keep the empty one for the next transfer */
		if(data->length == 0)
			break;
	}
	SpinLock::release(&lock);
	/* wakeup all threads that wait for writing in this node */
	Event::wakeup(EVI_PIPE_EMPTY,(evobj_t)this);
	return totalBytes;
}

ssize_t VFSPipe::write(A_UNUSED pid_t pid,A_UNUSED OpenFile *file,USER const void *buffer,
                       off_t offset,size_t count) {
	Thread *t = Thread::getRunning();

	/* Note that the size-check doesn't ensure that the total pipe-size can't be larger than the
	 * maximum. Its not really critical here, I think. Therefore its enough for making sure that
	 * we don't write all the time without reading most of the data. */

	/* wait while our node is full */
	if(count) {
		if(!isAlive())
			return -EDESTROYED;
		SpinLock::acquire(&lock);
		while((total + count) >= MAX_VFS_FILE_SIZE) {
			Event::wait(t,EVI_PIPE_EMPTY,(evobj_t)this);
			SpinLock::release(&lock);

			Thread::switchNoSigs();

			/* if we wake up and there is no pipe-reader anymore, send a signal to us so that we
			 * either terminate or react on that signal. */
			SpinLock::acquire(&lock);
			if(!isAlive()) {
				SpinLock::release(&lock);
				return -EDESTROYED;
			}
			if(noReader) {
				SpinLock::release(&lock);
				Proc::addSignalFor(pid,SIG_PIPE_CLOSED);
				return -EPIPE;
			}
		}
		SpinLock::release(&lock);
	}

	/* build pipe-data */
	PipeData *data = (PipeData*)Cache::alloc(sizeof(PipeData) + count);
	if(data == NULL)
		return -ENOMEM;
	data->offset = offset;
	data->length = count;
	if(count) {
		Thread::addHeapAlloc(data);
		memcpy(data->data,buffer,count);
		Thread::remHeapAlloc(data);
	}

	/* append */
	SpinLock::acquire(&lock);
	if(!isAlive()) {
		SpinLock::release(&lock);
		Cache::free(data);
		return -EDESTROYED;
	}
	list.append(data);
	total += count;
	SpinLock::release(&lock);
	Event::wakeup(EVI_PIPE_FULL,(evobj_t)this);
	return count;
}
