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

#include <esc/proto/net.h>
#include <esc/proto/nic.h>
#include <stddef.h>
#include <vector>
#include <string>
#include <limits>

namespace info {
	class link;
	std::istream& operator >>(std::istream& is,link& l);
	std::ostream& operator <<(std::ostream& os,const link& l);

	class link {
		friend std::istream& operator >>(std::istream& is,link& l);
	public:
		static std::vector<link*> get_list();

		explicit link()
			: _rxpkts(), _txpkts(), _rxbytes(), _txbytes(), _mtu(), _name(), _status(),
		      _mac(), _ip(), _subnetmask() {
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

		ulong mtu() const {
			return _mtu;
		}

		const std::string &name() const {
			return _name;
		}
		esc::Net::Status status() const {
			return _status;
		}
		const esc::NIC::MAC &mac() const {
			return _mac;
		}
		const esc::Net::IPv4Addr &ip() const {
			return _ip;
		}
		const esc::Net::IPv4Addr &subnetMask() const {
			return _subnetmask;
		}

	private:
		ulong _rxpkts;
		ulong _txpkts;
		ulong _rxbytes;
		ulong _txbytes;
		ulong _mtu;
		std::string _name;
		esc::Net::Status _status;
		esc::NIC::MAC _mac;
		esc::Net::IPv4Addr _ip;
		esc::Net::IPv4Addr _subnetmask;
	};
}
