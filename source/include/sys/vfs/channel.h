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

#pragma once

#include <sys/common.h>
#include <sys/vfs/node.h>
#include <sys/col/slist.h>

class VFSChannel : public VFSNode {
	struct Message : public SListItem {
		msgid_t id;
		size_t length;
		Thread *thread;
	};

public:
	/**
	 * Creates a new channel for given process
	 *
	 * @param pid the process-id
	 * @param parent the parent-node
	 * @param success whether the constructor succeeded (is expected to be true before the call!)
	 */
	explicit VFSChannel(pid_t pid,VFSNode *parent,bool &success);

	/**
	 * Marks the given channel as used/unused.
	 *
	 * @param used whether to mark it used or unused
	 */
	void setUsed(bool used) {
		this->used = used;
	}

	/**
	 * @return the file-descriptor for the driver to communicate with this channel
	 */
	int getFd() const {
		return fd;
	}

	/**
	 * Checks whether the channel has a reply for the client
	 *
	 * @return true if so
	 */
	bool hasReply() const {
		return recvList.length() > 0;
	}

	/**
	 * Checks whether the channel has work to do for the server
	 *
	 * @return true if so
	 */
	bool hasWork() const {
		return !used && sendList.length() > 0;
	}

	/**
	 * Sends the given message to the channel
	 *
	 * @param pid the process-id
	 * @param flags the flags of the file
	 * @param id the message-id
	 * @param data1 the message-data
	 * @param size1 the data-size
	 * @param data2 for the device-messages: a second message (NULL = no second one)
	 * @param size2 the size of the second message
	 * @return 0 on success
	 */
	ssize_t send(pid_t pid,ushort flags,msgid_t id,USER const void *data1,size_t size1,
	             USER const void *data2,size_t size2);

	/**
	 * Receives a message from the channel
	 *
	 * @param pid the process-id
	 * @param flags the flags of the file
	 * @param id will be set to the message-id (if not NULL)
	 * @param data the buffer to write the message to
	 * @param size the size of the buffer
	 * @param block whether to block if no message is available
	 * @param ignoreSigs whether to ignore signals while sleeping
	 * @return the number of written bytes on success
	 */
	ssize_t receive(pid_t pid,ushort flags,msgid_t *id,void *data,size_t size,bool block,
	                bool ignoreSigs);

	/**
	 * Shares a file with the device that hosts this channel.
	 *
	 * @param pid the process-id
	 * @param file the file
	 * @param path the path to the file
	 * @param cliaddr the address where the client of this channel has mapped it
	 * @param size the size of the mmap'd file
	 * @return 0 on success
	 */
	int sharefile(pid_t pid,OpenFile *file,const char *path,void *cliaddr,size_t size);

	virtual ssize_t open(pid_t pid,OpenFile *file,uint flags);
	virtual off_t seek(pid_t pid,off_t position,off_t offset,uint whence) const;
	virtual size_t getSize(pid_t pid) const;
	virtual ssize_t read(pid_t pid,OpenFile *file,void *buffer,off_t offset,size_t count);
	virtual ssize_t write(pid_t pid,OpenFile *file,const void *buffer,off_t offset,size_t count);
	virtual void close(pid_t pid,OpenFile *file);
	virtual void print(OStream &os) const;

protected:
	virtual void invalidate();

private:
	static Message *getMsg(Thread *t,SList<Message> *list,ushort flags);
	int isSupported(int op) const;

	int fd;
	bool used;
	bool closed;
	void *shmem;
	size_t shmemSize;
	Thread *curClient;
	/* a list for sending messages to the device */
	SList<Message> sendList;
	/* a list for reading messages from the device */
	SList<Message> recvList;
};
