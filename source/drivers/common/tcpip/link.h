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

#include <esc/common.h>
#include <esc/messages.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

class Link : public ipc::NIC {
public:
	static const size_t NAME_LEN	= 16;

	explicit Link(const std::string &n,const char *path)
		: ipc::NIC(path,IO_MSGS | IO_READ | IO_WRITE), _rxpkts(), _txpkts(), _rxbytes(), _txbytes(),
		  _mtu(getMTU()), _name(n), _status(ipc::Net::DOWN), _mac(getMAC()), _ip(), _subnetmask() {
	}

	const std::string &name() const {
		return _name;
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

	ipc::Net::Status status() const {
		return _status;
	}
	void status(ipc::Net::Status nstatus) {
		_status = nstatus;
	}

	ulong mtu() const {
		return _mtu;
	}
	const ipc::NIC::MAC &mac() const {
		return _mac;
	}

	const ipc::Net::IPv4Addr &ip() const {
		return _ip;
	}
	void ip(const ipc::Net::IPv4Addr &nip) {
		_ip = nip;
	}

	const ipc::Net::IPv4Addr &subnetMask() const {
		return _subnetmask;
	}
	void subnetMask(const ipc::Net::IPv4Addr &nm) {
		_subnetmask = nm;
	}

	ssize_t read(void *buffer,size_t size) {
		ssize_t res = ::read(fd(),buffer,size);
		if(res > 0) {
			_rxpkts++;
			_rxbytes += res;
		}
		return res;
	}
	ssize_t write(const void *buffer,size_t size) {
		ssize_t res = ::write(fd(),buffer,size);
		if(res > 0) {
			_txpkts++;
			_txbytes += res;
		}
		return res;
	}

private:
	ulong _rxpkts;
	ulong _txpkts;
	ulong _rxbytes;
	ulong _txbytes;
	ulong _mtu;
	std::string _name;
	volatile ipc::Net::Status _status;
	ipc::NIC::MAC _mac;
	ipc::Net::IPv4Addr _ip;
	ipc::Net::IPv4Addr _subnetmask;
};
