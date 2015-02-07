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
#include <sys/messages.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

class Link : public esc::NIC, public std::enable_shared_from_this<Link> {
public:
	static const size_t NAME_LEN	= 16;

	explicit Link(const std::string &n,const char *path)
		: esc::NIC(path,O_RDWRMSG), _rxpkts(), _txpkts(), _rxbytes(), _txbytes(),
		  _mtu(getMTU()), _name(n), _status(esc::Net::DOWN), _mac(getMAC()), _ip(), _subnetmask() {
		sharebuf(fd(),mtu(),&_buffer,&_bufname,0);
		if(_buffer == NULL)
			throw esc::default_error("Not enough memory for buffer",-ENOMEM);
	}
	virtual ~Link();

	const std::string &name() const {
		return _name;
	}
	void *sharedmem() {
		return _buffer;
	}

	ulong txpackets() const {
		return _txpkts;
	}
	ulong txbytes() const {
		return _txbytes;
	}
	ulong rxpackets() const {
		return _rxpkts;
	}
	ulong rxbytes() const {
		return _rxbytes;
	}

	esc::Net::Status status() const {
		return _status;
	}
	void status(esc::Net::Status nstatus) {
		_status = nstatus;
	}

	ulong mtu() const {
		return _mtu;
	}
	const esc::NIC::MAC &mac() const {
		return _mac;
	}

	const esc::Net::IPv4Addr &ip() const {
		return _ip;
	}
	void ip(const esc::Net::IPv4Addr &nip) {
		_ip = nip;
	}

	const esc::Net::IPv4Addr &subnetMask() const {
		return _subnetmask;
	}
	void subnetMask(const esc::Net::IPv4Addr &nm) {
		_subnetmask = nm;
	}

	ssize_t read(void *buffer,size_t size);
	ssize_t write(const void *buffer,size_t size);

private:
	ulong _rxpkts;
	ulong _txpkts;
	ulong _rxbytes;
	ulong _txbytes;
	ulong _mtu;
	std::string _name;
	volatile esc::Net::Status _status;
	esc::NIC::MAC _mac;
	esc::Net::IPv4Addr _ip;
	esc::Net::IPv4Addr _subnetmask;
	ulong _bufname;
	void *_buffer;
};
