/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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
#include <esc/messages.h>
#include <errno.h>

class VFSDevice : public VFSNode {
public:
	/**
	 * Creates a server-node
	 *
	 * @param pid the process-id to use
	 * @param parent the parent-node
	 * @param name the node-name
	 * @param mode the mode to set
	 * @param type the device-type
	 * @param ops the supported operations
	 * @param success whether the constructor succeeded (is expected to be true before the call!)
	 */
	explicit VFSDevice(pid_t pid,VFSNode *parent,char *name,mode_t mode,uint type,uint ops,bool &success);

	/**
	 * @param funcs the functions to check
	 * @return true if the server supports the given functions
	 */
	bool supports(uint funcs) const {
		return (this->funcs & funcs) != 0;
	}

	/**
	 * @param id the msg-id to check
	 * @return true if the server accepts the given message
	 */
	bool accepts(uint id) const {
		if(IS_FS(mode) && id != MSG_FS_OPEN_RESP)
			return true;
		return false;
	}

	/**
	 * @return true if data can be read from the server (is available)
	 */
	bool isReadable() const {
		return !isEmpty;
	}

	/**
	 * Sets whether data is available
	 *
	 * @param readable the new value
	 * @return 0 on success
	 */
	int setReadable(bool readable);

	/**
	 * @return true if there is work
	 */
	bool hasWork() const {
		return msgCount > 0;
	}

	/**
	 * Increases the message-count for this device by <count>
	 */
	void addMsgs(ulong count) {
		// we hold the waitlock currently anyway, therefore unlocked
		msgCount += count;
	}

	/**
	 * Decreases the message-count for this device by <count>
	 */
	void remMsgs(ulong count) {
		// we hold the waitlock currently anyway, therefore unlocked
		assert(msgCount >= count);
		msgCount -= count;
	}

	/**
	 * Tells the server that the given client has been removed. This way, it can reset the internal
	 * state that stores which client will be served next.
	 *
	 * @param client the client-node
	 */
	void clientRemoved(const VFSNode *client) {
		/* we don't have to lock this, because its only called in unref(), which can only
		 * be called when the treelock is held. i.e. it is not possible during getwork() */
		if(lastClient == client)
			lastClient = client->next;
	}

	/**
	 * Searches for a channel of this device-node that should be served
	 *
	 * @param flags the getwork-flags
	 * @return the fd for the channel to retrieve a message from or a an error if there is none
	 */
	int getWork(uint flags);

	virtual size_t getSize(pid_t pid) const;
	virtual void close(pid_t pid,OpenFile *file,int msgid);
	virtual void print(OStream &os) const;

private:
	static uint buildMode(uint type);
	void wakeupClients(bool locked);

	/* whether there is data to read or not */
	bool isEmpty;
	/* implemented functions */
	uint funcs;
	/* total number of messages in all channels (for the device, not the clients) */
	ulong msgCount;
	/* the last served client */
	const VFSNode *lastClient;
};
