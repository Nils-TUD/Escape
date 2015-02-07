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
#include <esc/stream/ostream.h>
#include <esc/stream/istream.h>
#include <limits>
#include <stddef.h>
#include <string>
#include <vector>

namespace info {
	class arp;
	esc::IStream& operator >>(esc::IStream& is,arp& a);
	esc::OStream& operator <<(esc::OStream& os,const arp& a);

	class arp {
		friend esc::IStream& operator >>(esc::IStream& is,arp& a);
	public:
		static std::vector<arp*> get_list();

		explicit arp() : _ip(), _mac() {
		}

		const esc::Net::IPv4Addr &ip() const {
			return _ip;
		}
		const esc::NIC::MAC &mac() const {
			return _mac;
		}

	private:
		esc::Net::IPv4Addr _ip;
		esc::NIC::MAC _mac;
	};
}
