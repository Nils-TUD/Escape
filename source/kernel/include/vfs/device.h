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

#include <sys/messages.h>
#include <vfs/channel.h>
#include <vfs/node.h>
#include <common.h>
#include <errno.h>
#include <semaphore.h>

class VFSDevice : public VFSNode {
public:
	/**
	 * Creates a server-node
	 *
	 * @param u the user
	 * @param parent the parent-node
	 * @param name the node-name
	 * @param mode the mode to set
	 * @param type the device-type
	 * @param ops the supported operations
	 * @param success whether the constructor succeeded (is expected to be true before the call!)
	 */
	explicit VFSDevice(const fs::User &u,VFSNode *parent,char *name,uint mode,uint type,uint ops,bool &success);

	virtual bool isDeletable() const override {
		return false;
	}

	/**
	 * @param funcs the functions to check
	 * @return true if the server supports the given functions
	 */
	bool supports(uint funcs) const {
		return (this->funcs & funcs) != 0;
	}

	/**
	 * @return the pid of the owner
	 */
	pid_t getOwner() const {
		return owner;
	}
	/**
	 * @return the thread-id of the creator
	 */
	tid_t getCreator() const {
		return creator;
	}
	/**
	 * Binds this device to the given thread, i.e. binds all existing channels to this thread and
	 * uses <tid> as the default handler for all new channels.
	 *
	 * @param tid the thread-id
	 */
	void bindto(tid_t tid);

	/**
	 * Tells the server that the given channel has been removed. This way, it can reset the internal
	 * state that stores which channel will be served next.
	 *
	 * @param chan the channel
	 */
	void chanRemoved(const VFSChannel *chan);

	/**
	 * Searches for a channel of this device-node that should be served
	 *
	 * @param flags the flags (GW_*)
	 * @return the fd for the channel to retrieve a message from or a an error if there is none
	 */
	int getWork(uint flags);

	/**
	 * Sends the given message to the channel <chan>, which belongs to this device.
	 */
	int send(VFSChannel *chan,ushort flags,msgid_t id,USER const void *data1,
             size_t size1,USER const void *data2,size_t size2);

	/**
	 * Receives a message from the channel <chan>, which belongs to this device.
	 */
	ssize_t receive(VFSChannel *chan,ushort flags,msgid_t *id,USER void *data,size_t size);

	virtual ssize_t getSize() override;
	virtual void close(OpenFile *file,int msgid) override;
	virtual void print(OStream &os) const override;

private:
	void addMsgs(ulong count) {
		msgCount += count;
	}
	void remMsgs(ulong count) {
		assert(msgCount >= count);
		msgCount -= count;
	}

	void wakeupClients();
	int getClientFd(tid_t tid);

	static uint buildMode(uint type);
	static VFSChannel::Message *getMsg(esc::SList<VFSChannel::Message> *list,msgid_t mid,ushort flags);

	/* the process that receives the file descriptors */
	pid_t owner;
	/* the thread that created this device. all channels will initially get bound to this one */
	tid_t creator;
	/* implemented functions */
	uint funcs;
	/* total number of messages in all channels (for the device, not the clients) */
	ulong msgCount;
	/* the last served client */
	const VFSNode *lastClient;
	static SpinLock msgLock;
	static uint16_t nextRid;
};
